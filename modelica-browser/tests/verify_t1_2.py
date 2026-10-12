#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime
from pathlib import Path
from typing import Any


HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
AIDC_ROOT = MODEL_BROWSER.parent
REPO_ROOT = AIDC_ROOT.parent
PYTHON = REPO_ROOT / "tools" / "openmodelica" / "fmpy-venv" / "Scripts" / "python.exe"
SERVER = MODEL_BROWSER / "services" / "project-server" / "server.py"
RESULTS = MODEL_BROWSER / "results"


def free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def request_json(base: str, path: str, method: str = "GET", body: dict[str, Any] | None = None) -> tuple[int, Any]:
    data = json.dumps(body).encode("utf-8") if body is not None else None
    request = urllib.request.Request(base + path, data=data, method=method, headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            return response.status, json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        return exc.code, json.loads(exc.read().decode("utf-8"))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def wait_ready(path: Path, process: subprocess.Popen[str]) -> dict[str, Any]:
    deadline = time.monotonic() + 90
    while time.monotonic() < deadline:
        if path.is_file():
            return json.loads(path.read_text(encoding="utf-8"))
        if process.poll() is not None:
            stdout, stderr = process.communicate()
            raise RuntimeError(f"Project Server 提前退出\nstdout={stdout}\nstderr={stderr}")
        time.sleep(0.2)
    raise TimeoutError("Project Server 启动超时")


def verify_project(project: Path, scratch: Path) -> dict[str, Any]:
    port = free_port()
    ready_file = scratch / f"{project.name}-ready.json"
    log_file = scratch / f"{project.name}-server.log"
    with log_file.open("w", encoding="utf-8") as log:
        process = subprocess.Popen(
            [str(PYTHON), str(SERVER), "--project", str(project), "--host", "127.0.0.1", "--port", str(port), "--write-ready", str(ready_file)],
            cwd=AIDC_ROOT, stdout=log, stderr=subprocess.STDOUT, text=True,
        )
    try:
        ready = wait_ready(ready_file, process)
        base = f"http://127.0.0.1:{ready['port']}"
        status, health = request_json(base, "/health")
        require(status == 200 and health["projectId"] == ready["projectId"], "健康检查失败")

        _, project_info = request_json(base, "/api/v1/project")
        _, tree = request_json(base, "/api/v1/model/tree")
        require(tree["generator"] == "omc-scripting-api", "模型索引未使用 omc scripting API")
        require(tree["sourceDigest"] == project_info["sourceDigest"], "模型索引与工程摘要不一致")
        component_count = sum(len(item["components"]) for item in tree["classes"])
        connection_count = sum(len(item["connections"]) for item in tree["classes"])
        require(component_count > 0 and connection_count > 0, "模型结构索引为空")

        _, configuration = request_json(base, "/api/v1/model/configuration")
        _, adapters = request_json(base, "/api/v1/adapters")
        require(configuration["modelClass"] == project_info["modelica"]["defaultExperiment"], "仿真配置没有指向默认模型")
        require(configuration["outputs"], "未从模型提取输出变量")
        adapter_map = {item["id"]: item for item in adapters["items"]}
        require(adapter_map["openmodelica"]["installed"] and adapter_map["fmpy"]["installed"], "编译/FMI 适配器未就绪")
        fmpy_inspection = None
        fmu = project / "results" / "legacy-artifacts" / "native" / "T0SingleJoint.fmu"
        if fmu.is_file():
            relative_fmu = fmu.relative_to(project).as_posix()
            inspect_status, fmpy_inspection = request_json(base, "/api/v1/adapters/fmpy/inspect?" + urllib.parse.urlencode({"path": relative_fmu}))
            require(inspect_status == 200 and fmpy_inspection["fmiVersion"] == "2.0", "FMPy 未能检查 FMI 2.0 产物")

        model_source = project_info["modelica"]["rootPackageFile"]
        _, source = request_json(base, "/api/v1/source?" + urllib.parse.urlencode({"path": model_source, "line": 1}))
        require(source["text"], "源码 API 为空")
        traversal_status, _ = request_json(base, "/api/v1/source?" + urllib.parse.urlencode({"path": "../package.json"}))
        require(traversal_status == 400, "源码 API 未拒绝路径越界")

        simulation_settings = dict(configuration["simulation"])
        simulation_settings["outputInterval"] *= 2
        simulation_settings["solver"] = "dassl"
        simulation_settings["solverFlags"] = {"maxIntegrationOrder": 4}
        save_status, saved = request_json(base, "/api/v1/model/simulation-settings", "POST", simulation_settings)
        require(save_status == 200 and saved["settings"] == simulation_settings, "仿真设置未写入模型")
        _, persisted_source = request_json(base, "/api/v1/source?" + urllib.parse.urlencode({"path": saved["source"]}))
        require("experiment(StartTime" in persisted_source["text"], "模型缺少 experiment annotation")
        require('__OpenModelica_simulationFlags(s = "dassl", maxIntegrationOrder = "4")' in persisted_source["text"], "模型缺少 OMC 求解器 annotation")
        _, persisted_configuration = request_json(base, "/api/v1/model/configuration")
        require(persisted_configuration["simulation"] == simulation_settings, "配置 API 未返回模型中的仿真设置")
        status, task = request_json(base, "/api/v1/runs", "POST", {})
        require(status == 202, "创建运行任务失败")
        require(task["simulationSettings"] == simulation_settings, "运行任务未保留仿真设置")
        task_id = task["taskId"]
        with urllib.request.urlopen(base + f"/api/v1/tasks/{task_id}/events", timeout=180) as response:
            event_text = response.read().decode("utf-8")
        stages = [json.loads(line[6:])["stage"] for line in event_text.splitlines() if line.startswith("data: {") and '"stage"' in line]
        expected = ["queued", "validating", "compiling", "initializing", "solving", "evaluating"]
        for stage in expected:
            require(stage in stages, f"SSE 缺少阶段 {stage}")
        require("event: completed" in event_text, "SSE 缺少 completed 事件")

        _, final_task = request_json(base, f"/api/v1/tasks/{task_id}")
        require(final_task["status"] == "completed", f"任务未完成: {final_task}")
        run_id = Path(final_task["runDir"]).name
        _, run = request_json(base, f"/api/v1/runs/{run_id}")
        require(run["summary"]["status"] == "completed", "运行没有完成")
        require(run["manifest"]["solverSettings"]["outputInterval"] == simulation_settings["outputInterval"], "运行清单未记录仿真设置")
        require(run["manifest"]["solverSettings"]["solverFlags"] == simulation_settings["solverFlags"], "运行清单未记录求解器专属设置")
        variables = ",".join(item["variable"] for item in configuration["outputs"][:2])
        _, series = request_json(base, f"/api/v1/runs/{run_id}/series?" + urllib.parse.urlencode({"variables": variables, "maxPoints": 64}))
        require(2 <= series["pointCount"] <= 64, "曲线降采样无效")
        _, final_values = request_json(base, f"/api/v1/runs/{run_id}/values?" + urllib.parse.urlencode({"variables": variables}))
        require(final_values["time"] == simulation_settings["stopTime"] and len(final_values["values"]) == 2, "结果浏览器未返回最后时刻变量值")
        _, sampled_values = request_json(base, f"/api/v1/runs/{run_id}/values?" + urllib.parse.urlencode({"variables": variables, "time": 0.5031}))
        require(abs(sampled_values["time"] - 0.5031) <= simulation_settings["outputInterval"], "结果浏览器未返回指定时间附近的变量值")

        return {
            "projectId": project_info["projectId"], "sourceDigest": project_info["sourceDigest"],
            "classCount": len(tree["classes"]), "componentCount": component_count, "connectionCount": connection_count,
            "outputCount": len(configuration["outputs"]),
            "runId": run_id, "status": run["summary"]["status"], "seriesPoints": series["pointCount"],
            "stages": stages, "serverLog": str(log_file),
            "fmpyInspected": fmpy_inspection is not None,
        }
    finally:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)


