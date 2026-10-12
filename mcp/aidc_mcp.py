from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any
from uuid import uuid4

from jsonschema import Draft202012Validator

from aidc_core import __version__
from aidc_core.artifacts import ArtifactRegistry
from aidc_core.paths import product_root, state_root
from aidc_core.result import AidcError, CommandResult, utc_now
from aidc_core.runtime import RuntimeManager

PROTOCOL_VERSION = "2025-06-18"


def _cli_command(root: Path) -> list[str]:
    executable = root / "bin" / "aidc.exe"
    if getattr(sys, "frozen", False) and executable.is_file():
        return [str(executable)]
    return [sys.executable, str(root / "aidc.py")]


def _arguments(capability: str, values: dict[str, Any]) -> list[str]:
    mappings: dict[str, list[str]] = {
        "system.capabilities": ["capabilities"],
        "system.doctor": ["doctor"],
        "sysml.validate": ["sysml", "validate", values.get("path", "")],
        "sysml.open": ["sysml", "open", values.get("path", ""), "--no-browser"],
        "model3d.open": ["model3d", "open", values.get("projectPath", ""), "--no-browser"],
        "model3d.publish": ["model3d", "publish", values.get("inputPath", ""), values.get("outputPath", "")],
        "model3d.export-modelica": ["model3d", "export-modelica", values.get("projectPath", ""), values.get("outputPath", "")],
        "modelica.validate": ["modelica", "validate", values.get("path", "")],
        "modelica.generate": ["modelica", "generate", values.get("inputPath", ""), values.get("outputPath", "")],
        "modelica.open": ["modelica", "open", values.get("projectPath", ""), "--no-browser"],
        "modelica.simulate": ["modelica", "simulate", values.get("projectPath", "")],
        "modelica.package": ["modelica", "package", values.get("projectPath", ""), values.get("outputPath", "")],
        "freecad.open": ["freecad", "open", values.get("path", "")],
    }
    arguments = list(mappings[capability])
    if capability == "system.doctor" and values.get("reportPath"): arguments += ["--report", values["reportPath"]]
    if capability == "modelica.validate" and values.get("migrate"): arguments.append("--migrate")
    if capability == "modelica.generate" and values.get("profile"): arguments += ["--profile", values["profile"]]
    if capability == "modelica.simulate" and values.get("experiment"): arguments += ["--experiment", values["experiment"]]
    if capability == "modelica.package" and values.get("includeResults"): arguments.append("--include-results")
    return arguments


