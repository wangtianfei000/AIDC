#!/usr/bin/env python3
from __future__ import annotations

import csv, hashlib, json, math, shutil, socket, subprocess, sys, time, urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

HERE=Path(__file__).resolve();MODEL_BROWSER=HERE.parents[1];AIDC_ROOT=MODEL_BROWSER.parent;REPO_ROOT=AIDC_ROOT.parent
PYTHON=REPO_ROOT/"tools"/"openmodelica"/"fmpy-venv"/"Scripts"/"python.exe";SERVER=MODEL_BROWSER/"services"/"project-server"/"server.py";WEB_ROOT=MODEL_BROWSER/"apps"/"web"/"dist";RESULTS=MODEL_BROWSER/"results"
sys.path.insert(0,str(MODEL_BROWSER/"tools"));sys.path.insert(0,str(MODEL_BROWSER/"services"/"project-server"))
from project_tool import ContractError, read_json, run_project, source_fingerprint, validate_project, write_json
from server import ProjectService

def require(value:bool,message:str)->None:
    if not value:raise AssertionError(message)
def free_port()->int:
    with socket.socket() as sock:sock.bind(("127.0.0.1",0));return sock.getsockname()[1]
def copy_minimal(source:Path,destination:Path)->None:
    destination.mkdir(parents=True);shutil.copy2(source/"model-project.json",destination/"model-project.json");shutil.copytree(source/"modelica",destination/"modelica")
def start_server(project:Path,scratch:Path,label:str)->tuple[subprocess.Popen[str],str]:
    port=free_port();ready=scratch/f"{label}-{port}.json";log=scratch/f"{label}-{port}.log";handle=log.open("w",encoding="utf-8")
    process=subprocess.Popen([str(PYTHON),str(SERVER),"--project",str(project),"--port",str(port),"--write-ready",str(ready),"--web-root",str(WEB_ROOT)],cwd=AIDC_ROOT,stdout=handle,stderr=subprocess.STDOUT,text=True)
    process._aidc_log_handle=handle # type: ignore[attr-defined]
    deadline=time.monotonic()+120
    while time.monotonic()<deadline:
        if ready.is_file():return process,f"http://127.0.0.1:{port}"
        if process.poll() is not None:handle.close();raise RuntimeError(log.read_text(encoding="utf-8",errors="replace"))
        time.sleep(.2)
    stop_server(process);raise TimeoutError(f"Project Server 启动超时: {project}")
def stop_server(process:subprocess.Popen[str])->None:
    process.terminate()
    try:process.wait(timeout=10)
    except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
    handle=getattr(process,"_aidc_log_handle",None)
    if handle:handle.close()
def get_json(base:str,path:str)->dict[str,Any]:
    with urllib.request.urlopen(base+path,timeout=60) as response:return json.loads(response.read().decode("utf-8"))
def digest_file(path:Path)->str:
    value=hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda:handle.read(1024*1024),b""):value.update(chunk)
    return value.hexdigest()
def compare_series(left:Path,right:Path)->dict[str,Any]:
    max_abs=0.0;max_rel=0.0;points=0
    with left.open("r",encoding="utf-8-sig",newline="") as a,right.open("r",encoding="utf-8-sig",newline="") as b:
        ra,rb=csv.reader(a),csv.reader(b);ha,hb=next(ra),next(rb);require(ha==hb,"复跑结果列不一致")
        for row_a,row_b in zip(ra,rb,strict=True):
            points+=1
            for one,two in zip(row_a,row_b,strict=True):
                x,y=float(one),float(two);delta=abs(x-y);max_abs=max(max_abs,delta);max_rel=max(max_rel,delta/max(1e-12,abs(x),abs(y)))
    require(max_abs<=1e-10 or max_rel<=1e-10,f"复跑数值漂移: abs={max_abs}, rel={max_rel}")
    return {"pointCount":points,"variableCount":len(ha)-1,"maxAbsoluteDifference":max_abs,"maxRelativeDifference":max_rel,"columns":ha}
def synthetic_downsampling(project:Path)->dict[str,Any]:
    run_id="t1-4-100k-20-curves";run_dir=project/"results"/run_id;run_dir.mkdir(parents=True,exist_ok=True);variables=[f"signal{i:02d}" for i in range(1,21)]
    with (run_dir/"series.csv").open("w",encoding="utf-8",newline="") as handle:
        writer=csv.writer(handle);writer.writerow(["time",*variables])
        for index in range(100_000):
            t=index/1000;writer.writerow([f"{t:.3f}",*[f"{math.sin(t*(i+1)*.01):.12g}" for i in range(20)]])
    service=ProjectService(project);started=time.perf_counter();series=service.get_series(run_id,variables,None,None,800);elapsed=time.perf_counter()-started
    require(series["pointCount"]==800 and len(series["columns"])==21,"20×10万点降采样输出不符合要求");require(elapsed<5.0,f"降采样耗时过长: {elapsed:.3f}s")
    return {"inputPoints":100000,"curveCount":20,"outputPoints":series["pointCount"],"elapsedSeconds":elapsed}
