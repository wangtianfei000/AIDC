export type RunStatus = "queued" | "validating" | "compiling" | "initializing" | "solving" | "evaluating" | "completed" | "failed" | "cancelled";
export interface ModelProject {
  schemaVersion: "2.0"; projectId: string; name: string; revision: string; description: string;
  modelica: { rootPackageFile: string; rootPackage: string; defaultExperiment: string; languageVersion: string; libraries: Array<{name:string;version:string;path?:string}> };
}
export interface SimulationSettings { startTime:number; stopTime:number; outputInterval:number; relativeTolerance:number; solver:string; solverFlags:Record<string,string|number|boolean> }
export interface RunManifest {
  schemaVersion:"2.0"; runId:string; projectId:string; modelClass:string; status:RunStatus; stage:string; startedAt:string; finishedAt:string|null; exitCode:number|null;
  sourceDigest:string; sourceFiles:Array<{path:string;sha256:string;bytes:number}>; toolchain:Record<string,string>; platform:Record<string,string>; solverSettings:SimulationSettings&{numberOfIntervals:number}; artifacts:Array<{path:string;sha256:string;bytes:number;role:string}>; logs:Record<string,string>;
}
export interface RunSummary { schemaVersion:"2.0"; runId:string; status:"completed"|"failed"|"cancelled"; finalTime:number|null; variableCount:number }
export interface Diagnostic { stage:string; severity:"info"|"warning"|"error"; code:string; title:string; message:string; rawMessage?:string; source?:{path:string;line:number;column?:number}; exitCode?:number; logRef?:string }
