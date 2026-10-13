#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import mimetypes
import os
import re
import subprocess
import sys
import threading
import time
import traceback
import uuid
from dataclasses import dataclass, field
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, unquote, urlparse


SERVER_DIR = Path(__file__).resolve().parent
MODEL_BROWSER = SERVER_DIR.parents[1]
sys.path.insert(0, str(MODEL_BROWSER / "tools"))
sys.path.insert(0, str(SERVER_DIR))

from adapters import adapter_capabilities, inspect_fmu  # noqa: E402
from modelica_indexer import build_model_index  # noqa: E402
from project_tool import ContractError, read_json, resolve_inside, run_project, source_fingerprint, validate_project  # noqa: E402


SOLVER_FLAGS: dict[str, set[str]] = {
    "dassl": {"initialStepSize", "maxStepSize", "maxIntegrationOrder", "jacobian"},
    "ida": {"initialStepSize", "maxStepSize", "maxIntegrationOrder", "jacobian", "idaLS", "idaMaxNonLinIters", "idaMaxConvFails", "idaMaxErrorTestFails", "idaNonLinConvCoef", "idaScaling"},
    "cvode": {"cvodeLinearMultistepMethod", "cvodeNonlinearSolverIteration"},
    "gbode": {"initialStepSize", "maxStepSize", "gbm", "gbctrl", "gbratio"},
    "euler": set(),
    "rungekutta": set(),
}
NUMERIC_SOLVER_FLAGS = {"initialStepSize", "maxStepSize", "maxIntegrationOrder", "idaMaxNonLinIters", "idaMaxConvFails", "idaMaxErrorTestFails", "idaNonLinConvCoef", "gbratio"}
ENUM_SOLVER_FLAGS: dict[str, set[str]] = {
    "jacobian": {"coloredNumerical", "internalNumerical", "numerical", "coloredSymbolical", "symbolical", "coloredSymbolicalAdjoint"},
    "idaLS": {"dense", "klu", "spgmr", "spbcg", "sptfqmr"},
    "cvodeLinearMultistepMethod": {"CV_BDF", "CV_ADAMS"},
    "cvodeNonlinearSolverIteration": {"CV_ITER_NEWTON", "CV_ITER_FIXED_POINT"},
    "gbm": {"adams", "expl_euler", "impl_euler", "trapezoid", "sdirk2", "sdirk3", "sdirk4", "esdirk2", "esdirk3", "esdirk4", "esdirk4s7", "radauIA2", "radauIA3", "radauIA4", "radauIIA2", "radauIIA3", "radauIIA4", "radauIIA5", "radauIIA6", "radauIIA7", "lobattoIIIA3", "lobattoIIIA4", "lobattoIIIB3", "lobattoIIIB4", "lobattoIIIC3", "lobattoIIIC4", "gauss2", "gauss3", "gauss4", "gauss5", "gauss6", "merson", "mersonSsc1", "mersonSsc2", "heun", "fehlberg12", "fehlberg45", "fehlberg78", "fehlbergSsc1", "fehlbergSsc2", "rk810", "rk1012", "rk1214", "dopri45", "dopriSsc1", "dopriSsc2", "tsit5", "rungekutta", "rungekuttaSsc"},
    "gbctrl": {"i", "pi_33", "pi_34", "pi_42", "pid_h312", "pid_soederlind", "pid_stiff", "pc", "pc_hybrid", "pi_h211", "pi_h0_211", "pid_h0_312", "pid_h0_321", "ppid", "const"},
}


def validate_simulation_settings(defaults: dict[str, Any], overrides: dict[str, Any] | None) -> dict[str, Any]:
    settings = {**defaults, **(overrides or {})}
    allowed = {"startTime", "stopTime", "outputInterval", "relativeTolerance", "solver", "solverFlags"}
    unknown = set(settings) - allowed
    if unknown:
        raise ContractError(f"不支持的仿真设置: {', '.join(sorted(unknown))}")
    for name in ("startTime", "stopTime", "outputInterval", "relativeTolerance"):
        if not isinstance(settings.get(name), (int, float)):
            raise ContractError(f"仿真设置 {name} 必须是数值")
    if settings["stopTime"] <= settings["startTime"]:
        raise ContractError("仿真结束时间必须大于开始时间")
    if settings["outputInterval"] <= 0 or settings["relativeTolerance"] <= 0:
        raise ContractError("仿真步长和容差必须大于 0")
    solver = settings.get("solver")
    if solver not in SOLVER_FLAGS:
        raise ContractError(f"当前 OMC 未启用积分算法: {solver}")
    flags = settings.get("solverFlags") or {}
    if not isinstance(flags, dict):
        raise ContractError("solverFlags 必须是对象")
    unsupported = set(flags) - SOLVER_FLAGS[solver]
    if unsupported:
        raise ContractError(f"{solver} 不支持设置: {', '.join(sorted(unsupported))}")
    for name, value in flags.items():
        if name in NUMERIC_SOLVER_FLAGS and not isinstance(value, (int, float)):
            raise ContractError(f"{name} 必须是数值")
        if name in ENUM_SOLVER_FLAGS and value not in ENUM_SOLVER_FLAGS[name]:
            raise ContractError(f"{name} 的值不受 OMC 支持: {value}")
    if "maxIntegrationOrder" in flags and (int(flags["maxIntegrationOrder"]) != flags["maxIntegrationOrder"] or not 1 <= int(flags["maxIntegrationOrder"]) <= 5):
        raise ContractError("maxIntegrationOrder 必须是 1 到 5 的整数")
    if "gbratio" in flags and not 0 <= flags["gbratio"] <= 1:
        raise ContractError("gbratio 必须在 0 到 1 之间")
    for name in NUMERIC_SOLVER_FLAGS - {"gbratio", "maxIntegrationOrder"}:
        if name in flags and flags[name] <= 0:
            raise ContractError(f"{name} 必须大于 0")
    if "idaScaling" in flags and not isinstance(flags["idaScaling"], bool):
        raise ContractError("idaScaling 必须是布尔值")
    settings["solverFlags"] = {name: value for name, value in flags.items() if value not in (None, "", False)}
    return settings


