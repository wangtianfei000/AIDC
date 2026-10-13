from __future__ import annotations

import argparse
import json
import platform
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

from aidc_core import __version__
from aidc_core.paths import absolute_existing, product_root, state_root
from aidc_core.registry import ComponentRegistry
from aidc_core.result import AidcError, CommandResult
from aidc_core.runtime import RuntimeManager
from aidc_core.operations import model3d_export, model3d_publish, modelica, sysml_validate
from aidc_core.dependencies import DependencyManager, RUNTIME_DIRECTORIES
from aidc_core.artifacts import ArtifactRegistry
from aidc_core.integrations import IntegrationManager
from aidc_core.geometry import GeometryBackendRegistry
from mcp.aidc_mcp import TaskRegistry


class AidcArgumentParser(argparse.ArgumentParser):
    def error(self, message: str) -> None:
        raise AidcError(message, 2, "USAGE_ERROR")


def parser() -> argparse.ArgumentParser:
    root = AidcArgumentParser(prog="aidc", description="AIDC AI 工程工具统一入口")
    root.add_argument("--json", action="store_true", dest="global_json", help="输出稳定 JSON 结果")
    commands = root.add_subparsers(dest="domain", required=True)
    commands.add_parser("version")
    commands.add_parser("capabilities")
    doctor = commands.add_parser("doctor"); doctor.add_argument("--report", type=Path)
    deps = commands.add_parser("deps").add_subparsers(dest="action", required=True)
    dep_status = deps.add_parser("status"); dep_status.add_argument("id", nargs="?")
    for action in ("install", "repair"):
        operation = deps.add_parser(action); operation.add_argument("id"); operation.add_argument("--source")
    for action in ("verify", "rollback", "uninstall"): deps.add_parser(action).add_argument("id")
    for domain, label in (("sysml", "FILE"), ("model3d", "PROJECT"), ("modelica", "PROJECT")):
        sub = commands.add_parser(domain).add_subparsers(dest="action", required=True)
        launched = sub.add_parser("launch"); launched.add_argument("--no-browser", action="store_true")
        opened = sub.add_parser("open"); opened.add_argument("target", metavar=label); opened.add_argument("--no-browser", action="store_true")
        if domain == "sysml": sub.add_parser("validate").add_argument("target")
        elif domain == "model3d":
            for action in ("publish", "export-modelica"):
                operation = sub.add_parser(action); operation.add_argument("input"); operation.add_argument("output")
        else:
            validated = sub.add_parser("validate"); validated.add_argument("input"); validated.add_argument("--migrate", action="store_true")
            generated = sub.add_parser("generate"); generated.add_argument("input"); generated.add_argument("output"); generated.add_argument("--profile", choices=("auto", "p2", "p3"), default="auto")
            simulated = sub.add_parser("simulate"); simulated.add_argument("project"); simulated.add_argument("--experiment")
            packaged = sub.add_parser("package"); packaged.add_argument("project"); packaged.add_argument("output"); packaged.add_argument("--include-results", action="store_true")
    freecad = commands.add_parser("freecad").add_subparsers(dest="action", required=True)
    for action in ("open", "check"): freecad.add_parser(action).add_argument("file")
    geometry = commands.add_parser("geometry").add_subparsers(dest="action", required=True)
    geometry.add_parser("backends")
    geometry_validate = geometry.add_parser("validate")
    geometry_validate.add_argument("--backend", choices=("freecad-occ",), default="freecad-occ")
    geometry_validate.add_argument("--report", type=Path)
    integration = commands.add_parser("integrate"); integration.add_argument("host", choices=("codex", "cursor", "mcp"))
    integration_action = integration.add_mutually_exclusive_group()
    integration_action.add_argument("--check", action="store_true")
    integration_action.add_argument("--uninstall", action="store_true")
    integration_action.add_argument("--restore", action="store_true")
    integration.add_argument("--config", type=Path, help="覆盖宿主配置路径；默认使用用户级配置")
    browser = commands.add_parser("browser").add_subparsers(dest="action", required=True)
    browser.add_parser("list"); browser.add_parser("status")
    stopped = browser.add_parser("stop"); stopped.add_argument("instance_id", nargs="?"); stopped.add_argument("--all", action="store_true")
    task = commands.add_parser("task").add_subparsers(dest="action", required=True)
    task_status = task.add_parser("status"); task_status.add_argument("task_id")
    task_cancel = task.add_parser("cancel"); task_cancel.add_argument("task_id")
    artifacts = commands.add_parser("artifacts").add_subparsers(dest="action", required=True)
    artifact_list = artifacts.add_parser("list"); artifact_list.add_argument("--task-id")
    artifact_get = artifacts.add_parser("get"); artifact_get.add_argument("artifact_id")
    return root


