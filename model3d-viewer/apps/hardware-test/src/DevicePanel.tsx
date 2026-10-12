import {useState} from 'react';
import {request} from './api';
export default function DevicePanel({visible,device,online}:{visible:boolean;device:any;online:boolean}){
 const [devices,setDevices]=useState<any[]>([]),[busy,setBusy]=useState(''),[error,setError]=useState('');
 const run=async(name:string,path:string,body={})=>{setBusy(name);setError('');try{const s=await request(path,body);if(path==='scan')setDevices(s.devices);}catch(e){setError(String(e));}finally{setBusy('');}};
 return <section hidden={!visible}><p>扫描只发现设备，点击连接后建立 BLE 通道。重连不会自动启动或恢复运动。</p><div className="actions"><button disabled={!!busy||!online||device?.connected} onClick={()=>run('扫描中…','scan')}>扫描机器鸭</button><button disabled={!!busy||!device?.connected} onClick={()=>run('断开中…','disconnect')}>断开连接</button></div><p>{busy||(!online?'本机服务不可用':device?.connected?'已连接 '+device.address:'当前未连接')}</p>{devices.map(d=><div className="card" key={d.address}><b>{d.name}</b><p>{d.address}<br/>{device?.connected&&device.address===d.address?'已连接':'已发现 · 未连接'}</p><button disabled={!!busy||device?.connected} onClick={()=>run('连接中…','connect',{address:d.address})}>连接</button></div>)}{error&&<p role="alert">{error}</p>}</section>;
}
