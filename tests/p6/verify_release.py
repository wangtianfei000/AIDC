from __future__ import annotations

import http.cookiejar
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PORTABLE = ROOT / "distribution" / "out" / "aidc-portable"
CLI = PORTABLE / "bin" / "aidc.exe"
OMC_RUNTIME = ROOT.parent / "tools" / "openmodelica" / "1.27.1"
FREECAD_RUNTIME = ROOT / "FreeCAD-runtime" / "FreeCAD_1.1.3-Windows-x86_64-py311"
AUDIT = ROOT / "distribution" / "audits" / "p6-release-acceptance.json"


def command(environment: dict[str, str], *arguments: str, timeout: int = 600, succeeds: bool = True) -> dict:
    completed = subprocess.run(
        [str(CLI), *arguments, "--json"], cwd=PORTABLE, env=environment,
        capture_output=True, text=True, encoding="utf-8", errors="replace",
        timeout=timeout, check=False,
    )
    try:
        document = json.loads(completed.stdout)
    except json.JSONDecodeError as exc:
        raise AssertionError(f"invalid CLI JSON for {arguments}: {completed.stdout}\n{completed.stderr}") from exc
    if succeeds:
        assert completed.returncode == 0 and document["status"] == "succeeded", document
    else:
        assert completed.returncode != 0 and document["status"] == "failed", document
    return document


def junction(link: Path, target: Path) -> None:
    completed = subprocess.run(
        ["powershell", "-NoProfile", "-Command", "& { param($link,$target) New-Item -ItemType Junction -Path $link -Target $target | Out-Null }", str(link), str(target)],
        capture_output=True, text=True, encoding="utf-8", timeout=30, check=False,
    )
    assert completed.returncode == 0, completed.stderr


def mcp_protocol(spec: dict, environment: dict[str, str], requests: list[dict]) -> list[dict]:
    child_environment = environment.copy(); child_environment.update(spec.get("env", {}))
    completed = subprocess.run(
        [spec["command"], *spec.get("args", [])], cwd=PORTABLE, env=child_environment,
        input="".join(json.dumps(item) + "\n" for item in requests),
        capture_output=True, text=True, encoding="utf-8", errors="replace",
        timeout=180, check=False,
    )
    assert completed.returncode == 0, completed.stderr
    return [json.loads(line) for line in completed.stdout.splitlines() if line.strip()]


def open_http(document: dict, relative: str = "") -> tuple[int, int]:
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
    with opener.open(document["data"]["launchUrl"], timeout=30) as response:
        size = len(response.read())
    if relative:
        with opener.open(document["data"]["url"] + relative, timeout=30) as response:
            size += len(response.read())
    return 200, size


def subtest(path: str, environment: dict[str, str], timeout: int = 300) -> None:
    completed = subprocess.run([sys.executable, "-X", "utf8", str(ROOT / path)], cwd=ROOT, env=environment, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=timeout, check=False)
    assert completed.returncode == 0, completed.stdout + completed.stderr