def _number_text(value: Any) -> str:
    return format(float(value), ".15g")

def _unit_for_type(type_name: str) -> str:
    return {"Angle":"rad","AngularVelocity":"rad/s","Current":"A","Torque":"N.m","Voltage":"V","Power":"W"}.get(type_name.rsplit(".", 1)[-1], "")


def _split_variable_query(values: list[str]) -> list[str]:
    """Read repeated variable parameters and keep commas inside Modelica indices."""
    if len(values) != 1:
        return [value for value in values if value]
    value = values[0]
    result: list[str] = []
    start = 0
    depth = 0
    for index, character in enumerate(value):
        if character == "[":
            depth += 1
        elif character == "]":
            depth = max(0, depth - 1)
        elif character == "," and depth == 0:
            if value[start:index]:
                result.append(value[start:index])
            start = index + 1
    if value[start:]:
        result.append(value[start:])
    return result


@dataclass
class TaskRecord:
    task_id: str
    model_class: str
    simulation_settings: dict[str, Any]
    status: str = "queued"
    run_dir: str | None = None
    error: str | None = None
    events: list[dict[str, Any]] = field(default_factory=list)
    condition: threading.Condition = field(default_factory=threading.Condition)

    def emit(self, event: str, data: dict[str, Any]) -> None:
        with self.condition:
            item = {"id": len(self.events) + 1, "event": event, "data": data, "time": time.time()}
            self.events.append(item)
            if event == "stage":
                self.status = data["stage"]
            elif event == "completed":
                self.status = "completed"
                self.run_dir = data["runDir"]
            elif event == "failed":
                self.status = "failed"
                self.error = data["error"]
            self.condition.notify_all()

    def snapshot(self) -> dict[str, Any]:
        with self.condition:
            return {
                "taskId": self.task_id,
                "modelClass": self.model_class,
                "simulationSettings": self.simulation_settings,
                "status": self.status,
                "runDir": self.run_dir,
                "error": self.error,
                "lastEventId": len(self.events),
            }


