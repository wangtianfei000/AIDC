#!/usr/bin/env python3
"""Validate and run a minimal Modelica project."""
from __future__ import annotations

import argparse, csv, hashlib, json, os, platform, re, shutil, subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable
from jsonschema import Draft202012Validator, FormatChecker

SCRIPT=Path(__file__).resolve(); MODEL_BROWSER=Path(os.environ.get("AIDC_MODELICA_ROOT", SCRIPT.parents[1])); AIDC_ROOT=MODEL_BROWSER.parent; REPO_ROOT=AIDC_ROOT.parent
SCHEMA_ROOT=MODEL_BROWSER/"packages"/"contracts"/"schemas"; OMC=Path(os.environ.get("AIDC_OMC",REPO_ROOT/"tools"/"openmodelica"/"1.27.1"/"bin"/"omc.exe"))

class ContractError(RuntimeError): pass
def read_json(path:Path)->dict[str,Any]:
    try:return json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception as exc:raise ContractError(f"无法读取 JSON {path}: {exc}") from exc
def write_json(path:Path,value:Any)->None:path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
def sha256_file(path:Path)->str:
    digest=hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda:handle.read(1024*1024),b""):digest.update(chunk)
    return digest.hexdigest()
def normalized_text_digest(path:Path)->str:
    text=path.read_text(encoding="utf-8-sig").replace("\r\n","\n").replace("\r","\n")
    return hashlib.sha256(text.encode()).hexdigest()
def resolve_inside(root:Path,relative:str)->Path:
    if "\\" in relative:raise ContractError(f"工程路径必须使用 '/': {relative}")
    candidate=(root/relative).resolve()
    try:candidate.relative_to(root.resolve())
    except ValueError as exc:raise ContractError(f"工程路径越界: {relative}") from exc
    return candidate
def load_schema(name:str)->dict[str,Any]:return read_json(SCHEMA_ROOT/name)
def validate_document(data:Any,schema_name:str,path:Path)->None:
    errors=sorted(Draft202012Validator(load_schema(schema_name),format_checker=FormatChecker()).iter_errors(data),key=lambda x:list(x.absolute_path))
    if errors:raise ContractError("\n".join(f"{path}#/{'/'.join(map(str,e.absolute_path))}: {e.message}" for e in errors))
def validate_project(project_root:Path)->dict[str,Any]:
    root=project_root.resolve(); entry=root/"model-project.json"
    if not entry.is_file():raise ContractError(f"模型工程缺少入口: {entry}")
    project=read_json(entry);validate_document(project,"model-project.schema.json",entry)
    model_file=resolve_inside(root,project["modelica"]["rootPackageFile"])
    if not model_file.is_file():raise ContractError(f"Modelica 根包不存在: {model_file}")
    if model_file.parts[len(root.parts)]!="modelica":raise ContractError("Modelica 源文件必须位于 modelica/ 内")
    library_files=[]
    for library in project["modelica"]["libraries"]:
        if "path" not in library:continue
        candidate=(root/library["path"]).resolve()
        in_project = in_workspace = False
        try:candidate.relative_to(root);in_project = True
        except ValueError:pass
        try:candidate.relative_to(AIDC_ROOT.resolve());in_workspace = True
        except ValueError:pass
        if not (in_project or in_workspace):
            raise ContractError(f"Modelica 依赖路径必须位于工程或 AIDC 工作区内: {library['path']}")
        if not candidate.is_file():raise ContractError(f"Modelica 依赖包不存在: {candidate}")
        library_files.append({"name":library["name"],"version":library["version"],"path":candidate})
    return {"root":root,"entry":entry,"project":project,"modelFile":model_file,"libraryFiles":library_files}
def source_fingerprint(project_root:Path)->tuple[str,list[dict[str,Any]]]:
    root=project_root.resolve();files=[root/"model-project.json",*[p for p in (root/"modelica").rglob("*") if p.is_file()]];records=[];aggregate=hashlib.sha256()
    for path in sorted(files,key=lambda p:p.relative_to(root).as_posix()):
        relative=path.relative_to(root).as_posix()
        try:digest=normalized_text_digest(path)
        except UnicodeDecodeError:digest=sha256_file(path)
        records.append({"path":relative,"sha256":digest,"bytes":path.stat().st_size});aggregate.update(f"{relative}\0{digest}\n".encode())
    return f"sha256:{aggregate.hexdigest()}",records
def artifact_record(path:Path,run_dir:Path,role:str)->dict[str,Any]:return {"path":path.relative_to(run_dir).as_posix(),"sha256":sha256_file(path),"bytes":path.stat().st_size,"role":role}