class TaskRegistry:
    def __init__(self, root: Path):
        self.root = root
        self.directory = state_root() / "mcp-tasks"
        self.directory.mkdir(parents=True, exist_ok=True)
        self.index = self.directory / "index.json"
        self.artifacts = ArtifactRegistry()
        self._processes: dict[str, subprocess.Popen[Any]] = {}

    def _load(self) -> dict[str, Any]:
        try: return json.loads(self.index.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError): return {"schemaVersion": "aidc.mcp-tasks/1.0", "tasks": []}

    def _save(self, state: dict[str, Any]) -> None:
        temporary = self.index.with_suffix(".tmp")
        temporary.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
        os.replace(temporary, self.index)

    def start(self, tool: dict[str, Any], arguments: list[str]) -> dict[str, Any]:
        task_id = str(uuid4()); result_path = self.directory / f"{task_id}.json"; log_path = self.directory / f"{task_id}.log"
        output = result_path.open("wb"); log = log_path.open("wb")
        environment = os.environ.copy(); environment["PYTHONUTF8"] = "1"
        flags = 0
        if os.name == "nt": flags = subprocess.CREATE_NEW_PROCESS_GROUP | subprocess.CREATE_NO_WINDOW
        try:
            process = subprocess.Popen([*_cli_command(self.root), *arguments, "--json"], cwd=self.root, env=environment, stdin=subprocess.DEVNULL, stdout=output, stderr=log, creationflags=flags, close_fds=True)
        finally:
            output.close(); log.close()
        task = {"taskId": task_id, "tool": tool["name"], "command": tool["capability"], "state": "running", "progress": None, "pid": process.pid, "createdAt": utc_now(), "finishedAt": None, "resultPath": str(result_path), "logPath": str(log_path), "artifacts": []}
        self._processes[task_id] = process
        state = self._load(); state["tasks"].append(task); self._save(state)
        return self.public(task)

    def _refresh(self, task: dict[str, Any]) -> None:
        if task["state"] != "running": return
        if RuntimeManager._alive(int(task["pid"])): return
        process = self._processes.pop(task["taskId"], None)
        if process: process.wait(timeout=5)
        path = Path(task["resultPath"])
        try:
            document = json.loads(path.read_text(encoding="utf-8"))
            task["result"] = document; task["state"] = "succeeded" if document.get("status") == "succeeded" else "failed"
            task["artifacts"] = self.artifacts.record(document, task["taskId"])
        except (OSError, json.JSONDecodeError) as exc:
            task["state"] = "failed"; task["error"] = f"任务未生成有效结果：{exc}"
        task["progress"] = 1; task["finishedAt"] = utc_now()

    def get(self, task_id: str) -> dict[str, Any]:
        state = self._load(); task = next((item for item in state["tasks"] if item["taskId"] == task_id), None)
        if not task: raise AidcError(f"任务不存在：{task_id}", 3, "TASK_NOT_FOUND")
        self._refresh(task); self._save(state); return self.public(task)

    def cancel(self, task_id: str) -> dict[str, Any]:
        state = self._load(); task = next((item for item in state["tasks"] if item["taskId"] == task_id), None)
        if not task: raise AidcError(f"任务不存在：{task_id}", 3, "TASK_NOT_FOUND")
        self._refresh(task)
        if task["state"] != "running": raise AidcError(f"任务已结束，不能取消：{task_id}", 7, "TASK_ALREADY_FINISHED")
        RuntimeManager._stop_pid(int(task["pid"])); process = self._processes.pop(task_id, None)
        if process:
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired: pass
        task["state"] = "cancelled"; task["finishedAt"] = utc_now()
        self._save(state); return self.public(task)

    @staticmethod
    def public(task: dict[str, Any]) -> dict[str, Any]:
        return {key: task.get(key) for key in ("taskId", "tool", "command", "state", "progress", "createdAt", "finishedAt", "logPath", "result", "artifacts", "error") if task.get(key) is not None}