def inject_schema_fault(source:Path,scratch:Path)->dict[str,Any]:
    project=scratch/"schema-fault";copy_minimal(source,project);data=read_json(project/"model-project.json");data["schemaVersion"]="9.9";write_json(project/"model-project.json",data)
    try:validate_project(project)
    except ContractError as exc:
        message=str(exc);require("model-project.json" in message and "schemaVersion" in message,"Schema 故障未定位文件和 JSON 路径");return {"detected":True,"message":message}
    raise AssertionError("Schema 故障未被拒绝")
def inject_compile_fault(source:Path,scratch:Path)->dict[str,Any]:
    project=scratch/"compile-fault";copy_minimal(source,project);model_file=next((project/"modelica").rglob("package.mo"));text=model_file.read_text(encoding="utf-8-sig");needle="connect(reference.y, controller.u_s)";require(needle in text,"找不到编译故障注入点");model_file.write_text(text.replace(needle,"connect(reference.y, controller.missingInput)",1),encoding="utf-8")
    try:run_project(project,simulation_settings={"startTime":0,"stopTime":.02,"outputInterval":.01,"relativeTolerance":1e-7,"solver":"dassl","solverFlags":{}})
    except ContractError:pass
    else:raise AssertionError("Modelica 编译故障未被拒绝")
    run_dir=max((project/"results").iterdir(),key=lambda p:p.stat().st_mtime);diagnostics=read_json(run_dir/"diagnostics.json");item=diagnostics["diagnostics"][0]
    require(item["stage"]=="compiling" and item["exitCode"]==1,"编译诊断缺少阶段或退出码");require(item.get("source",{}).get("line"),"编译诊断缺少源码位置");require(item.get("rawMessage") and item.get("logRef")=="logs/compile.log","编译诊断缺少原始日志")
    return {"detected":True,"stage":item["stage"],"source":item["source"],"logRef":item["logRef"]}
def promote_baseline(source_project:Path,run_dir:Path,repro:dict[str,Any])->dict[str,Any]:
    manifest=read_json(run_dir/"run-manifest.json");destination=source_project/"results"/run_dir.name
    if destination.exists():shutil.rmtree(destination)
    shutil.copytree(run_dir,destination)
    baseline={"schemaVersion":"1.0","baselineId":f"{manifest['projectId']}.t1-4","projectId":manifest["projectId"],"modelClass":manifest["modelClass"],"runId":manifest["runId"],"sourceDigest":manifest["sourceDigest"],"toolchain":manifest["toolchain"],"solverSettings":manifest["solverSettings"],"seriesSha256":digest_file(destination/"series.csv"),"reproducibility":{k:repro[k] for k in ("maxAbsoluteDifference","maxRelativeDifference","pointCount","variableCount")},"promotedAt":datetime.now(timezone.utc).isoformat().replace("+00:00","Z")}
    write_json(source_project/"results"/"baseline.json",baseline);return baseline