DEFAULT_SIMULATION={"startTime":0.0,"stopTime":1.0,"outputInterval":0.001,"relativeTolerance":1e-6,"solver":"dassl","solverFlags":{}}
def model_simulation_settings(path:Path)->dict[str,Any]:
    settings={**DEFAULT_SIMULATION,"solverFlags":{}};text=path.read_text(encoding="utf-8-sig");experiment=re.search(r"\bexperiment\s*\(([^)]*)\)",text,re.DOTALL)
    if experiment:
        for source,target in {"StartTime":"startTime","StopTime":"stopTime","Interval":"outputInterval","Tolerance":"relativeTolerance"}.items():
            match=re.search(rf"\b{source}\s*=\s*([-+0-9.eE]+)",experiment.group(1))
            if match:settings[target]=float(match.group(1))
    flags=re.search(r'__OpenModelica_simulationFlags\s*\(((?:[^"()]|"(?:\\.|[^"])*")*)\)',text,re.DOTALL)
    if flags:
        values=dict(re.findall(r'(\w+)\s*=\s*"([^"]*)"',flags.group(1)));settings["solver"]=values.pop("s",settings["solver"]);settings["solverFlags"]=values
    return settings

def run_project(project_root:Path,on_stage:Callable[[str,dict[str,Any]],None]|None=None,simulation_settings:dict[str,Any]|None=None,model_class:str|None=None)->Path:
    if on_stage:on_stage("validating",{})
    loaded=validate_project(project_root);project=loaded["project"];model_class=model_class or project["modelica"]["defaultExperiment"];source_digest,source_files=source_fingerprint(project_root.resolve())
    timestamp=datetime.now(timezone.utc);run_id=f"{timestamp.strftime('%Y%m%d-%H%M%S')}-{model_class.rsplit('.',1)[-1]}-{source_digest[-8:]}";run_dir=resolve_inside(project_root.resolve(),f"results/{run_id}")
    build_dir=run_dir/"build";logs_dir=run_dir/"logs";build_dir.mkdir(parents=True,exist_ok=False);logs_dir.mkdir()
    manifest_path=run_dir/"run-manifest.json";diagnostics_path=run_dir/"diagnostics.json";summary_path=run_dir/"summary.json";started=timestamp.isoformat().replace("+00:00","Z")
    if not OMC.is_file():raise ContractError(f"OpenModelica 编译器不存在: {OMC}")
    omc_version=subprocess.run([str(OMC),"--version"],capture_output=True,text=True,encoding="utf-8",errors="replace",check=True).stdout.strip();sim={**model_simulation_settings(loaded["modelFile"]),**(simulation_settings or {})};solver_flags=sim.get("solverFlags") or {};intervals=round((sim["stopTime"]-sim["startTime"])/sim["outputInterval"])
    if intervals<=0:raise ContractError("仿真区间无效")
    libraries=[]
    for dependency in project["modelica"]["libraries"]:
        if "path" in dependency:
            resolved=next(item["path"] for item in loaded["libraryFiles"] if item["name"]==dependency["name"])
            libraries.append(f'loadFile("{resolved.as_posix()}");')
        else:libraries.append(f'loadModel({dependency["name"]}, {{"{dependency["version"]}"}});')
    mos="\n".join([f'setCommandLineOptions("--std={project["modelica"]["languageVersion"]}");',*libraries,f'loadFile("{loaded["modelFile"].resolve().as_posix()}");',f'cd("{build_dir.resolve().as_posix()}");',f'checkModel({model_class});','getErrorString();',f'buildModel({model_class}, startTime={sim["startTime"]}, stopTime={sim["stopTime"]}, numberOfIntervals={intervals}, tolerance={sim["relativeTolerance"]}, method="{sim["solver"]}", outputFormat="csv", fileNamePrefix="ModelRun");','getErrorString();',''])
    mos_path=run_dir/"run.mos";mos_path.write_text(mos,encoding="utf-8")
    manifest={"schemaVersion":"2.0","runId":run_id,"projectId":project["projectId"],"modelClass":model_class,"status":"compiling","stage":"compiling","startedAt":started,"finishedAt":None,"exitCode":None,"sourceDigest":source_digest,"sourceFiles":source_files,"toolchain":{"openModelica":omc_version,"modelicaStandardLibrary":project["modelica"]["libraries"][0]["version"],"runner":"aidc-project-tool/2.0"},"platform":{"system":platform.system(),"release":platform.release(),"machine":platform.machine(),"python":platform.python_version()},"solverSettings":{**sim,"numberOfIntervals":intervals,"solverFlags":solver_flags},"artifacts":[],"logs":{"compile":"logs/compile.log","solve":"logs/solve.log"}}
    write_json(manifest_path,manifest);diagnostics={"schemaVersion":"1.0","diagnostics":[]}
    def stage(name:str)->None:
        manifest["stage"]=name;manifest["status"]=name;write_json(manifest_path,manifest)
        if on_stage:on_stage(name,{})
    try:
        # OpenModelica on Windows expects UTF-8 command-line bytes and rejects a
        # full .mos path containing CJK characters.  The process API accepts a
        # Unicode cwd, so pass only the ASCII basename while keeping the script
        # and all outputs in the user's original project directory.
        stage("compiling");compiled=subprocess.run([str(OMC),mos_path.name],cwd=run_dir,capture_output=True,text=True,encoding="utf-8",errors="replace");compile_log=logs_dir/"compile.log";compile_log.write_text(compiled.stdout+("\n"+compiled.stderr if compiled.stderr else ""),encoding="utf-8");executable=build_dir/"ModelRun.exe"
        if compiled.returncode or not executable.is_file():raise ContractError(f"OpenModelica 检查或编译失败，退出码 {compiled.returncode}")
        stage("initializing");env=os.environ.copy();env["OPENMODELICAHOME"]=str(OMC.parents[1])
        runtime_roots=[project_root.resolve()/"modelica",*[item["path"].parent for item in loaded["libraryFiles"]]]
        runtime_dirs=[]
        for runtime_root in runtime_roots:
            runtime_dirs.extend(path for path in runtime_root.rglob("win64") if path.is_dir() and path.parent.name=="Library")
        env["PATH"]=os.pathsep.join([str(OMC.parent),*(str(path) for path in runtime_dirs),env.get("PATH","")]);command=[str(executable),f'-r={(build_dir/"ModelRun_res.csv").resolve()}']
        for name,value in solver_flags.items():
            if value is True:command.append(f"-{name}")
            elif value not in (None,"",False):command.append(f"-{name}={value}")
        stage("solving");solved=subprocess.run(command,cwd=build_dir,capture_output=True,text=True,encoding="utf-8",errors="replace",env=env);solve_log=logs_dir/"solve.log";solve_log.write_text(solved.stdout+("\n"+solved.stderr if solved.stderr else ""),encoding="utf-8");result_csv=build_dir/"ModelRun_res.csv"
        if solved.returncode or not result_csv.is_file():raise ContractError(f"OpenModelica 求解失败，退出码 {solved.returncode}")
        stage("evaluating");series_path=run_dir/"series.csv";shutil.copy2(result_csv,series_path)
        with series_path.open("r",encoding="utf-8-sig",newline="") as handle:
            reader=csv.reader(handle);header=next(reader,[]);last=None
            for last in reader:pass
        summary={"schemaVersion":"2.0","runId":run_id,"status":"completed","finalTime":float(last[0]) if last else None,"variableCount":max(0,len(header)-1)};write_json(summary_path,summary);write_json(diagnostics_path,diagnostics)
        manifest.update(status="completed",stage="completed",finishedAt=datetime.now(timezone.utc).isoformat().replace("+00:00","Z"),exitCode=0);manifest["artifacts"]=[artifact_record(series_path,run_dir,"timeseries"),artifact_record(summary_path,run_dir,"summary"),artifact_record(diagnostics_path,run_dir,"diagnostics"),artifact_record(compile_log,run_dir,"compiler-log"),artifact_record(solve_log,run_dir,"solver-log")];write_json(manifest_path,manifest)
        validate_document(manifest,"run-manifest.schema.json",manifest_path);validate_document(summary,"run-summary.schema.json",summary_path);return run_dir
    except Exception as exc:
        failed_stage=manifest["stage"];log_name="compile.log" if failed_stage=="compiling" else "solve.log";log_path=logs_dir/log_name;raw=log_path.read_text(encoding="utf-8",errors="replace") if log_path.is_file() else ""
        diagnostic={"stage":failed_stage,"severity":"error","code":"AIDC-RUN-FAILED","title":"模型运行失败","message":str(exc),"rawMessage":raw[-4000:],"exitCode":1,"logRef":f"logs/{log_name}"}
        location=re.search(r"\[(.+?):(\d+):(\d+)-\d+:\d+:[^\]]+\]",raw)
        if location:
            source_path=Path(location.group(1)).resolve()
            try:relative_source=source_path.relative_to(project_root.resolve()).as_posix()
            except ValueError:relative_source=source_path.as_posix()
            diagnostic["source"]={"path":relative_source,"line":int(location.group(2)),"column":int(location.group(3))}
        diagnostics["diagnostics"].append(diagnostic);write_json(diagnostics_path,diagnostics);manifest.update(status="failed",stage="failed",finishedAt=datetime.now(timezone.utc).isoformat().replace("+00:00","Z"),exitCode=1);write_json(manifest_path,manifest);raise

def main()->int:
    parser=argparse.ArgumentParser();sub=parser.add_subparsers(dest="command",required=True);v=sub.add_parser("validate");v.add_argument("project",type=Path);r=sub.add_parser("run");r.add_argument("project",type=Path);args=parser.parse_args()
    try:
        loaded=validate_project(args.project)
        if args.command=="validate":
            digest,files=source_fingerprint(args.project);print(json.dumps({"valid":True,"projectId":loaded["project"]["projectId"],"sourceDigest":digest,"sourceFileCount":len(files)},ensure_ascii=False));return 0
        print(json.dumps({"passed":True,"runDir":str(run_project(args.project))},ensure_ascii=False));return 0
    except Exception as exc:print(json.dumps({"passed":False,"error":str(exc)},ensure_ascii=False),file=os.sys.stderr);return 1
if __name__=="__main__":raise SystemExit(main())