class ProjectService:
    def __init__(self, project_root: Path):
        self.project_root = project_root.resolve()
        self.loaded = validate_project(self.project_root)
        self.digest, self.source_files = source_fingerprint(self.project_root)
        self.model_index = build_model_index(self.project_root)
        self.tasks: dict[str, TaskRecord] = {}
        self.tasks_lock = threading.Lock()
        self.run_lock = threading.Lock()
        self.saved_simulation_settings = self._read_model_simulation_settings({"startTime": 0.0, "stopTime": 1.0, "outputInterval": 0.001, "relativeTolerance": 1e-6, "solver": "dassl", "solverFlags": {}})

    @property
    def project(self) -> dict[str, Any]:
        return self.loaded["project"]

    def project_summary(self) -> dict[str, Any]:
        return {
            "schemaVersion": "2.0",
            "projectId": self.project["projectId"],
            "name": self.project["name"],
            "revision": self.project["revision"],
            "description": self.project["description"],
            "sourceDigest": self.digest,
            "sourceFileCount": len(self.source_files),
            "modelica": self.project["modelica"],
            "integrity": "valid",
        }

    def configuration(self) -> dict[str, Any]:
        experiment = self.project["modelica"]["defaultExperiment"]
        record = next((item for item in self.model_index["classes"] if item["id"] == experiment), None)
        integer_constants: dict[str, int] = {}
        for source_file in self.project_root.glob("modelica/**/*.mo"):
            text = source_file.read_text(encoding="utf-8-sig")
            for name, value in re.findall(r"\b(?:constant|parameter)\s+Integer\s+(\w+)\s*=\s*(\d+)\b", text):
                integer_constants[name] = int(value)
        outputs = []
        for item in (record or {}).get("components", []):
            if item.get("causality") == "output":
                dimensions = item.get("dimensions", [])
                sizes: list[int] = []
                for dimension in dimensions:
                    try:
                        size = int(dimension)
                    except (TypeError, ValueError):
                        size = integer_constants.get(str(dimension), 0)
                    if size <= 0:
                        sizes = []
                        break
                    sizes.append(size)
                names = [item["name"]]
                if dimensions and sizes:
                    indices: list[tuple[int, ...]] = [()]
                    for size in sizes:
                        indices = [(*prefix, index) for prefix in indices for index in range(1, size + 1)]
                    names = [f'{item["name"]}[{",".join(map(str, index))}]' for index in indices]
                elif dimensions:
                    names = []
                outputs.extend({"variable": name, "unit": _unit_for_type(item.get("type", "")), "description": item.get("comment", "")} for name in names)
        return {"modelClass": experiment, "simulation": self.saved_simulation_settings, "outputs": outputs}

    def _experiment_source(self) -> tuple[Path, str, str]:
        experiment = self.project["modelica"]["defaultExperiment"]
        record = next((item for item in self.model_index["classes"] if item["id"] == experiment), None)
        if not record or not record.get("source", {}).get("path"):
            raise ContractError(f"找不到实验模型源码: {experiment}")
        path = resolve_inside(self.project_root, record["source"]["path"])
        return path, experiment.rsplit(".", 1)[-1], path.read_text(encoding="utf-8-sig")

    def _read_model_simulation_settings(self, defaults: dict[str, Any]) -> dict[str, Any]:
        settings = {**defaults, "solverFlags": {}}
        try:
            _, _, text = self._experiment_source()
        except ContractError:
            return settings
        experiment = re.search(r"\bexperiment\s*\(([^)]*)\)", text, re.DOTALL)
        if experiment:
            names = {"StartTime": "startTime", "StopTime": "stopTime", "Interval": "outputInterval", "Tolerance": "relativeTolerance"}
            for source_name, target_name in names.items():
                match = re.search(rf"\b{source_name}\s*=\s*([-+0-9.eE]+)", experiment.group(1))
                if match:
                    settings[target_name] = float(match.group(1))
        flags = re.search(r'__OpenModelica_simulationFlags\s*\(((?:[^"()]|"(?:\\.|[^"])*")*)\)', text, re.DOTALL)
        if flags:
            values = dict(re.findall(r'(\w+)\s*=\s*"([^"]*)"', flags.group(1)))
            settings["solver"] = values.pop("s", settings["solver"])
            parsed_flags: dict[str, Any] = {}
            for name, value in values.items():
                if value == "()":
                    parsed_flags[name] = True
                elif name in NUMERIC_SOLVER_FLAGS:
                    parsed_flags[name] = float(value) if any(char in value.lower() for char in ".e") else int(value)
                else:
                    parsed_flags[name] = value
            settings["solverFlags"] = parsed_flags
        return validate_simulation_settings(defaults, settings)

    def save_simulation_settings(self, incoming: dict[str, Any]) -> dict[str, Any]:
        settings = validate_simulation_settings(self.saved_simulation_settings, incoming)
        path, class_name, text = self._experiment_source()
        start = re.search(rf"\b(?:model|block|class)\s+{re.escape(class_name)}\b", text)
        if not start:
            raise ContractError(f"源码中找不到模型 {class_name}")
        end = re.search(rf"\bend\s+{re.escape(class_name)}\s*;", text[start.start():])
        if not end:
            raise ContractError(f"源码中找不到模型结束标记 {class_name}")
        segment_end = start.start() + end.end()
        segment = text[start.start():segment_end]
        experiment_text = (
            f'experiment(StartTime = {_number_text(settings["startTime"])}, StopTime = {_number_text(settings["stopTime"])}, '
            f'Interval = {_number_text(settings["outputInterval"])}, Tolerance = {_number_text(settings["relativeTolerance"])})'
        )
        experiment_pattern = re.compile(r"\bexperiment\s*\([^)]*\)", re.DOTALL)
        if experiment_pattern.search(segment):
            segment = experiment_pattern.sub(experiment_text, segment, count=1)
        else:
            annotation_at = segment.rfind("annotation(")
            if annotation_at < 0:
                raise ContractError("实验模型缺少可写入的 annotation")
            insert_at = annotation_at + len("annotation(")
            segment = segment[:insert_at] + "\n      " + experiment_text + "," + segment[insert_at:]
        flag_values = {"s": settings["solver"], **settings.get("solverFlags", {})}
        flag_items = []
        for name, value in flag_values.items():
            encoded = "()" if value == () or value is True else str(value)
            flag_items.append(f'{name} = "{encoded}"')
        flags_text = "__OpenModelica_simulationFlags(" + ", ".join(flag_items) + ")"
        flags_pattern = re.compile(r'__OpenModelica_simulationFlags\s*\(((?:[^"()]|"(?:\\.|[^"])*")*)\)', re.DOTALL)
        if flags_pattern.search(segment):
            segment = flags_pattern.sub(flags_text, segment, count=1)
        else:
            segment = segment.replace(experiment_text, experiment_text + ",\n      " + flags_text, 1)
        path.write_text(text[:start.start()] + segment + text[segment_end:], encoding="utf-8")
        self.saved_simulation_settings = settings
        self.loaded = validate_project(self.project_root)
        self.digest, self.source_files = source_fingerprint(self.project_root)
        self.model_index["sourceDigest"] = self.digest
        return {"settings": settings, "source": str(path.relative_to(self.project_root)).replace("\\", "/"), "sourceDigest": self.digest}

    def list_runs(self) -> list[dict[str, Any]]:
        runs_root = self.project_root / "results"
        items = []
        if runs_root.is_dir():
            for directory in runs_root.iterdir():
                manifest_path = directory / "run-manifest.json"
                if not directory.is_dir() or not manifest_path.is_file():
                    continue
                try:
                    manifest = read_json(manifest_path)
                    summary_path = directory / "summary.json"
                    summary = read_json(summary_path) if summary_path.is_file() else None
                    items.append({
                        "runId": manifest["runId"], "modelClass": manifest.get("modelClass", manifest.get("scenarioId", self.project["modelica"]["defaultExperiment"])), "status": manifest["status"],
                        "stage": manifest["stage"], "startedAt": manifest["startedAt"], "finishedAt": manifest["finishedAt"],
                        "sourceDigest": manifest["sourceDigest"],
                    })
                except Exception:
                    continue
        return sorted(items, key=lambda item: item["startedAt"], reverse=True)

    def get_run(self, run_id: str) -> dict[str, Any]:
        if not run_id or any(char not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_." for char in run_id):
            raise ContractError("非法 runId")
        run_dir = resolve_inside(self.project_root, f"results/{run_id}")
        manifest_path = run_dir / "run-manifest.json"
        if not manifest_path.is_file():
            raise FileNotFoundError(run_id)
        result = {"manifest": read_json(manifest_path)}
        for name in ("summary", "diagnostics"):
            path = run_dir / f"{name}.json"
            result[name] = read_json(path) if path.is_file() else None
        return result

    def get_series(self, run_id: str, variables: list[str], start: float | None, end: float | None, max_points: int) -> dict[str, Any]:
        run_dir = resolve_inside(self.project_root, f"results/{run_id}")
        series_path = run_dir / "series.csv"
        if not series_path.is_file():
            raise FileNotFoundError(run_id)
        with series_path.open("r", encoding="utf-8-sig", newline="") as handle:
            reader = csv.DictReader(handle)
            available = reader.fieldnames or []
            selected = variables or [name for name in available if name != "time"]
            unknown = [name for name in selected if name not in available]
            if unknown:
                raise ContractError(f"未知时序变量: {', '.join(unknown)}")
            rows = []
            for row in reader:
                value = float(row["time"])
                if start is not None and value < start: continue
                if end is not None and value > end: continue
                rows.append([value, *[float(row[name]) for name in selected]])
        max_points = max(2, min(max_points, 5000))
        if len(rows) > max_points:
            indices = sorted({round(index * (len(rows) - 1) / (max_points - 1)) for index in range(max_points)})
            rows = [rows[index] for index in indices]
        return {"runId": run_id, "columns": ["time", *selected], "rows": rows, "pointCount": len(rows)}

    def get_values(self, run_id: str, variables: list[str], requested_time: float | None) -> dict[str, Any]:
        run_dir = resolve_inside(self.project_root, f"results/{run_id}")
        series_path = run_dir / "series.csv"
        if not series_path.is_file():
            raise FileNotFoundError(run_id)
        with series_path.open("r", encoding="utf-8-sig", newline="") as handle:
            reader = csv.DictReader(handle)
            available = reader.fieldnames or []
            selected = variables or [name for name in available if name != "time"]
            unknown = [name for name in selected if name not in available]
            if unknown:
                raise ContractError(f"未知结果变量: {', '.join(unknown)}")
            chosen: dict[str, str] | None = None
            chosen_distance = float("inf")
            for row in reader:
                if requested_time is None:
                    chosen = row
                    continue
                distance = abs(float(row["time"]) - requested_time)
                if distance < chosen_distance:
                    chosen, chosen_distance = row, distance
        if chosen is None:
            raise ContractError("仿真结果没有时序数据")
        actual_time = float(chosen["time"])
        return {
            "runId": run_id,
            "requestedTime": requested_time,
            "time": actual_time,
            "values": {name: float(chosen[name]) for name in selected},
        }

    def source(self, relative: str, line: int | None, end_line: int | None = None) -> dict[str, Any]:
        path = resolve_inside(self.project_root, relative)
        relative_path = path.relative_to(self.project_root)
        top_level = relative_path.parts[0] if relative_path.parts else ""
        allowed = path == self.project_root / "model-project.json" or top_level in {"modelica", "results"}
        if not allowed or path.suffix.lower() not in {".mo", ".json", ".md", ".txt", ".csv"}:
            raise ContractError("不允许读取该资源")
        if not path.is_file() or path.stat().st_size > 512 * 1024:
            raise ContractError("资源不存在或超过 512 KiB")
        lines = path.read_text(encoding="utf-8-sig").splitlines()
        if line and end_line:
            first = max(1, min(line, len(lines)))
            last = max(first, min(end_line, len(lines)))
        elif line:
            first = max(1, line - 10)
            last = min(len(lines), line + 10)
        else:
            first, last = 1, min(len(lines), 400)
        return {"path": relative, "startLine": first, "endLine": last, "totalLines": len(lines), "text": "\n".join(lines[first - 1:last])}

    def inspect_fmu(self, relative: str) -> dict[str, Any]:
        path = resolve_inside(self.project_root, relative)
        generated_root = resolve_inside(self.project_root, "results")
        try:
            path.relative_to(generated_root)
        except ValueError as exc:
            raise ContractError("FMU 必须位于 results/ 内") from exc
        if path.suffix.lower() != ".fmu" or not path.is_file():
            raise ContractError("FMU 文件不存在或扩展名无效")
        return inspect_fmu(path)

    def create_task(self, simulation_settings: dict[str, Any] | None = None) -> TaskRecord:
        settings = validate_simulation_settings(self.saved_simulation_settings, simulation_settings)
        record = TaskRecord(uuid.uuid4().hex, self.project["modelica"]["defaultExperiment"], settings)
        record.emit("stage", {"stage": "queued"})
        with self.tasks_lock:
            self.tasks[record.task_id] = record
        thread = threading.Thread(target=self._run_task, args=(record,), daemon=True)
        thread.start()
        return record

    def _run_task(self, record: TaskRecord) -> None:
        try:
            with self.run_lock:
                run_dir = run_project(self.project_root, lambda stage, details: record.emit("stage", {"stage": stage, **details}), record.simulation_settings)
            record.emit("completed", {"runDir": str(run_dir), "runId": run_dir.name})
        except Exception as exc:
            record.emit("failed", {"error": str(exc), "traceback": traceback.format_exc()})


class ApiHandler(BaseHTTPRequestHandler):
    server: "ProjectHttpServer"

    def log_message(self, format: str, *args: Any) -> None:
        return

    def _headers(self, status: int, content_type: str = "application/json; charset=utf-8") -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Last-Event-ID")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS")
        self.send_header("Cache-Control", "no-store")
        self.end_headers()

    def _json(self, status: int, value: Any) -> None:
        body = json.dumps(value, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _error(self, status: int, code: str, message: str) -> None:
        self._json(status, {"error": {"code": code, "message": message}})

    def _static(self, request_path: str) -> None:
        web_root = self.server.web_root
        if not web_root or not web_root.is_dir():
            return self._error(404, "WEB_NOT_BUILT", "浏览器前端尚未构建，请先执行 npm run build:modelica")
        relative = request_path.lstrip("/") or "index.html"
        candidate = (web_root / relative).resolve()
        try:
            candidate.relative_to(web_root)
        except ValueError:
            return self._error(400, "INVALID_REQUEST", "非法静态资源路径")
        if not candidate.is_file():
            candidate = web_root / "index.html"
        body = candidate.read_bytes()
        explicit_types = {".js": "application/javascript", ".mjs": "application/javascript", ".css": "text/css", ".html": "text/html", ".svg": "image/svg+xml", ".json": "application/json"}
        content_type = explicit_types.get(candidate.suffix.lower()) or mimetypes.guess_type(candidate.name)[0] or "application/octet-stream"
        if content_type.startswith("text/") or content_type in {"application/javascript", "application/json"}:
            content_type += "; charset=utf-8"
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache" if candidate.name == "index.html" else "public, max-age=31536000, immutable")
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self) -> None:
        self._headers(HTTPStatus.NO_CONTENT)

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        path = parsed.path.rstrip("/") or "/"
        query = parse_qs(parsed.query)
        service = self.server.service
        try:
            if path == "/api/v1/projects":
                return self._json(200, {"items": self.server.project_summaries()})
            scoped = self.server.scoped_service(path)
            if scoped:
                service, path = scoped
            if path == "/health": return self._json(200, {"status": "ok", "projectId": service.project["projectId"] if service else None})
            if service is None and path.startswith("/api/"):
                return self._error(404, "NO_PROJECT", "尚未打开 Modelica 工程")
            if path == "/api/v1/project": return self._json(200, service.project_summary())
            if path == "/api/v1/adapters": return self._json(200, {"items": adapter_capabilities()})
            if path == "/api/v1/adapters/fmpy/inspect": return self._json(200, service.inspect_fmu(query.get("path", [""])[0]))
            if path == "/api/v1/model/tree": return self._json(200, service.model_index)
            if path == "/api/v1/model/configuration": return self._json(200, service.configuration())
            if path == "/api/v1/source": return self._json(200, service.source(
                query.get("path", [""])[0],
                int(query["line"][0]) if "line" in query else None,
                int(query["endLine"][0]) if "endLine" in query else None,
            ))
            if path == "/api/v1/runs": return self._json(200, {"items": service.list_runs()})
            if path.startswith("/api/v1/runs/") and path.endswith("/series"):
                run_id = path.split("/")[-2]
                variables = _split_variable_query(query.get("variables", []))
                start = float(query["start"][0]) if "start" in query else None
                end = float(query["end"][0]) if "end" in query else None
                max_points = int(query.get("maxPoints", [1000])[0])
                return self._json(200, service.get_series(run_id, variables, start, end, max_points))
            if path.startswith("/api/v1/runs/") and path.endswith("/values"):
                run_id = path.split("/")[-2]
                variables = _split_variable_query(query.get("variables", []))
                requested_time = float(query["time"][0]) if "time" in query else None
                return self._json(200, service.get_values(run_id, variables, requested_time))
            if path.startswith("/api/v1/runs/"):
                return self._json(200, service.get_run(path.rsplit("/", 1)[-1]))
            if path.startswith("/api/v1/tasks/") and path.endswith("/events"):
                return self._sse(path.split("/")[-2], service)
            if path.startswith("/api/v1/tasks/"):
                task_id = path.rsplit("/", 1)[-1]
                record = service.tasks.get(task_id)
                if not record: raise FileNotFoundError(task_id)
                return self._json(200, record.snapshot())
            if path.startswith("/api/"):
                return self._error(404, "NOT_FOUND", "API 路径不存在")
            return self._static(parsed.path)
        except FileNotFoundError as exc:
            self._error(404, "NOT_FOUND", str(exc))
        except (ContractError, ValueError) as exc:
            self._error(400, "INVALID_REQUEST", str(exc))
        except Exception as exc:
            self._error(500, "INTERNAL_ERROR", str(exc))

    def _sse(self, task_id: str, service: ProjectService) -> None:
        record = service.tasks.get(task_id)
        if not record:
            return self._error(404, "NOT_FOUND", task_id)
        try:
            after = int(self.headers.get("Last-Event-ID", "0"))
        except ValueError:
            after = 0
        self._headers(200, "text/event-stream; charset=utf-8")
        deadline = time.monotonic() + 120
        try:
            while time.monotonic() < deadline:
                with record.condition:
                    pending = [item for item in record.events if item["id"] > after]
                    if not pending and record.status not in {"completed", "failed", "cancelled"}:
                        record.condition.wait(timeout=10)
                        pending = [item for item in record.events if item["id"] > after]
                for item in pending:
                    payload = json.dumps(item["data"], ensure_ascii=False)
                    self.wfile.write(f"id: {item['id']}\nevent: {item['event']}\ndata: {payload}\n\n".encode("utf-8"))
                    self.wfile.flush()
                    after = item["id"]
                if record.status in {"completed", "failed", "cancelled"} and after >= len(record.events):
                    break
                if not pending:
                    self.wfile.write(b": keep-alive\n\n")
                    self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            return

    def do_POST(self) -> None:
        path = urlparse(self.path).path.rstrip("/")
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length > 64 * 1024: return self._error(413, "PAYLOAD_TOO_LARGE", "请求超过 64 KiB")
            body = json.loads(self.rfile.read(length) or b"{}")
            if path == "/api/v1/projects/select-directory":
                return self._json(200, {"path": self.server.select_project_directory()})
            if path == "/api/v1/model-conversion/select-directory":
                kind = body.get("kind", "source") if isinstance(body, dict) else "source"
                return self._json(200, {"path": self.server.select_conversion_directory(kind)})
            if path == "/api/v1/model-conversion":
                if not isinstance(body, dict):
                    raise ContractError("模型转换请求必须是对象")
                return self._json(201, self.server.convert_model(body))
            if path == "/api/v1/projects/open":
                if not isinstance(body, dict) or not isinstance(body.get("path"), str):
                    raise ContractError("打开模型工程需要有效的 path")
                return self._json(201, self.server.open_project(Path(body["path"])))
            service = self.server.service
            scoped = self.server.scoped_service(path)
            if scoped:
                service, path = scoped
            if path not in {"/api/v1/runs", "/api/v1/model/simulation-settings"}:
                return self._error(404, "NOT_FOUND", "API 路径不存在")
            if path == "/api/v1/model/simulation-settings":
                if not isinstance(body, dict): raise ContractError("仿真设置必须是对象")
                return self._json(200, service.save_simulation_settings(body))
            allowed = {"simulationSettings"}
            unknown = set(body) - allowed
            if unknown: raise ContractError(f"不支持的字段: {', '.join(sorted(unknown))}")
            simulation_settings = body.get("simulationSettings")
            if simulation_settings is not None and not isinstance(simulation_settings, dict):
                raise ContractError("simulationSettings 必须是对象")
            task = service.create_task(simulation_settings)
            self._json(202, task.snapshot())
        except (json.JSONDecodeError, ContractError) as exc:
            self._error(400, "INVALID_REQUEST", str(exc))
        except Exception as exc:
            self._error(500, "INTERNAL_ERROR", str(exc))

    def do_DELETE(self) -> None:
        path = urlparse(self.path).path.rstrip("/")
        prefix = "/api/v1/projects/"
        try:
            if not path.startswith(prefix) or "/" in path[len(prefix):]:
                return self._error(404, "NOT_FOUND", "API 路径不存在")
            project_id = unquote(path[len(prefix):])
            self.server.unload_project(project_id)
            self._json(200, {"projectId": project_id, "status": "unloaded"})
        except (ContractError, ValueError) as exc:
            self._error(400, "INVALID_REQUEST", str(exc))
        except Exception as exc:
            self._error(500, "INTERNAL_ERROR", str(exc))


