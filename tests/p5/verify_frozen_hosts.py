from __future__ import annotations

import json
import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PORTABLE = ROOT / "distribution" / "out" / "aidc-portable"
CLI = PORTABLE / "bin" / "aidc.exe"


def run_cli(environment: dict[str, str], *arguments: str) -> dict:
    completed = subprocess.run([str(CLI), *arguments, "--json"], cwd=PORTABLE, env=environment, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=90, check=False)
    assert completed.returncode == 0, completed.stderr or completed.stdout
    return json.loads(completed.stdout)


def protocol(spec: dict, environment: dict[str, str], requests: list[dict]) -> list[dict]:
    child_environment = environment.copy(); child_environment.update(spec.get("env", {}))
    completed = subprocess.run([spec["command"], *spec.get("args", [])], cwd=PORTABLE, env=child_environment, input="".join(json.dumps(item) + "\n" for item in requests), capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=90, check=False)
    assert completed.returncode == 0, completed.stderr
    return [json.loads(line) for line in completed.stdout.splitlines() if line.strip()]


def main() -> int:
    assert CLI.is_file(), f"missing frozen CLI: {CLI}"
    with tempfile.TemporaryDirectory(prefix="aidc-p5-frozen-") as directory:
        base = Path(directory); home = base / "home"; local = base / "local"
        environment = os.environ.copy(); environment.update({"AIDC_INTEGRATION_HOME": str(home), "LOCALAPPDATA": str(local), "PYTHONUTF8": "1", "PATH": str(Path(os.environ["WINDIR"]) / "System32")})
        codex = run_cli(environment, "integrate", "codex")
        cursor = run_cli(environment, "integrate", "cursor")
        generic_path = base / "generic-mcp.json"
        generic = run_cli(environment, "integrate", "mcp", "--config", str(generic_path))
        for host in ("codex", "cursor", "mcp"):
            arguments = ["integrate", host, "--check"]
            if host == "mcp": arguments += ["--config", str(generic_path)]
            checked = run_cli(environment, *arguments)
            assert checked["data"]["probe"]["toolCount"] == 17
            assert checked["data"]["probe"]["resourceCount"] == 4
            assert checked["data"]["probe"]["coreCallPassed"]
        cursor_spec = json.loads(Path(cursor["data"]["configPath"]).read_text(encoding="utf-8"))["mcpServers"]["aidc"]
        generic_spec = json.loads(generic_path.read_text(encoding="utf-8"))["mcpServers"]["aidc"]
        assert cursor_spec == generic_spec

        sample = ROOT / "samples" / "VehicleMode.sysml"
        requests = [
            {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "p5-host", "version": "1"}}},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
            {"jsonrpc": "2.0", "id": 3, "method": "tools/call", "params": {"name": "aidc_sysml_open", "arguments": {"path": str(sample)}}},
            {"jsonrpc": "2.0", "id": 4, "method": "tools/call", "params": {"name": "aidc_artifacts_list", "arguments": {}}},
        ]
        responses = protocol(cursor_spec, environment, requests)
        assert len(responses[1]["result"]["tools"]) == 17
        opened = responses[2]["result"]["structuredContent"]
        assert opened["status"] == "succeeded" and opened["data"]["launchUrl"]
        artifacts = responses[3]["result"]["structuredContent"]["data"]["artifacts"]
        assert any(item["type"] == "aidc.browser.session" for item in artifacts)
        run_cli(environment, "browser", "stop", "--all")

        run_cli(environment, "integrate", "codex", "--uninstall")
        run_cli(environment, "integrate", "cursor", "--uninstall")
        run_cli(environment, "integrate", "mcp", "--uninstall", "--config", str(generic_path))
        print(json.dumps({"passed": True, "hosts": ["codex", "cursor", "mcp"], "toolCount": 17, "skillCount": 4, "browserOpened": True, "artifactReturned": True, "pathIsolation": True}, ensure_ascii=False))
    return 0


if __name__ == "__main__": raise SystemExit(main())
