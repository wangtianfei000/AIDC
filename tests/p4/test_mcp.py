from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from mcp.aidc_mcp import TaskRegistry, _arguments


class Client:
    def __init__(self, state: Path):
        environment = os.environ.copy(); environment["LOCALAPPDATA"] = str(state); environment["PYTHONUTF8"] = "1"
        self.process = subprocess.Popen([sys.executable, str(ROOT / "aidc_mcp.py")], cwd=ROOT, env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding="utf-8")
        self.identifier = 0

    def request(self, method: str, params: dict | None = None) -> dict:
        self.identifier += 1; request = {"jsonrpc": "2.0", "id": self.identifier, "method": method}
        if params is not None: request["params"] = params
        assert self.process.stdin and self.process.stdout
        self.process.stdin.write(json.dumps(request) + "\n"); self.process.stdin.flush()
        return json.loads(self.process.stdout.readline())

    def close(self) -> None:
        if self.process.stdin: self.process.stdin.close()
        self.process.wait(timeout=10)
        if self.process.stdout: self.process.stdout.close()
        if self.process.stderr: self.process.stderr.close()


class McpProtocolTests(unittest.TestCase):
    def test_protocol_tools_tasks_artifacts_and_skill_resources(self):
        with tempfile.TemporaryDirectory(prefix="aidc-p4-") as directory:
            root = Path(directory); client = Client(root)
            try:
                initialized = client.request("initialize", {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "test", "version": "1"}})
                self.assertEqual(initialized["result"]["serverInfo"]["version"], "0.4.0")
                tools = client.request("tools/list")["result"]["tools"]
                self.assertEqual(len(tools), 17); self.assertIn("aidc_artifacts_list", {item["name"] for item in tools})
                resources = client.request("resources/list")["result"]["resources"]
                self.assertEqual(len(resources), 4)
                skill = client.request("resources/read", {"uri": "aidc-skill://robot-modelica-model-development/SKILL.md"})
                self.assertIn("aidc_modelica_generate", skill["result"]["contents"][0]["text"])
                capabilities = client.request("tools/call", {"name": "aidc_system_capabilities", "arguments": {}})["result"]
                self.assertFalse(capabilities["isError"]); self.assertEqual(capabilities["structuredContent"]["command"], "system.capabilities")
                invalid = client.request("tools/call", {"name": "aidc_sysml_validate", "arguments": {}})["result"]
                self.assertTrue(invalid["isError"]); self.assertEqual(invalid["structuredContent"]["diagnostics"][0]["code"], "MCP_INPUT_INVALID")
                report = root / "doctor.json"
                accepted = client.request("tools/call", {"name": "aidc_system_doctor", "arguments": {"reportPath": str(report)}})["result"]["structuredContent"]
                task_id = accepted["task"]["taskId"]
                task = None
                for _ in range(100):
                    task_result = client.request("tools/call", {"name": "aidc_task_status", "arguments": {"taskId": task_id}})["result"]["structuredContent"]
                    task = task_result["data"]
                    if task["state"] != "running": break
                    time.sleep(.05)
                self.assertEqual(task["state"], "succeeded"); self.assertTrue(report.is_file())
                environment = os.environ.copy(); environment["LOCALAPPDATA"] = str(root)
                cli_status = subprocess.run([sys.executable, str(ROOT / "aidc.py"), "task", "status", task_id, "--json"], cwd=ROOT, env=environment, capture_output=True, text=True, encoding="utf-8", timeout=30, check=False)
                self.assertEqual(cli_status.returncode, 0, cli_status.stderr)
                self.assertEqual(json.loads(cli_status.stdout)["data"]["state"], "succeeded")
                artifacts = client.request("tools/call", {"name": "aidc_artifacts_list", "arguments": {"taskId": task_id}})["result"]["structuredContent"]["data"]["artifacts"]
                self.assertEqual(len(artifacts), 1); self.assertEqual(artifacts[0]["type"], "aidc.diagnostic.report")
                artifact = client.request("tools/call", {"name": "aidc_artifacts_get", "arguments": {"artifactId": artifacts[0]["artifactId"]}})["result"]["structuredContent"]["data"]
                self.assertEqual(artifact["sha256"], artifacts[0]["sha256"])
            finally: client.close()

    def test_task_cancellation_stops_worker(self):
        with tempfile.TemporaryDirectory(prefix="aidc-p4-cancel-") as directory:
            root = Path(directory); product = root / "product"; product.mkdir()
            (product / "aidc.py").write_text("import time; time.sleep(30)", encoding="utf-8")
            with patch.dict(os.environ, {"LOCALAPPDATA": str(root)}):
                tasks = TaskRegistry(product)
                task = tasks.start({"name": "test_long", "capability": "test.long"}, [])
                cancelled = tasks.cancel(task["taskId"])
                self.assertEqual(cancelled["state"], "cancelled")

    def test_modelica_options_are_forwarded(self):
        self.assertEqual(_arguments("modelica.validate", {"path": "x", "migrate": True})[-1], "--migrate")
        self.assertEqual(_arguments("modelica.generate", {"inputPath": "x", "outputPath": "y", "profile": "p3"})[-2:], ["--profile", "p3"])


if __name__ == "__main__": unittest.main()