def main()->int:
    started=time.perf_counter();stamp=datetime.now().strftime("%Y%m%d-%H%M%S");scratch=AIDC_ROOT/".codex-runtime"/f"t1-4-{stamp}";scratch.mkdir(parents=True)
    sources=[AIDC_ROOT/"samples"/"modelica-t0-single-joint",AIDC_ROOT/"samples"/"modelica-t1-dc-motor"]
    bundle="\n".join(path.read_text(encoding="utf-8",errors="ignore") for path in (WEB_ROOT/"assets").glob("*.js"));project_results=[]
    for index,source in enumerate(sources):
        loaded=validate_project(source);project=loaded["project"];require(project["name"] not in bundle and project["modelica"]["defaultExperiment"] not in bundle,"浏览器构建包含工程专有常量")
        copy=scratch/"renamed-projects"/f"arbitrary-location-{index+1}";copy_minimal(source,copy);require(set(p.name for p in copy.iterdir())=={"model-project.json","modelica"},"干净副本包含结果或私有状态")
        run_one=run_project(copy);run_two=run_project(copy);manifest_one=read_json(run_one/"run-manifest.json");manifest_two=read_json(run_two/"run-manifest.json");repro=compare_series(run_one/"series.csv",run_two/"series.csv")
        require(manifest_one["sourceDigest"]==manifest_two["sourceDigest"] and manifest_one["toolchain"]==manifest_two["toolchain"] and manifest_one["solverSettings"]==manifest_two["solverSettings"],"复跑清单的事实源、工具链或求解配置不一致")
        process,base=start_server(copy,scratch,f"switch-{index}")
        try:
            summary=get_json(base,"/api/v1/project");configuration=get_json(base,"/api/v1/model/configuration");html=urllib.request.urlopen(base+"/",timeout=30).read().decode("utf-8");require(summary["projectId"]==project["projectId"] and configuration["modelClass"]==project["modelica"]["defaultExperiment"] and "Modelica 模型浏览器" in html,"同一浏览器构建未能切换工程")
        finally:stop_server(process)
        baseline=promote_baseline(source,run_one,repro);project_results.append({"projectId":project["projectId"],"copiedPath":str(copy),"runIds":[run_one.name,run_two.name],"reproducibility":repro,"baseline":baseline})
    # 服务重启后，运行记录仍由工程恢复。
    restart_project=scratch/"renamed-projects"/"arbitrary-location-1";before=len(ProjectService(restart_project).list_runs());process,base=start_server(restart_project,scratch,"restart")
    try:after=len(get_json(base,"/api/v1/runs")["items"])
    finally:stop_server(process)
    require(before==after and before>=2,"服务重启后运行状态未从工程恢复")
    schema_fault=inject_schema_fault(sources[0],scratch);compile_fault=inject_compile_fault(sources[0],scratch);downsampling=synthetic_downsampling(restart_project)
    workbench=(MODEL_BROWSER/"apps"/"web"/"src"/"Workbench.tsx").read_text(encoding="utf-8");require("localStorage.setItem(\"modelica.leftWidth\"" in workbench and "localStorage.setItem(\"modelica.rightWidth\"" in workbench,"界面宽度偏好未明确保存");require(all(token not in workbench for token in ("localStorage.setItem(\"parameter","localStorage.setItem(\"scenario","localStorage.setItem(\"result","localStorage.setItem(\"conclusion")),"浏览器私有存储包含业务状态")
    duration=time.perf_counter()-started;summary={"passed":True,"completedAt":datetime.now(timezone.utc).isoformat().replace("+00:00","Z"),"durationSeconds":duration,"projects":project_results,"schemaFault":schema_fault,"compileFault":compile_fault,"downsampling":downsampling,"storage":{"allowedKeys":["modelica.leftWidth","modelica.rightWidth"],"businessStateKeys":[]},"serviceRestart":{"runsBefore":before,"runsAfter":after},"webBuild":str(WEB_ROOT),"screenEvidence":"results/T1.4-通用性与复现验收屏幕证据.jpg"}
    RESULTS.mkdir(parents=True,exist_ok=True);write_json(RESULTS/"t1-4-summary.json",summary)
    report=["# T1.4：通用性与复现验收报告","","**结论：通过**","",f"验收耗时：{duration:.2f} s。相同生产构建在两个不同工程及改名副本上运行，无需修改或重建前端。","","## 通用性","","| 工程 | 干净重建 | 双次复跑 | 最大绝对差 | 最大相对差 | Baseline |","|---|---|---|---:|---:|---|"]
    for item in project_results:report.append(f"| `{item['projectId']}` | 通过 | 通过 | {item['reproducibility']['maxAbsoluteDifference']:.3g} | {item['reproducibility']['maxRelativeDifference']:.3g} | `{item['baseline']['runId']}` |")
    report += ["","## 故障与存储","",f"- Schema 故障：已定位 `model-project.json#/schemaVersion`。",f"- Modelica 故障：已定位 `{compile_fault['source']['path']}:{compile_fault['source']['line']}`，阶段 `compiling`，日志 `{compile_fault['logRef']}`。",f"- 大结果：20 条曲线 × 100,000 点降采样为 {downsampling['outputPoints']} 点，耗时 {downsampling['elapsedSeconds']:.3f} s。","- 浏览器存储仅包含左右面板宽度；模型参数、仿真设置、结果和结论均未写入私有存储。",f"- 服务重启前后运行数：{before}/{after}，状态由工程 `results/` 恢复。","","## 受控结果","","两个工程的 `results/baseline.json` 均记录源摘要、OMC/MSL 版本、求解设置、结果 SHA-256 和数值复现误差。","","## 证据","","- `t1-4-summary.json`：机器可读验收数据。","- `T1.4-通用性与复现验收屏幕证据.jpg`：同一生产浏览器打开两个工程的屏幕证据。",""]
    (RESULTS/"T1.4-通用性与复现验收报告.md").write_text("\n".join(report),encoding="utf-8");shutil.rmtree(scratch);print(json.dumps({"passed":True,"report":str(RESULTS/"T1.4-通用性与复现验收报告.md")},ensure_ascii=False));return 0
if __name__=="__main__":raise SystemExit(main())