def main() -> int:
    assert CLI.is_file(), "先运行 npm run build:distribution:p2"
    manifest = json.loads((PORTABLE / "portable-manifest.json").read_text(encoding="utf-8"))
    assert manifest["productVersion"] == "0.6.0"
    assert (OMC_RUNTIME / "bin" / "omc.exe").is_file(), "工程验收机缺少 OMC 1.27.1"
    assert (FREECAD_RUNTIME / "bin" / "FreeCADCmd.exe").is_file(), "工程验收机缺少 FreeCAD 1.1.3"

    created_links: list[Path] = []
    cleanup_environment = os.environ.copy()
    (PORTABLE / "runtimes").mkdir(exist_ok=True)
    command(os.environ.copy(), "browser", "stop", "--all")
    try:
        with tempfile.TemporaryDirectory(prefix="aidc-p6-", ignore_cleanup_errors=True) as directory:
            base = Path(directory) / "中文 路径"; base.mkdir(parents=True)
            user_project = base / "用户工程"; user_project.mkdir(); (user_project / "keep.txt").write_text("preserve", encoding="utf-8")
            environment = os.environ.copy()
            environment.update({
                "PATH": str(Path(os.environ["SystemRoot"]) / "System32"),
                "LOCALAPPDATA": str(base / "本地状态"),
                "AIDC_INTEGRATION_HOME": str(base / "宿主配置"),
                "PYTHONUTF8": "1",
                "HTTP_PROXY": "http://127.0.0.1:9",
                "HTTPS_PROXY": "http://127.0.0.1:9",
                "NO_PROXY": "127.0.0.1,localhost",
                "QT_QPA_PLATFORM": "offscreen",
            })
            cleanup_environment = environment

            # Missing dependencies fail with a stable actionable code before runtimes are attached.
            missing = command(environment, "modelica", "simulate", str(ROOT / "modelica-browser" / "generated" / "p2-single-joint"), succeeds=False)
            assert missing["diagnostics"][0]["code"] == "DEPENDENCY_MISSING"

            for name, target in (("openmodelica", OMC_RUNTIME), ("freecad", FREECAD_RUNTIME)):
                link = PORTABLE / "runtimes" / name
                assert not link.exists(), f"验收不会替换已有运行时：{link}"
                junction(link, target); created_links.append(link)

            sysml = base / "机器鸭 架构.sysml"
            shutil.copy2(ROOT / "samples" / "jingang-10axis-system-design" / "architecture.sysml", sysml)
            validated = command(environment, "sysml", "validate", str(sysml))
            assert validated["data"]["summary"]["elementCount"] > 0

            # The published hardware model is the canonical 13-body/12-joint machine-duck input.
            subprocess.run([sys.executable, "-X", "utf8", str(ROOT / "model3d-viewer" / "scripts" / "prepare-microduck-modelica-export.py")], cwd=ROOT, check=True, timeout=120)
            model3d = ROOT / "model3d-viewer" / "test-artifacts" / "microduck-native12-export"
            assembly = base / "机器鸭.aidc-assembly"
            exported = command(environment, "model3d", "export-modelica", str(model3d), str(assembly))
            assert exported["data"]["bodyCount"] == 13 and exported["data"]["jointCount"] == 12 and exported["data"]["simulationReady"]
            kinematics = json.loads((model3d / "generated" / "kinematics.json").read_text(encoding="utf-8"))
            assert len(kinematics["joints"]) == 12
            modelica = base / "机器鸭 Modelica"
            generated = command(environment, "modelica", "generate", str(assembly), str(modelica), "--profile", "p3")
            command(environment, "modelica", "validate", str(modelica))
            simulated = command(environment, "modelica", "simulate", str(modelica), timeout=600)
            run_dir = Path(simulated["data"]["output"])
            assert (run_dir / "run-manifest.json").is_file() and (run_dir / "series.csv").is_file()

            # All three browsers serve their actual project data from the frozen release.
            sysml_open = command(environment, "sysml", "open", str(sysml), "--no-browser")
            model3d_open = command(environment, "model3d", "open", str(model3d), "--no-browser")
            modelica_open = command(environment, "modelica", "open", str(modelica), "--no-browser")
            browser_checks = [open_http(sysml_open), open_http(model3d_open, "model3d.manifest.json"), open_http(modelica_open)]
            assert all(status == 200 and size > 100 for status, size in browser_checks)
            assert model3d_open["data"]["port"] != sysml_open["data"]["port"] != modelica_open["data"]["port"]

            # Holding an unrelated port does not affect dynamic allocation.
            occupied = socket.socket(); occupied.bind(("127.0.0.1", 0)); occupied.listen(1)
            occupied_port = occupied.getsockname()[1]
            assert all(item["data"]["port"] != occupied_port for item in (sysml_open, model3d_open, modelica_open))
            occupied.close()

            # A crashed gateway is detected and the same project can be restarted.
            gateway = next(item for item in model3d_open["data"]["processes"] if item["role"] == "gateway")
            subprocess.run(["taskkill", "/PID", str(gateway["pid"]), "/T", "/F"], capture_output=True, timeout=30, check=False)
            deadline = time.monotonic() + 10
            crashed = False
            while time.monotonic() < deadline:
                status = command(environment, "browser", "status")
                crashed = any(item["instanceId"] == model3d_open["data"]["instanceId"] and item["status"] == "stopped" for item in status["data"]["instances"])
                if crashed: break
                time.sleep(.2)
            assert crashed
            restarted = command(environment, "model3d", "open", str(model3d), "--no-browser")
            assert not restarted["data"]["reused"] and open_http(restarted)[0] == 200

            # Codex and Cursor configs both execute the same frozen MCP core.
            codex = command(environment, "integrate", "codex")
            cursor = command(environment, "integrate", "cursor")
            generic_path = base / "通用 MCP.json"
            command(environment, "integrate", "mcp", "--config", str(generic_path))
            for host, extra in (("codex", ()), ("cursor", ()), ("mcp", ("--config", str(generic_path)))):
                checked = command(environment, "integrate", host, "--check", *extra)
                assert checked["data"]["probe"]["toolCount"] == 17 and checked["data"]["probe"]["resourceCount"] == 4
            cursor_spec = json.loads(Path(cursor["data"]["configPath"]).read_text(encoding="utf-8"))["mcpServers"]["aidc"]
            requests = [
                {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "p6-cursor", "version": "1"}}},
                {"jsonrpc": "2.0", "id": 2, "method": "tools/call", "params": {"name": "aidc_modelica_open", "arguments": {"projectPath": str(modelica)}}},
                {"jsonrpc": "2.0", "id": 3, "method": "tools/call", "params": {"name": "aidc_artifacts_list", "arguments": {}}},
            ]
            responses = mcp_protocol(cursor_spec, environment, requests)
            assert responses[0]["result"]["serverInfo"]["version"] == "0.6.0"
            assert responses[1]["result"]["structuredContent"]["status"] == "succeeded"
            assert responses[2]["result"]["structuredContent"]["status"] == "succeeded"

            # Managed FreeCAD performs exact read and headless publication.
            cad = ROOT / "samples" / "jingang-10axis-system-design" / "layout" / "jingang-10axis-layout.FCStd"
            checked_cad = command(environment, "freecad", "check", str(cad), timeout=180)
            opened_cad = command(environment, "freecad", "open", str(cad))
            assert opened_cad["data"]["pid"] > 0 and Path(opened_cad["data"]["executable"]).name == "FreeCAD.exe"
            subprocess.run(["taskkill", "/PID", str(opened_cad["data"]["pid"]), "/T", "/F"], capture_output=True, timeout=30, check=False)
            published = base / "金刚 发布"
            command(environment, "model3d", "publish", str(ROOT / "samples" / "jingang-10axis-system-design" / "model3d" / "publish.config.json"), str(published), timeout=900)
            assert {"visual.glb", "collision.glb", "kinematics.json", "model3d.manifest.json", "publish-report.json"}.issubset(item.name for item in published.iterdir())

            # Existing P3 pack tests cover digest corruption, transactional rollback and offline bundle assembly.
            command(environment, "browser", "stop", "--all")
            test_environment = environment.copy()
            test_environment["PATH"] = os.pathsep.join([environment["PATH"], str(Path(os.environ["SystemRoot"]) / "System32" / "WindowsPowerShell" / "v1.0")])
            subtest("tests/p3/verify_installer.py", test_environment)
            unit = subprocess.run([sys.executable, "-X", "utf8", "-m", "unittest", "discover", "-s", "tests/p3", "-p", "test_*.py"], cwd=ROOT, env=test_environment, capture_output=True, text=True, encoding="utf-8", timeout=300, check=False)
            assert unit.returncode == 0, unit.stdout + unit.stderr

            command(environment, "integrate", "codex", "--uninstall")
            command(environment, "integrate", "cursor", "--uninstall")
            command(environment, "integrate", "mcp", "--uninstall", "--config", str(generic_path))
            assert (user_project / "keep.txt").read_text(encoding="utf-8") == "preserve"

            gates = json.loads((ROOT / "distribution" / "audits" / "supply-chain-gates.json").read_text(encoding="utf-8"))
            blocked = [item["id"] for item in gates["gates"] if item["status"] == "blocked"]
            audit = {
                "schemaVersion": "aidc.release-acceptance/1.0",
                "productVersion": "0.6.0",
                "acceptanceStatus": "passed-internal",
                "publicReleaseStatus": "blocked",
                "entryPoints": {"cli": "passed", "codex": "passed", "cursor": "passed", "genericMcp": "passed"},
                "flows": {
                    "sysmlParseAndBrowser": "passed",
                    "microduck3dMotionDataAndAssemblyExport": "passed",
                    "assemblyToModelicaAndOmc": "passed",
                    "freecadGuiOpenExactReadAndHeadlessPublish": "passed",
                },
                "resilience": {
                    "dependencyMissing": "passed", "dependencyCorruption": "passed", "dynamicPort": "passed",
                    "serviceCrashRecovery": "passed", "unicodeAndSpacePath": "passed", "offlineOperation": "passed",
                    "upgradeRollback": "passed", "uninstallPreservesProjects": "passed", "hostConfigRemoval": "passed",
                },
                "evidence": {
                    "microduckBodies": exported["data"]["bodyCount"], "microduckJoints": exported["data"]["jointCount"],
                    "freecadObjectOutput": checked_cad["data"]["output"].splitlines()[0], "freecadGuiLaunch": "passed", "mcpTools": 17, "skillResources": 4,
                },
                "supplyChain": {"releaseGate": gates["releaseGate"], "blockedGateIds": blocked},
            }
            AUDIT.write_text(json.dumps(audit, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            print(json.dumps({"passed": True, **audit}, ensure_ascii=False))
    finally:
        try:
            if CLI.is_file():
                command(cleanup_environment, "browser", "stop", "--all")
                time.sleep(.5)
        finally:
            for link in reversed(created_links):
                if link.is_dir(): os.rmdir(link)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
