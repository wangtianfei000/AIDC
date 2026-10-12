import MotionRecording from './MotionRecording';
import {useEffect,useRef,useState} from 'react';
import {request,token} from './api';
export type SimRoot={position:number[];quaternion:number[]};
type Joint={jointId:string;standOffsetDeg:number;designLimitDeg:number[]};
export default function MotionPanel({visible,device,online,linked,joints,setPose,setRoot}:{visible:boolean;device:any;online:boolean;linked:boolean;joints:Joint[];setPose:(p:Record<string,number>)=>void;setRoot:(p:SimRoot|null)=>void}){
 const [physical,setPhysical]=useState(false),[modelMode,setModelMode]=useState('加载 ONNX…'),[held,setHeld]=useState(''),[message,setMessage]=useState(''),[busy,setBusy]=useState(false),[ready,setReady]=useState(false),[simReady,setSimReady]=useState(false);
 const [awaiting,setAwaiting]=useState('');
 const [popup,setPopup]=useState('');const errorDialog=useRef<HTMLDialogElement|null>(null);
 const notify=(text:string)=>{setMessage(text);setPopup(text);};
 useEffect(()=>{if(popup&&!errorDialog.current?.open)errorDialog.current?.showModal();},[popup]);
 const sending=useRef(false),latestDevice=useRef(device);latestDevice.current=device;
 const [modelSpeed,setModelSpeed]=useState(.1),[physicalSpeed,setPhysicalSpeed]=useState(.1),[modelTurn,setModelTurn]=useState(.2),[physicalTurn,setPhysicalTurn]=useState(.2),[simInfo,setSimInfo]=useState<any>(null);
 const [simPolicy,setSimPolicy]=useState('w3'),[forceBody,setForceBody]=useState('head_yaw_link'),[forceN,setForceN]=useState(.3),[forceDuration,setForceDuration]=useState(.15),[forceAngle,setForceAngle]=useState(90);
 const speed=physical?physicalSpeed:modelSpeed,turnSpeed=physical?physicalTurn:modelTurn;
 const setSpeed=physical?setPhysicalSpeed:setModelSpeed,setTurnSpeed=physical?setPhysicalTurn:setModelTurn;
 const speeds=useRef({speed,turnSpeed});speeds.current={speed,turnSpeed};
 const ws=useRef<WebSocket|null>(null),sim=useRef<WebSocket|null>(null),pending=useRef(false),timer=useRef<ReturnType<typeof setInterval>|undefined>(undefined),holding=useRef('');
 const sendSim=(v:unknown)=>{if(sim.current?.readyState===WebSocket.OPEN){sim.current.send(JSON.stringify(v));return true;}return false;};
 const stop=()=>{const wasHolding=!!holding.current;clearInterval(timer.current);holding.current='';setHeld('');if(wasHolding){if(ws.current?.readyState===WebSocket.OPEN)ws.current.send(JSON.stringify({type:'stop_motion'}));sendSim({type:'stop'});}};
 useEffect(()=>{stop();setSimReady(false);if(!visible||physical)return;let disposed=false;let channel:WebSocket|null=null;
  token().then(t=>{if(disposed)return;channel=new WebSocket(`ws://${location.host}/ws/model-sim?token=${encodeURIComponent(t)}`);sim.current=channel;setModelMode('加载 ONNX 与物理模型…');setMessage('');
   channel.onopen=()=>channel?.send(JSON.stringify({type:'select_policy',policy:simPolicy}));
   channel.onmessage=e=>{if(disposed)return;pending.current=false;const v=JSON.parse(e.data);if(v.type==='error'){stop();setSimReady(false);setMessage(v.message);return;}setSimReady(true);setSimInfo(v);setPose(v.pose);setRoot(v.root);setModelMode(holding.current?`ONNX ${holding.current}`:'仿真已暂停');};
   channel.onerror=()=>{if(!disposed){stop();setMessage('ONNX 仿真连接失败');}};channel.onclose=()=>{if(!disposed){stop();setSimReady(false);}};
  }).catch(e=>{if(!disposed)setMessage(String(e));});
  return()=>{disposed=true;stop();channel?.close();if(sim.current===channel)sim.current=null;pending.current=false;};
 },[visible,physical,simPolicy]);
 useEffect(()=>{stop();setReady(false);if(!visible||!physical)return;setRoot(null);let disposed=false;let channel:WebSocket|null=null;
  token().then(t=>{if(disposed)return;channel=new WebSocket(`ws://${location.host}/ws?token=${encodeURIComponent(t)}`);ws.current=channel;channel.onopen=()=>{if(!disposed)setReady(true);};channel.onmessage=e=>{if(disposed)return;const v=JSON.parse(e.data);if(v.type==='error'){notify(v.message);stop();}else if(v.type==='state'&&(!v.data.link_healthy||![4,5,11].includes(v.data.telemetry?.mode)))stop();};channel.onerror=()=>{if(disposed)return;notify('运动通道不可用；若旧控制页占用，请关闭旧页后重新进入实体模式');stop();};channel.onclose=()=>{if(disposed)return;setReady(false);stop();};}).catch(e=>setMessage(String(e)));
  return()=>{disposed=true;stop();channel?.close();if(ws.current===channel)ws.current=null;};
 },[visible,physical]);
 useEffect(()=>{if(linked||(physical&&(!online||!device?.link_healthy)))stop();},[physical,online,device?.link_healthy,linked]);
 useEffect(()=>{const halt=()=>stop();const hidden=()=>{if(document.hidden)stop();};window.addEventListener('blur',halt);window.addEventListener('pagehide',halt);document.addEventListener('visibilitychange',hidden);return()=>{halt();window.removeEventListener('blur',halt);window.removeEventListener('pagehide',halt);document.removeEventListener('visibilitychange',hidden);};},[]);
 const mode=device?.telemetry?.mode,process=device?.status?.process,healthy=online&&device?.link_healthy;
 const walkAllowed=!linked&&(physical?healthy&&ready&&[4,5,11].includes(mode):simReady);
 const labels:Record<string,string>={start:'启动控制器',stand:'站立姿态',hold_zero:'保持零位姿态',stop_walk:'停止 · ONNX 平衡',release:'紧急卸力',stop:'关闭控制器',clear_fault:'清除故障'};
 const connectionReason=!online?'本机服务未连接，请刷新页面检查服务。':!device?.connected?'机器鸭未连接，请到“设备连接”连接蓝牙。':!healthy?'蓝牙反馈已过期，请到“设备连接”断开后重新连接。':'';
 const reason=(name:string)=>{
  if(!physical)return !simReady?'模型正在加载，请稍候。':'';
  if(connectionReason)return connectionReason;
  if((busy||awaiting)&&name!=='release')return '正在等待上一条命令完成；紧急卸力仍可用。';
  if(linked&&name!=='release'&&name!=='stop_walk')return '请先结束关节实体联动。';
  if(name==='release')return device?.telemetry?.armed||(process===2&&[1,12,13].includes(mode))?'':'舵机已经卸力，无需重复操作。';
  if(name==='clear_fault')return process===4?'':'当前没有可清除的控制器故障。';
  if(name==='stop')return device?.telemetry?.armed?'请托稳机身，先紧急卸力，再关闭控制器。':[2,4].includes(process)?'':'控制器已经关闭。';
  if(process===4)return '控制器故障：'+(device?.diagnostic||'请查看诊断记录')+'。请先清除故障，再启动。';
  if(name==='start')return process===0?'':'控制器已启动或正在预检，请查看上方状态。';
  if(name==='stand'||name==='hold_zero')return process===2&&mode===1?'':'需要控制器 READY 且已卸力；先启动预检，切换姿态前先卸力。';
  return [3,4,5,11].includes(mode)?'':'请先完成站立或零位姿态，再进入 ONNX 平衡。';
 };
 useEffect(()=>{
  if(!awaiting)return;
  const id=setInterval(()=>{
   const d=latestDevice.current,p=d?.status?.process,m=d?.telemetry?.mode;
   if(!d?.link_healthy)return;
   if(p===4&&awaiting!=='clear_fault'&&awaiting!=='stop'&&awaiting!=='release'){notify('执行失败：'+(d?.diagnostic||'控制器进入故障状态'));setAwaiting('');return;}
   const done=awaiting==='start'?p===2&&m===1:awaiting==='stand'?m===4&&d?.telemetry?.armed:awaiting==='hold_zero'?m===11&&d?.telemetry?.armed:awaiting==='release'?!d?.telemetry?.armed:awaiting==='stop'||awaiting==='clear_fault'?p===0:awaiting==='stop_walk'?m===4&&d?.status?.vx_mm_s===0&&d?.status?.wz_mrad_s===0:false;
   if(done){notify(labels[awaiting]+'：控制器反馈已确认');setAwaiting('');}
  },250);
  const timeout=setTimeout(()=>{notify(labels[awaiting]+'：12 秒内未确认完成，请检查连接与控制器状态；未自动重试。');setAwaiting('');},12000);
  return()=>{clearInterval(id);clearTimeout(timeout);};
 },[awaiting]);
 const preset=(zero:boolean)=>setPose(Object.fromEntries(joints.map(j=>[j.jointId,zero?0:j.standOffsetDeg])));
 const action=async(name:string)=>{const blocked=reason(name);if(blocked){notify(blocked);return;}if(sending.current){notify('蓝牙请求正在发送，请稍候。');return;}stop();setMessage('');if(!physical){if(name==='stop_walk'){hold('零速度平衡',0,0);return;}sendSim({type:name==='stand'?'stand':name==='hold_zero'?'zero':'stop'});return;}
  sending.current=true;setBusy(true);setAwaiting('');setMessage('正在发送：'+labels[name]+'…');try{await request('action',{name});if(name==='stand'||name==='hold_zero')preset(name==='hold_zero');setMessage(labels[name]+'：请求已送达，等待控制器确认…');setAwaiting(name);}catch(e){notify('发送失败：'+String(e));}finally{sending.current=false;setBusy(false);}};
 const hold=(name:string,x:number,y:number)=>{if(holding.current||!walkAllowed)return;holding.current=name;setHeld(name);
  // Policy starts from STAND, including after the static zero-pose reference.
  if(!physical&&simInfo?.time===0)sendSim({type:'stand'});
  const tick=()=>{const {speed:v,turnSpeed:w}=speeds.current;if(physical){if(ws.current?.readyState===WebSocket.OPEN)ws.current.send(JSON.stringify({type:'motion',x:x*Math.min(1,w/(device?.motion_limits?.turn_mrad_s/1000||.2)),y:y*Math.min(1,v/(device?.motion_limits?.forward_mm_s/1000||.1))}));else stop();}
   else if(!pending.current){pending.current=sendSim({type:'step',vx:y*v,wz:-x*w});}};
  tick();if(holding.current)timer.current=setInterval(tick,physical?100:40);};
 const buttons=[['前进',0,1],['左转',-1,0],['后退',0,-1],['右转',1,0]] as const;
 const maxV=physical?(device?.motion_limits?.forward_mm_s??100)/1000:(simInfo?.limits?.forward??.4),maxW=physical?(device?.motion_limits?.turn_mrad_s??200)/1000:(simInfo?.limits?.yaw??1);
 return <section hidden={!visible}><div className="segments"><button className={!physical?'active':''} onClick={()=>{stop();setPhysical(false);}}>仅模型</button><button disabled={linked} className={physical?'active':''} onClick={()=>{stop();setPhysical(true);}}>实体控制</button></div><p>{physical?'托稳机身后明确选择动作。模型画面不是实体实测姿态。':'ONNX 策略 + MuJoCo 物理仿真，显示实算关节、机身位移与姿态。点击方向持续运行，可在运动中施力；点击暂停结束。'}</p><div className="card"><h3>{physical?'控制器反馈':'模型状态'}</h3><p>{physical?`${device?.status?.process_name??'未知'} / ${(device?.telemetry?.mode_name==='STAND'?'站立 / 零速（是否运行策略取决于所选动作）':device?.telemetry?.mode_name)??'未知'}`:modelMode}</p>{!physical&&simInfo&&<p>{simInfo.policy}<br/>仿真 {simInfo.time.toFixed(2)} s · 推理 {simInfo.inferences} 次<br/>命令 {simInfo.command[0].toFixed(3)} m/s · {simInfo.command[2].toFixed(3)} rad/s<br/>机身位置 X {simInfo.root.position[0].toFixed(3)} m · Y {simInfo.root.position[1].toFixed(3)} m</p>}{physical&&<p>扭矩：{healthy?(device?.telemetry?.armed?'开启':'关闭'):'未知'}<br/>标定：{healthy?(device?.status?.calibrated?'已标定':'未标定'):'未知'}<br/>速度反馈：{healthy?`${device?.status?.vx_mm_s??0} mm/s · ${device?.status?.wz_mrad_s??0} mrad/s`:'—'}</p>}</div>
 {!physical&&<div className="card"><h3>CPU 仿真策略与外力</h3><label>仿真模型<select aria-label="仿真策略" value={simPolicy} onChange={e=>{stop();setSimPolicy(e.target.value);}}><option value="w3">W3 行走抗扰候选（未通过发布验收）</option><option value="v7">原 v7 行走基线</option></select></label><p>仅作用于中央仿真模型。头部由策略控制；方向为世界坐标。</p><label>受力部位<select aria-label="受力部位" value={forceBody} onChange={e=>setForceBody(e.target.value)}><option value="head_yaw_link">头部</option><option value="trunk">机身</option></select></label><label>推力（N，0–5）<input aria-label="外力大小" type="number" min="0" max="5" step="0.1" value={forceN} onChange={e=>setForceN(Number(e.target.value))}/></label><label>持续时间（秒，0.02–2）<input aria-label="外力持续时间" type="number" min="0.02" max="2" step="0.05" value={forceDuration} onChange={e=>setForceDuration(Number(e.target.value))}/></label><label>方向角（°，0=前，90=左）<input aria-label="外力方向" type="number" step="45" value={forceAngle} onChange={e=>setForceAngle(Number(e.target.value))}/></label><div className="actions"><button disabled={!simReady} onClick={()=>{stop();hold('零速度平衡',0,0);}}>运行平衡仿真</button><button disabled={!simReady} onClick={()=>{if(!Number.isFinite(forceN)||!Number.isFinite(forceDuration)||!Number.isFinite(forceAngle)||forceN<0||forceN>5||forceDuration<.02||forceDuration>2){setMessage('请检查推力和持续时间范围');return;}if(!holding.current)hold('零速度平衡',0,0);sendSim({type:'force',body:forceBody,newtons:forceN,duration:forceDuration,angle:forceAngle});}}>施加一次外力</button><button disabled={!simReady} onClick={()=>sendSim({type:'clear_force'})}>立即撤力</button><button onClick={stop}>暂停仿真</button></div><p>{simInfo?.force?.active?`外力 ${simInfo.force.newtons.toFixed(2)} N · 剩余 ${simInfo.force.remaining.toFixed(2)} 秒`:'外力已撤去'}</p><small>施力后继续运行，观察撤力后的恢复；切页或窗口失焦会暂停并撤力。</small></div>}
 <div className="card"><label>前后速度（m/s，最大 {maxV}）<input aria-label="前后速度" type="number" min="0" max={maxV} step="0.01" value={speed} onChange={e=>setSpeed(Math.max(0,Math.min(maxV,Number(e.target.value)||0)))}/></label><label>转向速度（rad/s，最大 {maxW}）<input aria-label="转向速度" type="number" min="0" max={maxW} step="0.05" value={turnSpeed} onChange={e=>setTurnSpeed(Math.max(0,Math.min(maxW,Number(e.target.value)||0)))}/></label></div>
 <p hidden={physical||speed<=0||speed>=.08} className="notice">当前 v7 策略在 0.05 m/s 下实测会保持站立。前进预览建议使用 0.1 m/s；输入速度会原样传入策略，不自动放大。</p><div className="actions motion-actions">{(physical?['start','stand','hold_zero','stop_walk','release','stop','clear_fault']:['stand','hold_zero','stop_walk']).map(name=><button key={name} aria-disabled={!!reason(name)} title={reason(name)||labels[name]} onClick={()=>action(name)}>{labels[name]}{busy&&physical?'（发送中）':awaiting===name?'（等待确认）':name==='start'?'（只读预检）':''}</button>)}
 </div><div className="direction-pad">{buttons.map(([name,x,y])=><button key={name} className={held===name?'active':''} disabled={!walkAllowed} onClick={()=>{if(!physical){stop();hold(name,x,y);}}} onPointerDown={e=>{if(!physical||e.button!==0)return;e.preventDefault();e.currentTarget.setPointerCapture(e.pointerId);hold(name,x,y);}} onPointerUp={()=>{if(physical)stop();}} onPointerCancel={()=>{if(physical)stop();}} onLostPointerCapture={()=>{if(physical)stop();}} onKeyDown={e=>{if(physical&&(e.key===' '||e.key==='Enter')&&!e.repeat){e.preventDefault();hold(name,x,y);}}} onKeyUp={()=>{if(physical)stop();}} onBlur={()=>{if(physical)stop();}}>{name}</button>)}</div><p>{physical?'按住运动，松开停止。':'点击方向持续仿真，点击暂停停止。'}{physical?`实体速度受后端上限约束；${device?.motion_limits?.watchdog_ms??300} ms 行走看门狗。`:'本机仿真，不会发送实体命令；零位为静态参考，行走从站立姿态开始。'}</p>{physical&&!walkAllowed&&<p className="notice">先完成预检，再选择保持零位姿态或站立姿态；到位后可直接行走或进入 ONNX 平衡。</p>}{!physical&&message&&<p role="status">{message}</p>}{physical&&<p className="notice">零位到位后可直接进入 ONNX，无需先切换站立姿态。策略接管后可能离开零位。站立姿态仅保持设定站姿；点击方向键或“停止 · ONNX 平衡”才启用策略。停止将速度归零，ONNX 继续根据关节与 IMU 反馈控制平衡，零速策略也可能迈步。要结束控制请先托住机身并紧急卸力。抗扰效果仍取决于候选策略，不能保证受推不倒。</p>}<MotionRecording visible={visible&&physical}/><dialog ref={errorDialog} className="motion-dialog" onClose={()=>setPopup('')} aria-labelledby="motion-error-title"><h3 id="motion-error-title">操作提示</h3><p>{popup}</p><form method="dialog"><button autoFocus>知道了</button></form></dialog></section>;
}
