import { useEffect,useRef,useState } from 'react';
import {createRoot} from 'react-dom/client';
import Viewport,{type ViewHandle} from './Viewport';
import './style.css';
import JointCalibration from './JointCalibration';
import JointLink from './JointLink';
import DevicePanel from './DevicePanel';
import ImuPanel,{imuPoses} from './ImuPanel';
import MotionPanel,{type SimRoot} from './MotionPanel';
import Diagnostics from './Diagnostics';
type Joint={jointId:string;servoId:number;bodyId:string;zeroRaw:number;direction:number;designLimitDeg:number[];standOffsetDeg:number};
type Part={nodeId:string;nodeName:string;label:string;bodyId:string;nodeType:string};
const panels=['设备连接','关节校准','IMU 标定','运动测试','诊断记录'];
const labels:Record<string,string>={left_hip_yaw:'左髋偏航',left_hip_roll:'左髋侧摆',left_hip_pitch:'左髋俯仰',left_knee:'左膝',left_ankle:'左踝',right_hip_yaw:'右髋偏航',right_hip_roll:'右髋侧摆',right_hip_pitch:'右髋俯仰',right_knee:'右膝',right_ankle:'右踝',neck_pitch:'颈部俯仰',head_yaw:'头部偏航'};
function App(){
 const [panel,setPanel]=useState(0),[joints,setJoints]=useState<Joint[]>([]),[parts,setParts]=useState<Part[]>([]);
 const [selected,setSelected]=useState(''),[pose,setPose]=useState<Record<string,number>>({}),[filter,setFilter]=useState('');
 const [tree,setTree]=useState('joint'),[collapsed,setCollapsed]=useState(false),[transparent,setTransparent]=useState(false),[axes,setAxes]=useState(true);
 const [status,setStatus]=useState('加载模型…'),[error,setError]=useState(''),[device,setDevice]=useState<any>(null),[online,setOnline]=useState(false);
 const [rightWidth,setRightWidth]=useState(370);
 const [linked,setLinked]=useState(false);
 const [simRoot,setSimRoot]=useState<SimRoot|null>(null);
 const [imuReference,setImuReference]=useState('level');
 const view=useRef<ViewHandle|null>(null);
 useEffect(()=>{const ctrl=new AbortController();Promise.all(['hardware-map.json','model3d.manifest.json'].map(n=>fetch(import.meta.env.BASE_URL+n,{signal:ctrl.signal}).then(r=>{if(!r.ok)throw Error(r.statusText);return r.json();}))).then(([map,manifest])=>{setJoints(map.joints);setParts(manifest.productStructure);}).catch(e=>{if(e.name!=='AbortError')setError('基线读取失败：'+e.message);});return()=>ctrl.abort();},[]);
 useEffect(()=>{let stopped=false;let timer:ReturnType<typeof setTimeout>;const ctrl=new AbortController();const poll=async()=>{try{const r=await fetch('/api/state',{signal:ctrl.signal});if(!r.ok)throw Error();const s=await r.json();if(!stopped){setDevice(s);setOnline(true);}}catch{if(!stopped)setOnline(false);}finally{if(!stopped)timer=setTimeout(poll,1500);}};void poll();return()=>{stopped=true;ctrl.abort();clearTimeout(timer);};},[]);
 const part=parts.find(p=>p.nodeName===selected), joint=joints.find(j=>j.bodyId===selected||j.bodyId===part?.bodyId);
 const angle=joint?pose[joint.jointId]??0:0;
 const setAngle=(v:number)=>{if(joint&&Number.isFinite(v))setPose(p=>({...p,[joint.jointId]:Math.max(joint.designLimitDeg[0],Math.min(joint.designLimitDeg[1],v))}));};
 const choose=(name:string)=>{setSelected(name);};
 const conn=!online?'本机服务未连接':device?.connected?(device.link_healthy?'BLE 已连接':'BLE 数据中断'):'设备未连接';
 return <div className="app">
  <header><div className="brand"><span className="logo">M</span><div><h1>Microduck <b>硬件测试平台</b></h1><small>HARDWARE WORKBENCH · P4</small></div></div><div className="header-right"><span className={'badge '+(device?.connected&&online?'good':'')}>{conn}</span><span className="badge">{linked?"单关节联动中":"中央视图：模型参考"}</span><button onClick={()=>setPanel(3)}>整机动作</button></div></header>
  <div className="workspace" style={{'--right-width':rightWidth+'px'} as React.CSSProperties}>
   <aside className={'tree '+(collapsed?'collapsed':'')}><div className="section-title"><strong>{!collapsed&&'模型结构'}</strong><button aria-label={collapsed?'展开模型树':'收起模型树'} onClick={()=>setCollapsed(!collapsed)}>{collapsed?'›':'‹'}</button></div>{!collapsed&&<><div className="segments"><button className={tree==='joint'?'active':''} onClick={()=>setTree('joint')}>关节 · 12</button><button className={tree==='part'?'active':''} onClick={()=>setTree('part')}>部件 · 58</button></div><input aria-label="搜索关节或部件" placeholder="搜索名称 / ID" value={filter} onChange={e=>setFilter(e.target.value)}/><div className="tree-list">{tree==='joint'?['左腿','右腿','头颈'].map((group,i)=><div key={group}><h3>{group}</h3>{joints.filter(j=>i===0?j.jointId.startsWith('left'):i===1?j.jointId.startsWith('right'):!j.jointId.match(/^(left|right)/)).filter(j=>(labels[j.jointId]+j.servoId+j.jointId).toLowerCase().includes(filter.toLowerCase())).map(j=><button key={j.jointId} className={'tree-item '+(joint?.jointId===j.jointId?'selected':'')} onClick={()=>choose(j.bodyId)}><span>◇ {labels[j.jointId]}</span><small>ID {j.servoId}</small></button>)}</div>):parts.filter(p=>p.nodeType==='part'&&(p.label+p.nodeName).toLowerCase().includes(filter.toLowerCase())).map(p=><button key={p.nodeId} className={'tree-item part '+(selected===p.nodeName?'selected':'')} onClick={()=>choose(p.nodeName)} title={p.label}>{p.label}</button>)}</div><div className="tree-foot">CAD 12 轴基线<br/>模型坐标：X 前 · Y 左 · Z 上</div></>}</aside>
   <main className="model-area"><nav className="model-tools"><span className="model-name">Microduck Native12</span><div>{[['iso','轴测'],['front','前'],['left','左'],['top','顶']].map(([k,t])=><button key={k} onClick={()=>view.current?.view(k)}>{t}</button>)}<button onClick={()=>view.current?.view('iso')}>适配</button><button className={transparent?'active':''} onClick={()=>setTransparent(!transparent)}>透明</button><button className={axes?'active':''} onClick={()=>setAxes(!axes)}>坐标轴</button></div></nav><Viewport simRoot={panel===3?simRoot:null} bodyEuler={panel===2?imuPoses[imuReference].euler:[0,0,0]} pose={pose} selected={selected} transparent={transparent} axes={axes} onSelect={choose} onReady={v=>{view.current=v;}} onStatus={setStatus}/><div className="model-caption"><span>{panel===2?`IMU 参考：${imuPoses[imuReference].name}`:status}</span><span>拖动旋转 · 滚轮缩放 · 点击选择</span></div>{error&&<div className="model-error">{error}</div>}</main>
   <div role="separator" aria-label="调整功能区宽度" aria-orientation="vertical" tabIndex={0} className="resize" onKeyDown={e=>{if(e.key==='ArrowLeft')setRightWidth(w=>Math.min(520,w+20));if(e.key==='ArrowRight')setRightWidth(w=>Math.max(320,w-20));}} onPointerDown={e=>{e.currentTarget.setPointerCapture(e.pointerId);}} onPointerMove={e=>{if(e.currentTarget.hasPointerCapture(e.pointerId))setRightWidth(Math.max(320,Math.min(520,window.innerWidth-e.clientX-12)));}}/>
   <aside className="functions"><nav className="tabs" aria-label="测试功能">{panels.map((name,i)=><button key={name} aria-pressed={panel===i} className={panel===i?'active':''} onClick={()=>setPanel(i)}>{name}</button>)}</nav><div className="panel">
    <div className="eyebrow">{String(panel+1).padStart(2,'0')} / HARDWARE TEST</div><h2>{panels[panel]}</h2>
    <DevicePanel visible={panel===0} device={device} online={online}/>
    {(panel===1)&&<><div className="selection"><span>当前选择</span><h3>{joint?`${labels[joint.jointId]} · ID ${joint.servoId}`:part?.label||'选择一个关节'}</h3><small>{part?.nodeType==='part'?'所选部件所属刚体；不会自动驱动实体':joint?.jointId||'点击左侧关节或中央模型'}</small></div>{joint&&<><label>模型目标角度 <strong>{angle.toFixed(1)}°</strong></label><input aria-label="模型角度滑块" type="range" min={joint.designLimitDeg[0]} max={joint.designLimitDeg[1]} step="0.5" value={angle} onChange={e=>setAngle(Number(e.target.value))}/><div className="angle-row"><input aria-label="模型目标角度" type="number" min={joint.designLimitDeg[0]} max={joint.designLimitDeg[1]} step="0.5" value={angle} onChange={e=>{if(e.target.value!=='')setAngle(Number(e.target.value));}}/><span>°　{joint.designLimitDeg.join(' ～ ')}°</span><button onClick={()=>setAngle(0)}>本轴归零</button></div></>}
    </>}
    <JointLink device={device} online={online} key={panel} id={joint?.servoId} angle={angle} visible={panel===1} onAngle={setAngle} onActive={setLinked}/>
    <JointCalibration id={joint?.servoId} angle={angle} visible={panel===1}/>
    <ImuPanel visible={panel===2} reference={imuReference} setReference={setImuReference}/>
    <MotionPanel visible={panel===3} device={device} online={online} linked={linked} joints={joints} setPose={setPose} setRoot={setSimRoot}/>
    <Diagnostics visible={panel===4} device={device} online={online} status={status}/>
   </div><div className="panel-bottom">P4 统一测试工作台<br/>BLE 采集与控制 · 本机校准配置</div></aside>
  </div><footer><span><i/> {conn}</span><span>{joint?`${labels[joint.jointId]} / ID ${joint.servoId}`:'未选择关节'}</span><span>目标姿态 ≠ 实测姿态</span><span className="footer-end">12 轴 · CAD 基线</span></footer>
 </div>;
}
createRoot(document.getElementById('root')!).render(<App/>);