class ProjectHttpServer(ThreadingHTTPServer):
    def __init__(self, address: tuple[str, int], service: ProjectService | None, web_root: Path | None = None):
        super().__init__(address, ApiHandler)
        self.service = service
        self.services: dict[str, ProjectService] = {service.project["projectId"]: service} if service else {}
        self.services_lock = threading.Lock()
        self.web_root = web_root.resolve() if web_root else None

    def project_summaries(self) -> list[dict[str, Any]]:
        with self.services_lock:
            services = list(self.services.values())
        return [{**service.project_summary(), "path": str(service.project_root)} for service in services]

    def select_project_directory(self) -> str:
        return self._select_directory("选择包含 model-project.json 的模型工程目录", False)

    def select_conversion_directory(self, kind: str) -> str:
        if kind == "source-file":
            return self._select_file("选择单零件 STL（仅用于单刚体测试）", "STL 三维模型 (*.stl)|*.stl")
        if kind in {"source", "source-directory"}:
            return self._select_directory("选择包含 assembly.manifest.json 的机器人装配发布包", False)
        if kind == "output":
            return self._select_directory("选择或新建 Modelica 工程输出目录", True)
        raise ContractError(f"未知目录类型: {kind}")

    def _select_file(self, description: str, file_filter: str) -> str:
        if sys.platform != "win32":
            raise ContractError("当前平台不支持系统文件选择器")
        script = r'''
Add-Type -AssemblyName System.Windows.Forms
[System.Windows.Forms.Application]::EnableVisualStyles()
$dialog = New-Object System.Windows.Forms.OpenFileDialog
$dialog.Title = $env:AIDC_DIALOG_DESCRIPTION
$dialog.Filter = $env:AIDC_DIALOG_FILTER
$dialog.Multiselect = $false
$owner = New-Object System.Windows.Forms.Form
$owner.TopMost = $true
$owner.ShowInTaskbar = $false
$owner.Opacity = 0
$owner.Show()
try {
  $result = $dialog.ShowDialog($owner)
  if ($result -eq [System.Windows.Forms.DialogResult]::OK) {
    [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()
    Write-Output $dialog.FileName
  }
} finally {
  $owner.Close()
  $owner.Dispose()
  $dialog.Dispose()
}
'''
        completed = subprocess.run(
            ["powershell.exe", "-NoProfile", "-STA", "-Command", script],
            capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=300, check=False,
            env={**os.environ, "AIDC_DIALOG_DESCRIPTION": description, "AIDC_DIALOG_FILTER": file_filter},
        )
        if completed.returncode != 0:
            raise ContractError(completed.stderr.strip() or "无法打开系统文件选择器")
        return completed.stdout.strip()

    def _select_directory(self, description: str, allow_new: bool) -> str:
        if sys.platform != "win32":
            raise ContractError("当前平台不支持系统文件夹选择器")
        script = r'''
Add-Type -AssemblyName System.Windows.Forms
[System.Windows.Forms.Application]::EnableVisualStyles()
$dialog = New-Object System.Windows.Forms.FolderBrowserDialog
$dialog.Description = $env:AIDC_DIALOG_DESCRIPTION
$dialog.ShowNewFolderButton = $env:AIDC_DIALOG_ALLOW_NEW -eq 'true'
$owner = New-Object System.Windows.Forms.Form
$owner.TopMost = $true
$owner.ShowInTaskbar = $false
$owner.Opacity = 0
$owner.Show()
try {
  $result = $dialog.ShowDialog($owner)
  if ($result -eq [System.Windows.Forms.DialogResult]::OK) {
    [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()
    Write-Output $dialog.SelectedPath
  }
} finally {
  $owner.Close()
  $owner.Dispose()
  $dialog.Dispose()
}
'''
        completed = subprocess.run(
            ["powershell.exe", "-NoProfile", "-STA", "-Command", script],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=300,
            check=False,
            env={**os.environ, "AIDC_DIALOG_DESCRIPTION": description, "AIDC_DIALOG_ALLOW_NEW": str(allow_new).lower()},
        )
        if completed.returncode != 0:
            raise ContractError(completed.stderr.strip() or "无法打开系统文件夹选择器")
        return completed.stdout.strip()

    def convert_model(self, request: dict[str, Any]) -> dict[str, Any]:
        allowed = {"source", "output", "package", "profile", "migrate", "force", "stlUnit", "density", "rootType"}
        unknown = set(request) - allowed
        if unknown:
            raise ContractError(f"不支持的转换字段: {', '.join(sorted(unknown))}")
        source_text, output_text = request.get("source"), request.get("output")
        if not isinstance(source_text, str) or not source_text.strip():
            raise ContractError("请选择机器人装配发布包目录")
        if not isinstance(output_text, str) or not output_text.strip():
            raise ContractError("请选择 Modelica 工程输出目录")
        source, output = Path(source_text).expanduser().resolve(), Path(output_text).expanduser().resolve()
        if not source.exists() or (source.is_file() and source.suffix.lower() != ".stl"):
            raise ContractError(f"转换输入必须是装配发布包目录、Robot IR 目录或测试用 STL 文件: {source}")
        if source == output:
            raise ContractError("输入目录和输出目录不能相同")
        profile = request.get("profile", "auto")
        if profile not in {"auto", "p2", "p3"}:
            raise ContractError(f"不支持的生成 Profile: {profile}")
        package = request.get("package")
        if package is not None and (not isinstance(package, str) or not package.strip()):
            package = None
        command = [sys.executable, str(MODEL_BROWSER / "tools" / "aidc_modelica.py"), "generate", str(source), str(output), "--profile", profile]
        if package:
            command.extend(["--package", package.strip()])
        if request.get("migrate") is True:
            command.append("--migrate")
        if request.get("force") is True:
            command.append("--force")
        if source.is_file():
            stl_unit = request.get("stlUnit", "mm")
            root_type = request.get("rootType", "fixed")
            density = request.get("density", 1000)
            if stl_unit not in {"mm", "m"}:
                raise ContractError("STL 单位必须是 mm 或 m")
            if root_type not in {"fixed", "floating"}:
                raise ContractError("根类型必须是 fixed 或 floating")
            if not isinstance(density, (int, float)) or not 0 < float(density) <= 50000:
                raise ContractError("材料密度必须在 0 到 50000 kg/m³ 之间")
            command.extend(["--stl-unit", stl_unit, "--density", str(float(density)), "--root-type", root_type])
        completed = subprocess.run(
            command,
            cwd=MODEL_BROWSER,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=600,
            check=False,
        )
        payload: dict[str, Any] = {}
        for stream in (completed.stdout, completed.stderr):
            try:
                candidate = json.loads(stream.strip())
                if isinstance(candidate, dict):
                    payload = candidate
                    break
            except json.JSONDecodeError:
                continue
        if completed.returncode != 0 or payload.get("passed") is False:
            diagnostics = payload.get("diagnostics") or []
            if isinstance(diagnostics, list) and diagnostics:
                message = diagnostics[0].get("message") if isinstance(diagnostics[0], dict) else str(diagnostics[0])
            else:
                message = completed.stderr.strip() or completed.stdout.strip() or "三维模型转换失败"
            raise ContractError(message)
        summary = self.open_project(output)
        return {"project": summary, "generation": payload}

    def open_project(self, project_root: Path) -> dict[str, Any]:
        candidate = ProjectService(project_root.expanduser().resolve())
        project_id = candidate.project["projectId"]
        with self.services_lock:
            existing = self.services.get(project_id)
            if existing and existing.project_root != candidate.project_root:
                raise ContractError(f"工程标识已加载但路径不同: {project_id}")
            # Opening the same path is also the explicit reload operation.  A
            # generated project may have changed on disk since it was first
            # loaded, so retaining the old ProjectService would leave the tree,
            # source ranges and component diagram stale.
            self.services[project_id] = candidate
            if self.service is existing:
                self.service = candidate
            service = candidate
        return {**service.project_summary(), "path": str(service.project_root)}

    def unload_project(self, project_id: str) -> None:
        with self.services_lock:
            if project_id not in self.services:
                raise ContractError(f"模型工程尚未加载: {project_id}")
            removed = self.services.pop(project_id)
            if self.service is removed:
                self.service = next(iter(self.services.values()), None)

    def scoped_service(self, path: str) -> tuple[ProjectService, str] | None:
        prefix = "/api/v1/projects/"
        if not path.startswith(prefix):
            return None
        remainder = path[len(prefix):]
        if "/" not in remainder:
            return None
        encoded_id, suffix = remainder.split("/", 1)
        project_id = unquote(encoded_id)
        with self.services_lock:
            service = self.services.get(project_id)
        if not service:
            raise FileNotFoundError(project_id)
        return service, "/api/v1/" + suffix


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", type=Path)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4185)
    parser.add_argument("--write-ready", type=Path)
    parser.add_argument("--web-root", type=Path, default=MODEL_BROWSER / "apps" / "web" / "dist")
    args = parser.parse_args()
    try:
        service = ProjectService(args.project) if args.project else None
        server = ProjectHttpServer((args.host, args.port), service, args.web_root)
        ready = {"status": "ready", "host": args.host, "port": server.server_address[1], "projectId": service.project["projectId"] if service else None, "sourceDigest": service.digest if service else None}
        if args.write_ready:
            args.write_ready.parent.mkdir(parents=True, exist_ok=True)
            args.write_ready.write_text(json.dumps(ready, ensure_ascii=False), encoding="utf-8")
        print(json.dumps(ready, ensure_ascii=False), flush=True)
        server.serve_forever()
        return 0
    except KeyboardInterrupt:
        return 0
    except Exception as exc:
        print(json.dumps({"status": "failed", "error": str(exc), "traceback": traceback.format_exc()}, ensure_ascii=False), file=sys.stderr, flush=True)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
