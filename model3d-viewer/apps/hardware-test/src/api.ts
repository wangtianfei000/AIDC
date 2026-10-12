export type EventRecord={at:string;stage:string;ok:boolean;message:string};
export const records:EventRecord[]=[];
export function record(stage:string,ok:boolean,message:string){records.unshift({at:new Date().toISOString(),stage,ok,message});records.splice(200);}
export async function token(){const r=await fetch('/api/workbench-session',{signal:AbortSignal.timeout(8000)});if(!r.ok)throw Error('本机服务不可用');return (await r.json()).token as string;}
export async function request(path:string,body?:unknown){
 try{const headers:Record<string,string>={};if(body!==undefined){headers['Content-Type']='application/json';headers['X-Microduck-Token']=await token();}
 const r=await fetch('/api/'+path,{method:body===undefined?'GET':'POST',signal:AbortSignal.timeout(path==='scan'?35000:path==='connect'?60000:15000),headers,body:body===undefined?undefined:JSON.stringify(body)});
 const data=await r.json();if(!r.ok)throw Error(data.detail||'请求失败');if(body!==undefined)record(path,true,'操作已接收');return data;
 }catch(e){const error=e instanceof Error&&e.name==='TimeoutError'?new Error(path==='scan'?'蓝牙扫描超时，请检查电脑蓝牙是否开启后重试。':path==='connect'?'蓝牙连接超时，请确认机器鸭在线且未被其他设备占用；先检查连接状态再重试。':'请求超时，请检查服务与连接状态；命令未自动重发。'):e;if(body!==undefined)record(path,false,String(error));throw error;}
}
export function download(name:string,data:unknown){const url=URL.createObjectURL(new Blob([JSON.stringify(data,null,2)],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