class McpServer:
    def __init__(self, root: Path | None = None):
        self.root = (root or product_root()).resolve()
        catalog_path = self.root / "distribution" / "contracts" / "mcp-tools.json"
        self.catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        self.tools = {tool["name"]: tool for tool in self.catalog["tools"]}
        skill_catalog = json.loads((self.root / "skills" / "catalog.json").read_text(encoding="utf-8"))
        self.skills = {f"aidc-skill://{item['id']}/SKILL.md": item for item in skill_catalog["skills"]}
        self.tasks = TaskRegistry(self.root); self.artifacts = self.tasks.artifacts

    @staticmethod
    def _tool_result(document: dict[str, Any]) -> dict[str, Any]:
        return {"content": [{"type": "text", "text": document.get("message", "AIDC result")}], "structuredContent": document, "isError": document.get("status") != "succeeded"}

    def _execute(self, arguments: list[str]) -> dict[str, Any]:
        completed = subprocess.run([*_cli_command(self.root), *arguments, "--json"], cwd=self.root, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=600, check=False)
        try: document = json.loads(completed.stdout)
        except json.JSONDecodeError:
            document = CommandResult("mcp.tool", "AIDC CLI 未返回有效 JSON", status="failed", exit_code=10, diagnostics=[{"code": "CLI_PROTOCOL_ERROR", "severity": "error", "message": completed.stderr or completed.stdout, "path": None, "details": {}}]).finish()
        self.artifacts.record(document)
        return document

    def call_tool(self, name: str, values: dict[str, Any]) -> dict[str, Any]:
        tool = self.tools.get(name)
        if not tool: return self._tool_result(CommandResult("mcp.tools.call", f"未知工具：{name}", status="failed", exit_code=3).finish())
        errors = sorted(Draft202012Validator(tool["inputSchema"]).iter_errors(values), key=lambda item: list(item.path))
        if errors:
            message = "; ".join(error.message for error in errors)
            return self._tool_result(CommandResult(tool["capability"], f"参数无效：{message}", status="failed", exit_code=2, diagnostics=[{"code": "MCP_INPUT_INVALID", "severity": "error", "message": message, "path": None, "details": {}}]).finish())
        capability = tool["capability"]
        try:
            if capability == "task.status":
                task = self.tasks.get(values["taskId"]); document = CommandResult(capability, f"任务状态：{task['state']}", task, task={"taskId": task["taskId"], "state": task["state"], "progress": task.get("progress")}).finish()
            elif capability == "task.cancel":
                task = self.tasks.cancel(values["taskId"]); document = CommandResult(capability, "任务已取消", task, task={"taskId": task["taskId"], "state": task["state"], "progress": task.get("progress")}).finish()
            elif capability == "artifacts.list":
                items = self.artifacts.list(values.get("taskId")); document = CommandResult(capability, f"找到 {len(items)} 个产物", {"artifacts": items}).finish()
            elif capability == "artifacts.get":
                item = self.artifacts.get(values["artifactId"]); document = CommandResult(capability, "产物信息已读取", item, artifacts=[item]).finish()
            else:
                arguments = _arguments(capability, values)
                if tool["longRunning"]:
                    task = self.tasks.start(tool, arguments); document = CommandResult(capability, "任务已提交", {"accepted": True}, task={"taskId": task["taskId"], "state": "running", "progress": None}).finish()
                else: document = self._execute(arguments)
        except AidcError as exc:
            document = CommandResult(capability, str(exc), status="failed", exit_code=exc.exit_code, diagnostics=[{"code": exc.code, "severity": "error", "message": str(exc), "path": exc.path, "details": {}}]).finish()
        return self._tool_result(document)

    def handle(self, request: dict[str, Any]) -> dict[str, Any] | None:
        request_id = request.get("id"); method = request.get("method")
        if request_id is None: return None
        try:
            if method == "initialize":
                result = {"protocolVersion": PROTOCOL_VERSION, "capabilities": {"tools": {"listChanged": False}, "resources": {"subscribe": False, "listChanged": False}}, "serverInfo": {"name": "aidc-mcp", "title": "AIDC Engineering Tools", "version": __version__}, "instructions": "Use AIDC tools for explicit robot engineering projects. Long operations return a taskId; query aidc_task_status before consuming artifacts. Versioned AIDC Skills are available as read-only resources."}
            elif method == "ping": result = {}
            elif method == "tools/list":
                result = {"tools": [{"name": item["name"], "title": item["title"], "description": item["description"], "inputSchema": item["inputSchema"], "annotations": {"readOnlyHint": item["sideEffect"] == "read", "destructiveHint": False, "idempotentHint": item["sideEffect"] == "read", "openWorldHint": False}} for item in self.catalog["tools"]]}
            elif method == "tools/call":
                parameters = request.get("params") or {}; result = self.call_tool(parameters.get("name", ""), parameters.get("arguments") or {})
            elif method == "resources/list":
                result = {"resources": [{"uri": uri, "name": item["id"], "title": item["id"].replace("-", " ").title(), "description": f"AIDC Skill {item['id']} {item['version']}", "mimeType": "text/markdown"} for uri, item in self.skills.items()]}
            elif method == "resources/read":
                uri = (request.get("params") or {}).get("uri", ""); item = self.skills.get(uri)
                if not item: return {"jsonrpc": "2.0", "id": request_id, "error": {"code": -32002, "message": f"Resource not found: {uri}"}}
                skill_root = (self.root / "skills").resolve(); path = (skill_root / item["path"]).resolve()
                if skill_root not in path.parents: return {"jsonrpc": "2.0", "id": request_id, "error": {"code": -32602, "message": "Unsafe skill resource path"}}
                result = {"contents": [{"uri": uri, "mimeType": "text/markdown", "text": path.read_text(encoding="utf-8")}]}
            else: return {"jsonrpc": "2.0", "id": request_id, "error": {"code": -32601, "message": f"Method not found: {method}"}}
            return {"jsonrpc": "2.0", "id": request_id, "result": result}
        except Exception as exc:
            return {"jsonrpc": "2.0", "id": request_id, "error": {"code": -32603, "message": str(exc)}}


def main() -> int:
    server = McpServer()
    for line in sys.stdin:
        if not line.strip(): continue
        try: request = json.loads(line); response = server.handle(request)
        except json.JSONDecodeError as exc: response = {"jsonrpc": "2.0", "id": None, "error": {"code": -32700, "message": str(exc)}}
        if response is not None:
            print(json.dumps(response, ensure_ascii=False, separators=(",", ":")), flush=True)
    return 0


if __name__ == "__main__": raise SystemExit(main())
