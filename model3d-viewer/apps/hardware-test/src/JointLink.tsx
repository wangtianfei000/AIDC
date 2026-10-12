import {useEffect,useRef,useState} from 'react';
import {record} from './api';

export default function JointLink({id,angle,visible,device,online,onAngle,onActive}:{device:any;online:boolean;id?:number;angle:number;visible:boolean;onAngle:(v:number)=>void;onActive:(v:boolean)=>void}){
 const socket=useRef<WebSocket|null>(null), latest=useRef(angle), generation=useRef(0);
 const timer=useRef<ReturnType<typeof setTimeout>|undefined>(undefined);
 const [active,setActive]=useState(false),[pending,setPending]=useState(false),[error,setError]=useState(''),[feedback,setFeedback]=useState<any>(null),[stamp,setStamp]=useState(0),[now,setNow]=useState(Date.now());
 latest.current=angle;
 const healthy=online&&device?.link_healthy,process=device?.status?.process,mode=device?.telemetry?.mode;
 const fault=process===4;
 const diagnostic=device?.diagnostic||'';
 const reason=diagnostic.includes('trunk tilt')?'机身倾角超过 55°，控制器已拒绝使能。请先扶正机身；若已直立，请复核 IMU 安装标定。':diagnostic;
 const canBegin=healthy&&process===2&&mode===1&&!device?.telemetry?.armed&&!device?.calibration_active&&!device?.imu_active;
 const stop=()=>{generation.current++;clearTimeout(timer.current);socket.current?.close();socket.current=null;setActive(false);setPending(false);onActive(false);};
 useEffect(()=>{stop();setFeedback(null);setError('');return stop;},[id,visible]);
 useEffect(()=>{const hide=()=>{if(document.hidden)stop();};const leave=()=>stop();document.addEventListener('visibilitychange',hide);window.addEventListener('pagehide',leave);const clock=setInterval(()=>setNow(Date.now()),250);return()=>{clearInterval(clock);document.removeEventListener('visibilitychange',hide);window.removeEventListener('pagehide',leave);};},[]);
 const begin=async()=>{
  stop();const version=generation.current;setError('');setPending(true);
  try{
   const response=await fetch('/api/workbench-session');if(!response.ok)throw Error('本机服务不可用');const {token}=await response.json();
   if(version!==generation.current)return;
   const ws=new WebSocket(`ws://${location.host}/ws/joint-link?token=${encodeURIComponent(token)}`);socket.current=ws;let seq=0;
   const expired=()=>{if(version===generation.current){setError('反馈超时，联动已结束；请重新开启');stop();}};
   timer.current=setTimeout(expired,2500);
   ws.onopen=()=>ws.send(JSON.stringify({servo_id:id}));
   ws.onmessage=e=>{
    if(version!==generation.current)return;
    clearTimeout(timer.current);const data=JSON.parse(e.data);
    if(data.error){record('单轴联动',false,data.error);setError(data.error);stop();return;}
    setFeedback(data);setStamp(Date.now());setPending(false);setActive(true);onActive(true);
    if(data.ready){record('单轴联动',true,`ID ${id} 已开启，运行配置 ${data.signature}`);latest.current=data.angle_deg;onAngle(data.angle_deg);}
    timer.current=setTimeout(()=>{
     if(version!==generation.current||ws.readyState!==WebSocket.OPEN)return;
     ws.send(JSON.stringify({servo_id:id,sequence:++seq,angle_deg:latest.current}));
     timer.current=setTimeout(expired,900);
    },150);
   };
   ws.onerror=()=>{if(version===generation.current){setError('实体联动连接失败');stop();}};
   ws.onclose=()=>{if(version===generation.current){setError('联动已结束，需明确重新开启');stop();}};
  }catch(e){if(version===generation.current){setError(String(e));stop();}}
 };
 const controllerAction=async(name:string)=>{try{const r=await fetch('/api/workbench-session');const {token}=await r.json();const a=await fetch('/api/action',{method:'POST',headers:{'Content-Type':'application/json','X-Microduck-Token':token},body:JSON.stringify({name})});if(!a.ok)throw Error((await a.json()).detail);setError(name==='clear_fault'?'已发送清除故障请求；待控制器停止后，再启动只读预检':'控制器启动请求已发送，待 READY 后开启联动');}catch(e){setError(String(e));}};
 const fresh=active&&now-stamp<1000;
 return <section className="card" hidden={!visible}>
  <h3>单关节实体联动</h3><p>托稳机身，停止采样。开启时模型对齐实测姿态，再调整上方角度；仅所选舵机运动，最高 10°/秒。</p>
  <div className="actions"><button disabled={active||pending||!healthy||process!==0} onClick={()=>controllerAction('start')}>启动控制器（只读预检）</button><button className="primary" disabled={!id||active||pending||!canBegin} onClick={begin}>{pending?'核对运行配置…':'开启实体联动'}</button><button disabled={!active&&!pending} onClick={stop}>结束联动并卸力</button>{fault&&<button disabled={!healthy||pending} onClick={()=>controllerAction('clear_fault')}>清除故障</button>}</div>
  {fault&&<p role="alert" className="notice">控制器故障：{reason||'等待详细故障信息'}<br/>当前扭矩：{healthy?(device?.telemetry?.armed?'开启':'关闭'):'未知'}</p>}
  {!fault&&!active&&!pending&&!canBegin&&<p className="notice">{!healthy?'请先连接设备并等待状态更新':process===0?'请先启动控制器，等待 READY 后开启联动':'请停止采样或退出其他运动，等待控制器 READY'}</p>}
  <p className="notice">{active?'模型 + 所选实体关节':'仅模型预览'} · 切换关节、功能页或隐藏页面将结束联动。断流后树莓派独立卸力，恢复连接不会自动续动。</p>
  {feedback&&<dl><dt>目标角度</dt><dd>{angle.toFixed(1)}°</dd><dt>实测角度 / 舵盘</dt><dd>{feedback.angle_deg.toFixed(1)}° / {feedback.tick} tick</dd><dt>反馈</dt><dd>{fresh?`${feedback.age_ms} ms + 本机 ${Math.max(0,now-stamp)} ms`:'已停止 / 过期'}</dd><dt>控制器扭矩状态</dt><dd>{fresh?(feedback.torque?'已开启':'已关闭'):'待确认'}</dd><dt>运行配置</dt><dd>{feedback.signature}（已核对）</dd></dl>}
  {error&&<p role="alert" className="notice">{error}</p>}
 </section>;
}