def _doctor(registry: ComponentRegistry) -> dict[str, Any]:
    root = registry.root
    assets = {
        "sysml": (root / "components/sysml/browser/index.html") if (root / "components/sysml/browser/index.html").is_file() else root / "sysml-browser/dist/index.html",
        "model3d": (root / "components/model3d/browser/index.html") if (root / "components/model3d/browser/index.html").is_file() else root / "model3d-viewer/apps/browser/dist/index.html",
        "modelica": (root / "components/modelica/browser/index.html") if (root / "components/modelica/browser/index.html").is_file() else root / "modelica-browser/apps/web/dist/index.html",
    }
    dependencies = registry.dependency_status()
    return {
        "healthy": all(item["available"] for item in dependencies if item["id"] == "sysml-parser-runtime") and all(path.is_file() for path in assets.values()),
        "productRoot": str(root), "stateRoot": str(state_root()),
        "platform": {"system": platform.system(), "release": platform.release(), "architecture": platform.machine()},
        "python": {"version": platform.python_version(), "executable": str(Path(sys.executable).resolve())},
        "node": {"available": registry.executable("node") is not None, "path": registry.executable("node")},
        "npm": {"available": registry.executable("npm.cmd" if sys.platform == "win32" else "npm") is not None},
        "browserAssets": {name: {"available": path.is_file(), "path": str(path.resolve())} for name, path in assets.items()},
        "dependencies": dependencies,
    }