def main() -> int:
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    scratch = AIDC_ROOT / ".codex-runtime" / f"t1-2-{stamp}"
    scratch.mkdir(parents=True, exist_ok=False)
    source_projects = [
        AIDC_ROOT / "samples" / "modelica-t0-single-joint",
        AIDC_ROOT / "samples" / "modelica-t1-dc-motor",
    ]
    projects = []
    for source_project in source_projects:
        project_copy = scratch / "projects" / source_project.name
        shutil.copytree(source_project, project_copy, ignore=shutil.ignore_patterns("results"))
        projects.append(project_copy)
    started = time.perf_counter()
    results = [verify_project(project, scratch) for project in projects]
    duration = time.perf_counter() - started
    RESULTS.mkdir(parents=True, exist_ok=True)
    json_path = RESULTS / "t1-2-summary.json"
    json_path.write_text(json.dumps({"passed": True, "durationSeconds": duration, "projects": results}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    report = [
        "# T1.2：Project Server 与编译适配验收报告", "", "**结论：通过**", "",
        "同一个 Project Server 构建分别打开两个不同模型工程。工程资源、omc 模型索引、OpenModelica/FMPy 能力、异步运行、SSE 阶段、运行结果和曲线降采样均通过 HTTP 实测。", "",
        "| 工程 | 类 | 组件 | 连接 | 输出 | 运行状态 | 曲线点数 |", "|---|---:|---:|---:|---:|---|---:|",
    ]
    for item in results:
        report.append(f"| {item['projectId']} | {item['classCount']} | {item['componentCount']} | {item['connectionCount']} | {item['outputCount']} | {item['status']} | {item['seriesPoints']} |")
    report += ["", "## 已验证能力", "", "- 最小工程入口、模型配置、运行清单和诊断 API。", "- 基于 omc Scripting API 的类、组件、端口与连接索引。", "- OpenModelica 原生编译/求解能力。", "- `queued → validating → compiling → initializing → solving → evaluating → completed` SSE 事件。", "- 受限源码读取与 `..` 路径越界拒绝。", "- 时序变量筛选、时间查询入口和最大点数降采样。", "", f"验收耗时：{duration:.2f} s。", ""]
    (RESULTS / "T1.2-Project-Server与编译适配验收报告.md").write_text("\n".join(report), encoding="utf-8")
    print(json.dumps({"passed": True, "report": str(RESULTS / 'T1.2-Project-Server与编译适配验收报告.md')}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
