from __future__ import annotations

import hashlib
import json
import os
import secrets
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request
import webbrowser
from pathlib import Path
from typing import Any
from uuid import uuid4

from .paths import product_root, state_root
from .registry import ComponentRegistry
from .result import AidcError, utc_now


class RuntimeManager:
    def __init__(self, root: Path | None = None, state_directory: Path | None = None):
        self.root = (root or product_root()).resolve()
        self.state_directory = (state_directory or state_root()).resolve()
        self.state_directory.mkdir(parents=True, exist_ok=True)
        (self.state_directory / "logs").mkdir(exist_ok=True)
        self.state_file = self.state_directory / "runtime-state.json"

    def _load(self) -> dict[str, Any]:
        if not self.state_file.exists(): return {"schemaVersion": "1.0", "instances": [], "tasks": []}
        try: return json.loads(self.state_file.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError): return {"schemaVersion": "1.0", "instances": [], "tasks": []}

    def _save(self, state: dict[str, Any]) -> None:
        temporary = self.state_file.with_suffix(".tmp")
        temporary.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
        os.replace(temporary, self.state_file)

    @staticmethod
    def _port() -> int:
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0)); return int(sock.getsockname()[1])

    @staticmethod
    def _alive(pid: int) -> bool:
        if pid <= 0: return False
        try:
            if os.name == "nt":
                result = subprocess.run(["tasklist", "/FI", f"PID eq {pid}", "/FO", "CSV", "/NH"], capture_output=True, text=True, timeout=3, check=False)
                return f'"{pid}"' in result.stdout
            os.kill(pid, 0); return True
        except (OSError, subprocess.SubprocessError): return False

    @staticmethod
    def _http_ok(url: str, timeout: float = 1.0) -> bool:
        try:
            with urllib.request.urlopen(url, timeout=timeout) as response: return 200 <= response.status < 300
        except (OSError, urllib.error.URLError): return False

    @staticmethod
    def _launch_url(instance: dict[str, Any]) -> str:
        suffix = "&runtime=/runtime" if instance["kind"] == "sysml" else ""
        return instance["url"] + "?token=" + instance["token"] + suffix

    def _wait_http(self, url: str, process: subprocess.Popen[Any], timeout: float = 30) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if process.poll() is not None: raise AidcError(f"服务启动失败，退出码 {process.returncode}", 6, "SERVICE_START_FAILED")
            if self._http_ok(url): return
            time.sleep(0.2)
        raise AidcError(f"服务健康检查超时：{url}", 11, "HEALTH_TIMEOUT")

    def _spawn(self, instance_id: str, role: str, command: list[str], cwd: Path, env: dict[str, str] | None = None) -> dict[str, Any]:
        log_path = self.state_directory / "logs" / f"{instance_id}-{role}.log"
        log = log_path.open("ab", buffering=0)
        flags = 0
        if os.name == "nt": flags = subprocess.CREATE_NEW_PROCESS_GROUP | subprocess.DETACHED_PROCESS | subprocess.CREATE_NO_WINDOW
        process = subprocess.Popen(command, cwd=cwd, env=env, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, creationflags=flags, close_fds=True)
        log.close()
        return {"role": role, "pid": process.pid, "command": command, "logPath": str(log_path), "_process": process}

    def _start_task(self, command: str, instance_id: str) -> dict[str, Any]:
        return {"taskId": str(uuid4()), "command": command, "instanceId": instance_id, "state": "running", "createdAt": utc_now(), "finishedAt": None}

    def _clean(self, state: dict[str, Any]) -> None:
        for instance in state["instances"]:
            if instance.get("status") == "running" and not all(self._alive(int(item["pid"])) for item in instance.get("processes", [])):
                instance["status"] = "stopped"; instance["stoppedAt"] = utc_now(); instance["stopReason"] = "process-exited"

    def start(self, kind: str, target: Path, launch_browser: bool = True) -> dict[str, Any]:
        target = target.resolve()
        state = self._load(); self._clean(state)
        key = hashlib.sha256(f"{kind}\0{str(target).casefold()}".encode()).hexdigest()
        existing = next((item for item in state["instances"] if item.get("reuseKey") == key and item.get("status") == "running"), None)
        if existing and self._http_ok(existing["healthUrl"]):
            existing["lastUsedAt"] = utc_now(); self._save(state)
            launch_url = self._launch_url(existing)
            if launch_browser: webbrowser.open(launch_url)
            return {**self._public(existing), "reused": True, "launchUrl": launch_url}

        instance_id = str(uuid4()); token = secrets.token_urlsafe(32); public_port = self._port()
        task = self._start_task(f"{kind}.open", instance_id); processes: list[dict[str, Any]] = []
        try:
            if kind == "sysml":
                processes.extend(self._start_sysml(instance_id, target, public_port, token))
            elif kind == "model3d":
                processes.extend(self._start_model3d(instance_id, target, public_port, token))
            elif kind == "modelica":
                processes.extend(self._start_modelica(instance_id, target, public_port, token))
            else: raise AidcError(f"未知浏览器类型：{kind}", 2, "USAGE_ERROR")
            gateway = processes[-1]["_process"]
            health_url = f"http://127.0.0.1:{public_port}/health?token={token}"
            self._wait_http(health_url, gateway)
            now = utc_now()
            instance = {
                "instanceId": instance_id, "kind": kind, "target": str(target), "reuseKey": key, "status": "running",
                "host": "127.0.0.1", "port": public_port, "url": f"http://127.0.0.1:{public_port}/", "healthUrl": health_url,
                "token": token, "ownerPid": os.getpid(), "createdAt": now, "lastUsedAt": now,
                "processes": [{k: v for k, v in item.items() if k != "_process"} for item in processes],
            }
            task["state"] = "succeeded"; task["finishedAt"] = now
            state["instances"].append(instance); state["tasks"].append(task); self._save(state)
            launch_url = self._launch_url(instance)
            if launch_browser: webbrowser.open(launch_url)
            return {**self._public(instance), "reused": False, "launchUrl": launch_url, "taskId": task["taskId"]}
        except Exception:
            for item in reversed(processes): self._stop_pid(int(item["pid"]))
            task["state"] = "failed"; task["finishedAt"] = utc_now(); state["tasks"].append(task); self._save(state)
            raise

    def _gateway(self, instance_id: str, public_port: int, token: str, arguments: list[str]) -> dict[str, Any]:
        command = [sys.executable, "__gateway", "--port", str(public_port), *arguments] if getattr(sys, "frozen", False) else [sys.executable, "-m", "aidc_core.gateway", "--port", str(public_port), *arguments]
        environment = os.environ.copy(); environment["AIDC_SESSION_TOKEN"] = token
        if getattr(sys, "frozen", False): environment["PYINSTALLER_RESET_ENVIRONMENT"] = "1"
        return self._spawn(instance_id, "gateway", command, self.root, environment)

    def _start_sysml(self, instance_id: str, target: Path, public_port: int, token: str) -> list[dict[str, Any]]:
        if target.suffix.lower() not in {".sysml", ".kerml"}: raise AidcError("SysML 输入必须是 .sysml 或 .kerml 文件", 3, "INPUT_INVALID", str(target))
        executable = next((Path(item["path"]) for item in ComponentRegistry(self.root).dependency_status("sysml-parser-runtime") if item["path"]), None)
        if not executable: raise AidcError("未找到 SysML Parser Runtime 0.8.1", 4, "DEPENDENCY_MISSING")
        static_root = self.root / "sysml-browser" / "dist"
        if not (static_root / "index.html").is_file(): raise AidcError("SysML 浏览器生产资源未构建", 4, "COMPONENT_NOT_BUILT")
        runtime_port = self._port()
        runtime = self._spawn(instance_id, "sysml-runtime", [str(executable), "--port", str(runtime_port)], self.root)
        try:
            self._wait_http(f"http://127.0.0.1:{runtime_port}/health", runtime["_process"])
            gateway = self._gateway(instance_id, public_port, token, ["--static", str(static_root), "--map", f"/runtime=http://127.0.0.1:{runtime_port}", "--input", str(target)])
            return [runtime, gateway]
        except Exception:
            self._stop_pid(runtime["pid"]); raise

    def _start_model3d(self, instance_id: str, target: Path, public_port: int, token: str) -> list[dict[str, Any]]:
        manifest = target / "generated" / "model3d.manifest.json"
        if not manifest.is_file(): raise AidcError("三维工程缺少 generated/model3d.manifest.json", 3, "INPUT_INVALID", str(target))
        npm = ComponentRegistry.executable("npm.cmd" if os.name == "nt" else "npm")
        if not npm: raise AidcError("未找到受管 Node.js/npm", 4, "DEPENDENCY_MISSING")
        internal_port = self._port(); exact_port = self._port(); env = os.environ.copy()
        env.update({"AIDC_MODEL3D_PROJECT_DIR": str(target), "AIDC_EXACT_SERVICE_PORT": str(exact_port), "NO_COLOR": "1"})
        command = [npm, "--workspace", "@aidc/model3d-browser", "run", "dev", "--", "--host", "127.0.0.1", "--port", str(internal_port), "--strictPort"]
        vite = self._spawn(instance_id, "model3d-web", command, self.root, env)
        try:
            self._wait_http(f"http://127.0.0.1:{internal_port}/", vite["_process"], 45)
            gateway = self._gateway(instance_id, public_port, token, ["--proxy", f"http://127.0.0.1:{internal_port}"])
            return [vite, gateway]
        except Exception:
            self._stop_pid(vite["pid"]); raise

    def _start_modelica(self, instance_id: str, target: Path, public_port: int, token: str) -> list[dict[str, Any]]:
        if not (target / "model-project.json").is_file(): raise AidcError("Modelica 工程缺少 model-project.json", 3, "INPUT_INVALID", str(target))
        web_root = self.root / "modelica-browser" / "apps" / "web" / "dist"
        if not (web_root / "index.html").is_file(): raise AidcError("Modelica 浏览器生产资源未构建", 4, "COMPONENT_NOT_BUILT")
        backend_port = self._port(); server = self.root / "modelica-browser" / "services" / "project-server" / "server.py"
        managed_python = self.root.parent / "tools" / "openmodelica" / "fmpy-venv" / "Scripts" / "python.exe"
        python = managed_python if managed_python.is_file() else Path(sys.executable)
        backend = self._spawn(instance_id, "modelica-project", [str(python), str(server), "--project", str(target), "--host", "127.0.0.1", "--port", str(backend_port), "--web-root", str(web_root)], self.root)
        try:
            self._wait_http(f"http://127.0.0.1:{backend_port}/health", backend["_process"], 90)
            gateway = self._gateway(instance_id, public_port, token, ["--proxy", f"http://127.0.0.1:{backend_port}"])
            return [backend, gateway]
        except Exception:
            self._stop_pid(backend["pid"]); raise

    @staticmethod
    def _public(instance: dict[str, Any]) -> dict[str, Any]:
        return {key: instance.get(key) for key in ("instanceId", "kind", "target", "status", "host", "port", "url", "createdAt", "lastUsedAt", "processes")}

    def status(self) -> list[dict[str, Any]]:
        state = self._load(); self._clean(state); self._save(state)
        return [self._public(item) for item in state["instances"]]

    def task(self, task_id: str) -> dict[str, Any] | None:
        return next((item for item in self._load()["tasks"] if item["taskId"] == task_id), None)

    def cancel_task(self, task_id: str) -> dict[str, Any]:
        state = self._load(); task = next((item for item in state["tasks"] if item["taskId"] == task_id), None)
        if not task: raise AidcError(f"任务不存在：{task_id}", 3, "TASK_NOT_FOUND")
        if task["state"] != "running": raise AidcError(f"任务已结束，不能取消：{task_id}", 7, "TASK_ALREADY_FINISHED")
        instance = next((item for item in state["instances"] if item["instanceId"] == task.get("instanceId")), None)
        if instance:
            for process in reversed(instance.get("processes", [])): self._stop_pid(int(process["pid"]))
            instance["status"] = "stopped"; instance["stoppedAt"] = utc_now(); instance["stopReason"] = "task-cancelled"
        task["state"] = "cancelled"; task["finishedAt"] = utc_now(); self._save(state)
        return task

    @staticmethod
    def _stop_pid(pid: int) -> None:
        if not RuntimeManager._alive(pid): return
        try:
            if os.name == "nt":
                subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True, timeout=15, check=False)
                deadline = time.monotonic() + 5
                while RuntimeManager._alive(pid) and time.monotonic() < deadline: time.sleep(0.1)
            else: os.kill(pid, 15)
        except (OSError, subprocess.SubprocessError): pass

    def stop(self, instance_id: str) -> dict[str, Any]:
        state = self._load(); instance = next((item for item in state["instances"] if item["instanceId"] == instance_id), None)
        if not instance: raise AidcError(f"浏览器实例不存在：{instance_id}", 3, "INSTANCE_NOT_FOUND")
        for process in reversed(instance.get("processes", [])): self._stop_pid(int(process["pid"]))
        instance["status"] = "stopped"; instance["stoppedAt"] = utc_now(); instance["stopReason"] = "requested"; self._save(state)
        return self._public(instance)

    def stop_all(self) -> list[dict[str, Any]]:
        running = [item for item in self.status() if item["status"] == "running"]
        return [self.stop(item["instanceId"]) for item in running]
