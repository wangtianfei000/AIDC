import type { ModelConfiguration, ModelTree, ProjectSummary, RunDetail, RunListItem, Series } from "./types";

async function request<T>(path:string, init?:RequestInit):Promise<T>{
  const response=await fetch(path,init);
  const data=await response.json().catch(()=>({}));
  if(!response.ok) throw new Error(data?.error?.message || `${response.status} ${response.statusText}`);
  return data as T;
}
export const api={
  projects:()=>request<{items:ProjectSummary[]}>("/api/v1/projects"),
  selectProjectDirectory:()=>request<{path:string}>("/api/v1/projects/select-directory",{method:"POST"}),
  openProject:(path:string)=>request<ProjectSummary>("/api/v1/projects/open",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({path})}),
  unloadProject:(projectId:string)=>request<{projectId:string;status:string}>(`/api/v1/projects/${encodeURIComponent(projectId)}`,{method:"DELETE"}),
  selectConversionDirectory:(kind:"source-file"|"source-directory"|"output")=>request<{path:string}>("/api/v1/model-conversion/select-directory",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({kind})}),
  convertModel:(input:{source:string;output:string;package?:string;profile:"auto"|"p2"|"p3";migrate:boolean;force:boolean;stlUnit:"mm"|"m";density:number;rootType:"fixed"|"floating"})=>request<{project:ProjectSummary;generation:{cacheStatus?:string;profile?:string;generationDigest?:string;output?:string;stlImport?:{mass_kg:number;volume_m3:number;triangleCount:number;watertight:boolean;warnings:string[]}}}>("/api/v1/model-conversion",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(input)}),
  project:(projectId:string)=>request<ProjectSummary>(scope(projectId,"/project")),
  model:(projectId:string)=>request<ModelTree>(scope(projectId,"/model/tree")),
  configuration:(projectId:string)=>request<ModelConfiguration>(scope(projectId,"/model/configuration")),
  runs:(projectId:string)=>request<{items:RunListItem[]}>(scope(projectId,"/runs")),
  run:(projectId:string,id:string)=>request<RunDetail>(scope(projectId,`/runs/${encodeURIComponent(id)}`)),
  series:(projectId:string,id:string,variables:string[],start?:number,end?:number)=>{const p=new URLSearchParams({maxPoints:"800"});variables.forEach(variable=>p.append("variables",variable));if(start!==undefined)p.set("start",String(start));if(end!==undefined)p.set("end",String(end));return request<Series>(`${scope(projectId,`/runs/${encodeURIComponent(id)}/series`)}?${p}`)},
  values:(projectId:string,id:string,variables:string[],time?:number)=>{const p=new URLSearchParams();variables.forEach(variable=>p.append("variables",variable));if(time!==undefined)p.set("time",String(time));return request<{runId:string;requestedTime:number|null;time:number;values:Record<string,number>}>(`${scope(projectId,`/runs/${encodeURIComponent(id)}/values`)}?${p}`)},
  source:(projectId:string,path:string,line?:number,endLine?:number)=>request<{path:string;startLine:number;endLine:number;totalLines:number;text:string}>(`${scope(projectId,"/source")}?${new URLSearchParams({path,...(line?{line:String(line)}:{}),...(endLine?{endLine:String(endLine)}:{})})}`),
  saveSimulationSettings:(projectId:string,simulationSettings:{startTime:number;stopTime:number;outputInterval:number;relativeTolerance:number;solver:string;solverFlags:Record<string,string|number|boolean>})=>request<{settings:typeof simulationSettings;source:string;sourceDigest:string}>(scope(projectId,"/model/simulation-settings"),{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(simulationSettings)}),
  startRun:(projectId:string,simulationSettings?:{startTime:number;stopTime:number;outputInterval:number;relativeTolerance:number;solver:string;solverFlags?:Record<string,string|number|boolean>})=>request<{taskId:string}>(scope(projectId,"/runs"),{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({simulationSettings})}),
};

function scope(projectId:string,path:string){return `/api/v1/projects/${encodeURIComponent(projectId)}${path}`}
