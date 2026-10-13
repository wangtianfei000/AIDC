from __future__ import annotations

import json
import os
import socket
import subprocess
import sys
import tempfile
import time
import urllib.request
from pathlib import Path
from typing import Any

from .registry import ComponentRegistry
from .result import AidcError


def _run_json(command: list[str], *, cwd: Path, env: dict[str, str] | None = None, timeout: int = 300) -> dict[str, Any]:
    completed = subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=timeout, check=False)
    stream = completed.stdout.strip() or completed.stderr.strip()
    try: payload = json.loads(stream)
    except json.JSONDecodeError:
        raise AidcError(stream or f"工具退出码 {completed.returncode}", 6, "TOOL_FAILED")
    if completed.returncode or payload.get("passed") is False:
        raise AidcError(str(payload.get("error") or payload.get("message") or "工具执行失败"), 6, "TOOL_FAILED")
    return payload


def _modelica_command(root: Path) -> tuple[list[str], dict[str, str]]:
    executable = root / "components" / "modelica" / "tools" / "aidc-modelica-tools.exe"
    env = os.environ.copy()
    if executable.is_file():
        env["AIDC_MODELICA_ROOT"] = str(root / "components" / "modelica")
        env["AIDC_ROBOT_IR_ROOT"] = str(root / "components" / "modelica" / "packages" / "robot-ir")
        env["AIDC_ASSEMBLY_PACKAGE_ROOT"] = str(root / "components" / "modelica" / "packages" / "assembly-package")
    omc = next((item.get("path") for item in ComponentRegistry(root).dependency_status("openmodelica") if item.get("path")), None)
    if omc: env["AIDC_OMC"] = omc
    if executable.is_file(): return [str(executable)], env
    return [sys.executable, str(root / "modelica-browser" / "tools" / "aidc_modelica.py")], env


def _modelica_resource_roots(root: Path) -> dict[str, str]:
    """Resolve the same schema roots in source and frozen distributions."""
    packaged = root / "components" / "modelica"
    source = root / "modelica-browser"
    base = packaged if packaged.is_dir() else source
    return {
        "AIDC_MODELICA_ROOT": str(base),
        "AIDC_ROBOT_IR_ROOT": str(base / "packages" / "robot-ir"),
        "AIDC_ASSEMBLY_PACKAGE_ROOT": str(base / "packages" / "assembly-package"),
    }


def modelica(root: Path, action: str, arguments: list[str]) -> dict[str, Any]:
    command, env = _modelica_command(root)
    return _run_json([*command, action, *arguments], cwd=root, env=env, timeout=600)


def sysml_validate(root: Path, source: Path) -> dict[str, Any]:
    runtime = next((Path(item["path"]) for item in ComponentRegistry(root).dependency_status("sysml-parser-runtime") if item["path"]), None)
    if not runtime: raise AidcError("未找到 SysML Parser Runtime 0.8.1", 4, "DEPENDENCY_MISSING")
    with socket.socket() as sock: sock.bind(("127.0.0.1", 0)); port = sock.getsockname()[1]
    process = subprocess.Popen([str(runtime), "--port", str(port)], cwd=runtime.parent, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
    try:
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            try:
                urllib.request.urlopen(f"http://127.0.0.1:{port}/health", timeout=.5).close(); break
            except OSError: time.sleep(.1)
        body = json.dumps({"uri": source.as_uri(), "text": source.read_text(encoding="utf-8"), "requestId": "aidc-cli"}).encode()
        request = urllib.request.Request(f"http://127.0.0.1:{port}/parse", body, {"Content-Type": "application/json"}, method="POST")
        with urllib.request.urlopen(request, timeout=120) as response: payload = json.load(response)
        diagnostics = payload.get("diagnostics", [])
        snapshot = payload.get("snapshot", payload)
        return {
            "passed": not any(item.get("severity") == "error" for item in diagnostics), "source": str(source), "diagnostics": diagnostics,
            "summary": {"elementCount": len(snapshot.get("elements", [])), "relationshipCount": len(snapshot.get("relationships", [])), "schemaVersion": snapshot.get("schemaVersion")},
        }
    finally:
        if process.poll() is None: process.terminate()


def model3d_export(root: Path, project: Path, output: Path) -> dict[str, Any]:
    tool = root / "components" / "model3d" / "tools" / "aidc-model3d-export.exe"
    manifest = project / "generated" / "model3d.manifest.json"
    robot_ir = next((candidate for candidate in (project / "robot-ir", project.parent / "robot-ir", project) if (candidate / "physics.json").is_file()), None)
    if not manifest.is_file() or robot_ir is None: raise AidcError("三维工程缺少 generated/model3d.manifest.json 或 robot-ir", 3, "INPUT_INVALID")
    command = [str(tool)] if tool.is_file() else [sys.executable, str(root / "model3d-viewer" / "tools" / "freecad-publisher" / "src" / "export_modelica_assembly.py")]
    env = os.environ.copy(); env.update(_modelica_resource_roots(root))
    return _run_json([*command, "--manifest", str(manifest), "--robot-ir", str(robot_ir), "--output", str(output)], cwd=root, env=env, timeout=600)


def model3d_publish(root: Path, config: Path, output: Path) -> dict[str, Any]:
    freecad = next((Path(item["path"]) for item in ComponentRegistry(root).dependency_status("freecad") if item["path"]), None)
    if not freecad: raise AidcError("未找到受管 FreeCAD", 4, "DEPENDENCY_MISSING")
    publisher = root / "components" / "model3d" / "tools" / "publish_model.py"
    if not publisher.is_file(): publisher = root / "model3d-viewer" / "tools" / "freecad-publisher" / "src" / "publish_model.py"
    value = json.loads(config.read_text(encoding="utf-8")); value["outputDirectory"] = str(output.resolve())
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".json", prefix=".aidc-publish-", dir=config.parent, delete=False) as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)
        temporary = Path(stream.name)
    env = os.environ.copy(); env.update({"AIDC_MODEL3D_PUBLISHER": str(publisher), "AIDC_MODEL3D_CONFIG": str(temporary)})
    try:
        completed = subprocess.run([str(freecad), "-c", "import os,runpy;runpy.run_path(os.environ['AIDC_MODEL3D_PUBLISHER'],run_name='__main__')"], cwd=root, env=env, capture_output=True, text=True, timeout=900, check=False)
        if completed.returncode: raise AidcError(completed.stderr or completed.stdout, 6, "TOOL_FAILED")
        return {"passed": True, "output": str(output.resolve()), "report": str((output / "publish-report.json").resolve())}
    finally: temporary.unlink(missing_ok=True)