def execute(args: argparse.Namespace) -> CommandResult:
    registry = ComponentRegistry(); manager = RuntimeManager()
    if args.domain == "version":
        return CommandResult("system.version", f"AIDC CLI {__version__}", {"cliVersion": __version__, "contractVersion": "0.6.0", "pythonVersion": platform.python_version()})
    if args.domain == "capabilities":
        return CommandResult("system.capabilities", f"发现 {len(registry.capabilities())} 项能力", {"capabilities": registry.capabilities(), "components": registry.components()})
    if args.domain == "doctor":
        data = _doctor(registry)
        result = CommandResult("system.doctor", "系统诊断通过" if data["healthy"] else "系统诊断发现缺失组件", data)
        if args.report:
            path = args.report.expanduser().resolve(); path.parent.mkdir(parents=True, exist_ok=True); path.write_text(json.dumps(result.finish(), ensure_ascii=False, indent=2), encoding="utf-8")
            result.artifacts.append({"kind": "file", "path": str(path)})
        return result
    if args.domain == "deps" and args.action == "status":
        dependency_manager = DependencyManager(registry.root)
        data = registry.dependency_status(args.id)
        if args.id and not data: raise AidcError(f"未知依赖：{args.id}", 3, "DEPENDENCY_UNKNOWN")
        for item in data:
            if item["id"] in RUNTIME_DIRECTORIES:
                managed = dependency_manager.status(item["id"])
                item.update({"managed": managed["installed"], "healthy": managed["healthy"] if managed["installed"] else item["available"], "issues": managed["issues"], "receipt": managed["receipt"]})
        return CommandResult("deps.status", f"检查了 {len(data)} 项依赖", {"dependencies": data})
    if args.domain == "deps" and args.action in {"install", "repair", "verify", "rollback", "uninstall"}:
        dependency_manager = DependencyManager(registry.root)
        if args.action == "install": data = dependency_manager.install(args.id, args.source)
        elif args.action == "repair": data = dependency_manager.repair(args.id, args.source)
        elif args.action == "verify": data = dependency_manager.status(args.id)
        elif args.action == "rollback": data = dependency_manager.rollback(args.id)
        else: data = dependency_manager.uninstall(args.id)
        message = {"install": "依赖包安装完成", "repair": "依赖包修复完成", "verify": "依赖包校验完成", "rollback": "依赖包回滚完成", "uninstall": "依赖包卸载完成"}[args.action]
        if args.action == "verify" and not data["healthy"]: raise AidcError(f"依赖包不健康：{', '.join(data['issues'])}", 6, "DEPENDENCY_UNHEALTHY")
        return CommandResult(f"deps.{args.action}", message, data, artifacts=[{"kind": "directory", "path": data["path"]}] if data.get("path") else [])
    if args.domain in {"sysml", "model3d", "modelica"} and args.action in {"launch", "open"}:
        target = absolute_existing(args.target, "工程或模型") if args.action == "open" else None
        data = manager.start(args.domain, target, launch_browser=not args.no_browser)
        return CommandResult(f"{args.domain}.{args.action}", f"已打开 {args.domain} 浏览器", data, task={"taskId": data["taskId"] if not data["reused"] else "reused-" + data["instanceId"], "state": "succeeded", "progress": 1})
    if args.domain == "sysml" and args.action == "validate":
        source = absolute_existing(args.target, "SysML 文件"); data = sysml_validate(registry.root, source)
        if not data["passed"]: raise AidcError("SysML 校验失败", 8, "VALIDATION_FAILED", str(source))
        return CommandResult("sysml.validate", "SysML 校验通过", data, diagnostics=data["diagnostics"])
    if args.domain == "model3d" and args.action == "publish":
        source = absolute_existing(args.input, "发布配置"); output = Path(args.output).expanduser().resolve(); data = model3d_publish(registry.root, source, output)
        return CommandResult("model3d.publish", "三维模型发布完成", data, artifacts=[{"kind": "directory", "path": str(output)}])
    if args.domain == "model3d" and args.action == "export-modelica":
        source = absolute_existing(args.input, "三维工程"); output = Path(args.output).expanduser().resolve(); data = model3d_export(registry.root, source, output)
        return CommandResult("model3d.export-modelica", "Modelica 装配输入包导出完成", data, artifacts=[{"kind": "directory", "path": str(output)}])
    if args.domain == "modelica" and args.action in {"validate", "generate", "package"}:
        if args.action == "validate":
            arguments = [str(absolute_existing(args.input, "模型输入"))]
            if args.migrate: arguments.append("--migrate")
        elif args.action == "generate":
            arguments = [str(absolute_existing(args.input, "模型输入")), str(Path(args.output).expanduser().resolve()), "--profile", args.profile]
        else:
            arguments = [str(absolute_existing(args.project, "Modelica 工程")), str(Path(args.output).expanduser().resolve())]
            if args.include_results: arguments.append("--include-results")
        data = modelica(registry.root, args.action, arguments)
        artifact = data.get("output")
        return CommandResult(f"modelica.{args.action}", f"Modelica {args.action} 完成", data, artifacts=[{"kind": "path", "path": artifact}] if artifact else [])
    if args.domain == "modelica" and args.action == "simulate":
        dependency = registry.dependency_status("openmodelica")
        if not dependency or not dependency[0]["available"]: raise AidcError("Modelica 仿真需要 P3 OpenModelica Runtime Pack", 4, "DEPENDENCY_MISSING")
        arguments = [str(absolute_existing(args.project, "Modelica 工程"))]
        if args.experiment: arguments.extend(["--experiment", args.experiment])
        data = modelica(registry.root, "simulate", arguments)
        return CommandResult("modelica.simulate", "Modelica 仿真完成", data, artifacts=[{"kind": "directory", "path": data["output"]}])
    if args.domain == "freecad" and args.action in {"open", "check"}:
        source = absolute_existing(args.file, "CAD 文件")
        status = registry.dependency_status("freecad"); command_exe = Path(status[0]["path"]) if status and status[0]["path"] else None
        if not command_exe: raise AidcError("未找到受管 FreeCAD", 4, "DEPENDENCY_MISSING")
        if args.action == "open":
            gui = command_exe.with_name("FreeCAD.exe")
            if not gui.is_file(): raise AidcError("未找到 FreeCAD 图形入口", 4, "DEPENDENCY_MISSING")
            process = subprocess.Popen([str(gui), str(source)], cwd=gui.parent, creationflags=subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0)
            return CommandResult("freecad.open", "已打开 FreeCAD", {"file": str(source), "executable": str(gui), "pid": process.pid})
        env = os.environ.copy(); env["AIDC_FREECAD_INPUT"] = str(source)
        completed = subprocess.run([str(command_exe), "-c", "import FreeCAD as App,os;d=App.openDocument(os.environ['AIDC_FREECAD_INPUT']);print(len(d.Objects));App.closeDocument(d.Name)"], cwd=command_exe.parent, env=env, capture_output=True, text=True, errors="replace", timeout=120, check=False)
        if completed.returncode: raise AidcError(completed.stderr or completed.stdout, 6, "CAD_CHECK_FAILED", str(source))
        return CommandResult("freecad.check", "FreeCAD 文件检查通过", {"file": str(source), "output": completed.stdout.strip()})
    if args.domain == "geometry" and args.action == "backends":
        backends = GeometryBackendRegistry(registry.root).list()
        return CommandResult("geometry.backends", f"发现 {len(backends)} 个几何后端", {"setting": "geometry.backend", "default": "freecad-occ", "backends": backends})
    if args.domain == "geometry" and args.action == "validate":
        backend = GeometryBackendRegistry(registry.root).get(args.backend)
        data = backend.validate()
        artifacts = []
        if args.report:
            path = args.report.expanduser().resolve()
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
            artifacts.append({"kind": "file", "path": str(path)})
        if data["state"] == "unavailable":
            raise AidcError(f"几何后端不可用：{args.backend}（{data['probe']['error']}）", 4, "DEPENDENCY_MISSING")
        if data["state"] != "passed":
            return CommandResult("geometry.validate", f"几何后端能力验证失败：{args.backend}", data, status="failed", exit_code=8, diagnostics=[{"code": "GEOMETRY_VALIDATION_FAILED", "severity": "error", "message": item["message"], "path": None, "details": item.get("metrics", {})} for item in data["checks"] if not item["passed"]], artifacts=artifacts)
        return CommandResult("geometry.validate", f"几何后端能力验证通过：{args.backend}", data, artifacts=artifacts)
    if args.domain == "integrate":
        integrations = IntegrationManager(registry.root)
        if args.check:
            data = integrations.check(args.host, args.config)
            message = f"{args.host} 接入兼容性检查通过"
        elif args.uninstall:
            data = integrations.uninstall(args.host, args.config)
            message = f"{args.host} 接入已卸载"
        elif args.restore:
            data = integrations.restore(args.host)
            message = f"{args.host} 接入备份已恢复"
        else:
            data = integrations.install(args.host, args.config)
            message = f"{args.host} 接入已安装"
        artifacts = [{"kind": "file", "path": data["configPath"]}] if Path(data["configPath"]).is_file() else []
        return CommandResult("integrate.host", message, data, artifacts=artifacts)
    if args.domain == "browser" and args.action in {"list", "status"}:
        data = manager.status(); return CommandResult("browser.status", f"共 {len(data)} 个浏览器实例", {"instances": data})
    if args.domain == "browser" and args.action == "stop":
        if args.all: data = manager.stop_all(); return CommandResult("browser.stop", f"已停止 {len(data)} 个实例", {"instances": data})
        if not args.instance_id: raise AidcError("请提供 INSTANCE_ID 或 --all", 2, "USAGE_ERROR")
        return CommandResult("browser.stop", "浏览器实例已停止", manager.stop(args.instance_id))
    if args.domain == "task" and args.action == "status":
        try: data = TaskRegistry(registry.root).get(args.task_id)
        except AidcError as exc:
            if exc.code != "TASK_NOT_FOUND": raise
            data = manager.task(args.task_id)
            if not data: raise AidcError(f"任务不存在：{args.task_id}", 3, "TASK_NOT_FOUND")
        return CommandResult("task.status", f"任务状态：{data['state']}", data, task={"taskId": data["taskId"], "state": data["state"], "progress": 1 if data["state"] == "succeeded" else None})
    if args.domain == "task" and args.action == "cancel":
        try: data = TaskRegistry(registry.root).cancel(args.task_id)
        except AidcError as exc:
            if exc.code != "TASK_NOT_FOUND": raise
            data = manager.cancel_task(args.task_id)
        return CommandResult("task.cancel", "任务已取消", data, task={"taskId": data["taskId"], "state": "cancelled", "progress": None})
    if args.domain == "artifacts" and args.action == "list":
        items = ArtifactRegistry().list(args.task_id)
        return CommandResult("artifacts.list", f"找到 {len(items)} 个产物", {"artifacts": items})
    if args.domain == "artifacts" and args.action == "get":
        item = ArtifactRegistry().get(args.artifact_id)
        return CommandResult("artifacts.get", "产物信息已读取", item, artifacts=[item])
    raise AidcError("命令尚未实现", 5, "CAPABILITY_UNAVAILABLE")


