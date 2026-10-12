import {useEffect,useState} from 'react';
import {request} from './api';
export const imuPoses:Record<string,{name:string;tip:string;euler:number[]}>= {
 level:{name:'水平正立',tip:'+Z 朝上，机身水平',euler:[0,0,0]},
 nose_down:{name:'机头朝下',tip:'+X 朝地面，整机前倾 90°',euler:[0,90,0]},
 nose_up:{name:'机头朝上',tip:'+X 朝天空，整机后仰 90°',euler:[0,-90,0]},
 left_down:{name:'左侧朝下',tip:'+Y 朝地面，整机向左侧翻 90°',euler:[-90,0,0]},
 right_down:{name:'右侧朝下',tip:'−Y 朝地面，整机向右侧翻 90°',euler:[90,0,0]},
 upside_down:{name:'机身倒置',tip:'+Z 朝地面，整机翻转 180°',euler:[180,0,0]},
};
const vec=(v?:number[])=>v?v.map(n=>n.toFixed(4)).join(', '):'—';
export default function ImuPanel({visible,reference,setReference}:{visible:boolean;reference:string;setReference:(v:string)=>void}){
 const [state,setState]=useState<any>(null),[online,setOnline]=useState(false),[busy,setBusy]=useState(false),[message,setMessage]=useState('');
 useEffect(()=>{let done=false;let timer:ReturnType<typeof setTimeout>;const poll=async()=>{try{const s=await request('imu-mount');if(!done){setState(s);setOnline(true);}}catch{if(!done)setOnline(false);}if(!done)timer=setTimeout(poll,600);};void poll();return()=>{done=true;clearTimeout(timer);};},[]);
 const run=async(path:string,body={})=>{setBusy(true);setMessage('');try{setState(await request('imu-mount/'+path,body));setMessage('操作已完成');}catch(e){setMessage(String(e));}finally{setBusy(false);}};
 const latest=state?.latest,fresh=online&&state?.active&&state.age_s!==null&&state.age_s<5;
 return <section hidden={!visible}><p>托稳整机并卸力、关闭控制器。中央模型显示指定参考姿态，不跟随传感器读数。机身坐标：X 前、Y 左、Z 上。</p><div className="actions"><button disabled={busy||!online||state?.active} onClick={()=>run('start')}>开始 IMU 实时采集</button><button disabled={busy||!state?.active} onClick={()=>run('stop')}>停止采集</button></div><div className="card"><h3>BLE 实测数据</h3><p>{fresh?'正在采集':'未采集 / 数据过期'} · 帧 {latest?.sample_id??'—'} · 累计 {state?.samples_received??0}<br/>最近报告 {state?.age_s??'—'} 秒前</p><dl><dt>重力方向（传感器坐标）</dt><dd>{vec(latest?.gravity)}</dd><dt>角速度 rad/s</dt><dd>{vec(latest?.gyro)}</dd></dl><small>现有 BLE 协议传输重力方向和角速度，未提供原始加速度或完整实时姿态四元数。读数不变时仍以帧号、时间判断更新。</small></div><label>指定机身姿态</label><select aria-label="IMU 指定姿态" value={reference} onChange={e=>setReference(e.target.value)}>{Object.entries(imuPoses).map(([key,p])=><option key={key} value={key}>{p.name}{state?.captures?.[key]?' ✓':''}</option>)}</select><p>{imuPoses[reference].tip}，稳定约 1 秒后采样。</p><button disabled={busy||!fresh} onClick={()=>run('capture',{pose:reference})}>记录此姿态</button><p>已记录 {Object.keys(state?.captures||{}).length} / 6</p><div className="actions"><button disabled={busy||Object.keys(state?.captures||{}).length!==6} onClick={()=>run('calculate')}>计算安装方向</button><button disabled={busy||!state?.candidate} onClick={()=>run('apply-local')}>保存安装方向到本机</button></div><div className="card"><h3>IMU 安装四元数 · w,x,y,z</h3><p>候选：{vec(state?.candidate?.quaternion_wxyz)}</p><p>已保存：{vec(state?.installed_wxyz)}</p><small>表示传感器到机身的固定安装旋转，与当前机身姿态不同。</small><p>{state?.saved_local?'本机已保存':'本轮未保存'} · {state?.pi_synced?'Pi 已写入并读回':'Pi 未核实'} · 运行加载待重启核实</p>{state?.candidate&&<pre>{JSON.stringify(state.candidate.pose_error_deg,null,2)}</pre>}</div>{(message||state?.error||state?.notice)&&<p role="status">{[message,state?.error,state?.notice].filter(Boolean).join('；')}</p>}</section>;
}
