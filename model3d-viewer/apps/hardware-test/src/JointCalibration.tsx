import {useEffect,useState} from 'react';
import {request} from './api';
type Props={id?:number;angle:number;visible:boolean};
type Draft={zero:string;direction:number;revision:string;reference?:{tick:number;angle:number;at:string};dirty:boolean};
export default function JointCalibration({id,angle,visible}:Props){
 const [state,setState]=useState<any>(null),[drafts,setDrafts]=useState<Record<number,Draft>>({});
 const [token,setToken]=useState(''),[message,setMessage]=useState(''),[busy,setBusy]=useState(false),[reachable,setReachable]=useState(false);
 useEffect(()=>{let ended=false;let timer:ReturnType<typeof setTimeout>;const ctrl=new AbortController();
  const poll=async()=>{try{const r=await fetch('/api/joint-calibration',{signal:ctrl.signal});if(!r.ok)throw Error('校准服务不可用');const s=await r.json();if(!ended){setState(s);setReachable(true);}}catch{if(!ended)setReachable(false);}finally{if(!ended)timer=setTimeout(poll,700);}};
  void poll();fetch('/api/workbench-session',{signal:ctrl.signal}).then(r=>r.json()).then(r=>{if(!ended)setToken(r.token);}).catch(()=>{});
  return()=>{ended=true;ctrl.abort();clearTimeout(timer);};
 },[]);
 const row=state?.joints.find((j:any)=>j.id===id);
 const draft=id===undefined?undefined:drafts[id];
 const zero=draft?.zero??String(row?.measured_zero_raw??''), direction=draft?.direction??row?.direction??1;
 const change=(patch:Partial<Draft>)=>{if(id!==undefined&&row)setDrafts(d=>({...d,[id]:{...(d[id]??{zero:String(row.measured_zero_raw??''),direction:row.direction??1,revision:state.revision}),...patch,dirty:true}}));};
 const post=async(path:string,body:any={})=>{const s=await request(path,body);setState(s);return s;};
 const run=async(fn:()=>Promise<void>)=>{if(busy)return;setBusy(true);setMessage('');try{await fn();}catch(e){setMessage((e as Error).message);}finally{setBusy(false);}};
 const fresh=reachable&&row?.live?.fresh;
 const canRecord=fresh&&row.live.torque===0;
 const reload=()=>{if(id===undefined||!row)return;setDrafts(d=>({...d,[id]:{zero:String(row.measured_zero_raw??''),direction:row.direction??1,revision:state.revision,dirty:false}}));setMessage('已载入本机保存值');};
 return <section hidden={!visible} className="calibration">
  <div className="actions"><button disabled={busy||!reachable||!token||state?.active} onClick={()=>run(async()=>{await post('joint-calibration/live/start');})}>开始实时读取</button><button disabled={busy||!state?.active} onClick={()=>run(async()=>{await post('joint-calibration/live/stop');})}>停止读取</button></div>
  <p className="notice">只读采集走蓝牙，不需要 SSH。先卸力并关闭控制器，再手动摆放关节。单轴错误不会停止其他轴。</p>
  {!reachable&&<p role="alert">校准服务未连接；输入草稿仍保留。</p>}
  {state?.error&&<p role="alert">读取停止：{state.error}</p>}
  {row?<><div className="card"><h3>① 识别与对齐 · ID {id}</h3><dl><dt>原始位置</dt><dd>{row.live?.position??'—'} tick {fresh?'· 实时':'· 非实时'}</dd><dt>扭矩</dt><dd>{row.live?.torque===0?'关闭':row.live?.torque===1?'开启':'未知'}</dd><dt>读数时间</dt><dd>{row.live?.sampled_at?new Date(row.live.sampled_at).toLocaleTimeString():'尚未读取'}</dd></dl>{row.live?.error&&<p role="alert">{row.live.error}；上方保留最后有效读数</p>}<label className="check"><input type="checkbox" checked={!!row.manual_confirmed} disabled={busy||!token} onChange={e=>{const confirmed=e.target.checked;void run(async()=>{await post('joint-calibration/confirm',{servo_id:id,confirmed});});}}/>已手动确认该 ID 对应此实体关节</label><p>将实体摆到中央模型当前姿态（{angle.toFixed(1)}°），观察 tick 后手动录入。</p><button disabled={!canRecord||busy} onClick={()=>{change({reference:{tick:row.live.position,angle,at:row.live.sampled_at}});setMessage('已记录参考快照；尚未改写零位输入');}}>记录当前读数</button>{draft?.reference&&<p className="notice">参考：{draft.reference.tick} tick / CAD {draft.reference.angle}° / {new Date(draft.reference.at).toLocaleTimeString()}</p>}</div>
  <div className="card"><h3>② 软件零位与方向</h3><label>软件零位 tick（CAD 0°）</label><input aria-label="软件零位 tick" inputMode="numeric" value={zero} onChange={e=>change({zero:e.target.value})}/><label>CAD 正角时 tick 变化</label><select aria-label="关节方向" value={direction} onChange={e=>change({direction:Number(e.target.value)})}><option value={1}>增加（+1）</option><option value={-1}>减少（−1）</option></select><p className="notice">CAD 0° 对齐时，原始 tick 即软件零位。无需扫描完整总线或采样两端。此处不改写舵机内部中位。</p>
  <details><summary>非零参考角换算（可选）</summary><p>使用上方已记录的 tick 和参考角，按当前所选方向计算。结果只填入草稿。</p><button disabled={!draft?.reference} onClick={()=>{const r=draft?.reference;if(r){change({zero:String(Math.round(r.tick-direction*r.angle*4096/360))});setMessage('单点换算已填入零位草稿，请核对后保存');}}}>换算参考姿态</button></details>
  <div className="actions"><button className="primary" disabled={busy||!reachable||!token} onClick={()=>run(async()=>{if(!/^\d+$/.test(zero)||Number(zero)>4095)throw Error('软件零位须为 0–4095 的整数');const savedId=id!;const s=await post('joint-calibration/save',{servo_id:savedId,zero:Number(zero),direction,revision:draft?.revision??state.revision});setDrafts(d=>({...d,[savedId]:d[savedId]&&(d[savedId].zero!==zero||d[savedId].direction!==direction)?{...d[savedId],revision:s.revision}:{...d[savedId],zero,direction,revision:s.revision,dirty:false}}));setMessage(`ID ${savedId} 已保存到本机，并备份原配置；尚未同步树莓派`);})}>保存零位与方向</button><button disabled={busy||!row} onClick={reload}>载入已保存值</button></div>
  {row.warnings.map((w:string)=><p role="alert" key={w}>{w}</p>)}<p>本机：{row.measured_zero_raw} tick / {row.direction===1?'+1':'−1'}{draft?.dirty?' · 输入草稿未保存':''}</p></div>

  <details><summary>舵机内部中位维护</summary><p>内部中位与软件零位不同。维护操作继续使用现有硬件中位工具；修改后应重新记录读数并录入软件零位。</p><a href="/#cal" target="_blank" rel="noreferrer">打开舵机内部中位工具 ↗</a></details>
  </>:<p>选择左侧关节以读取或编辑校准数据。</p>}
  {message&&<p className="notice" role="status">{message}</p>}
 </section>;
}