def print_human(document: dict[str, Any]) -> None:
    print(document["message"])
    data = document.get("data")
    if isinstance(data, dict):
        if "instances" in data:
            for item in data["instances"]: print(f"{item['instanceId']}  {item['kind']}  {item['status']}  {item['url']}")
        elif "dependencies" in data:
            for item in data["dependencies"]: print(f"{item['id']}: {'可用' if item['available'] else '缺失'}  {item['version']}  {item['path'] or '-'}")
        elif "launchUrl" in data: print(data["launchUrl"])


def main(argv: list[str] | None = None) -> int:
    raw = list(sys.argv[1:] if argv is None else argv)
    as_json = "--json" in raw
    command = ".".join(item for item in raw if not item.startswith("-") and item not in {"true", "false"})
    try:
        args = parser().parse_args([item for item in raw if item != "--json"])
        command = ".".join(filter(None, [getattr(args, "domain", None), getattr(args, "action", None)]))
        result = execute(args)
    except AidcError as exc:
        result = CommandResult(command or "system.command", str(exc), status="failed", exit_code=exc.exit_code, diagnostics=[{"code": exc.code, "severity": "error", "message": str(exc), "path": exc.path, "details": {}}])
    except Exception as exc:
        result = CommandResult(command or "system.command", "AIDC 内部错误", status="failed", exit_code=10, diagnostics=[{"code": "INTERNAL_ERROR", "severity": "error", "message": str(exc), "path": None, "details": {}}])
    document = result.finish()
    recorded = ArtifactRegistry().record(document)
    if recorded: document["artifacts"] = recorded
    if as_json: print(json.dumps(document, ensure_ascii=False))
    else: print_human(document)
    return result.exit_code


if __name__ == "__main__": raise SystemExit(main())
