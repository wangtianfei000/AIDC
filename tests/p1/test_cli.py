from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from aidc_core.gateway import GatewayServer
from aidc_core.registry import ComponentRegistry
from aidc_core.result import CommandResult


class P1ContractTests(unittest.TestCase):
    def test_component_discovery(self):
        registry = ComponentRegistry(ROOT)
        self.assertEqual(len(registry.components()), 6)
        self.assertIn("system.doctor", registry.capabilities())

    def test_result_matches_frozen_shape(self):
        result = CommandResult("system.version", "ok", {"version": "test"}).finish()
        self.assertEqual(result["schemaVersion"], "1.0")
        self.assertEqual(result["exitCode"], 0)
        self.assertEqual(result["diagnostics"], [])
        self.assertIn("startedAt", result["meta"])

    def test_cli_json_stdout(self):
        completed = subprocess.run([sys.executable, str(ROOT / "aidc.py"), "version", "--json"], cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=False)
        self.assertEqual(completed.returncode, 0, completed.stderr)
        document = json.loads(completed.stdout)
        self.assertEqual(document["command"], "system.version")
        self.assertEqual(document["data"]["contractVersion"], "0.3.0")
        self.assertEqual(completed.stderr, "")

    def test_invalid_command_has_structured_usage_error(self):
        completed = subprocess.run([sys.executable, str(ROOT / "aidc.py"), "unknown", "--json"], cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=False)
        document = json.loads(completed.stdout)
        self.assertEqual(completed.returncode, 2)
        self.assertEqual(document["diagnostics"][0]["code"], "USAGE_ERROR")

    def test_dependency_install_requires_an_audited_pack(self):
        completed = subprocess.run([sys.executable, str(ROOT / "aidc.py"), "deps", "install", "freecad", "--json"], cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=False)
        document = json.loads(completed.stdout)
        self.assertEqual(completed.returncode, 4)
        self.assertEqual(document["diagnostics"][0]["code"], "PACK_SOURCE_REQUIRED")

    def test_gateway_rejects_missing_token(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); (root / "index.html").write_text("ok", encoding="utf-8")
            server = GatewayServer(("127.0.0.1", 0), "secret", root, None, [], None)
            import threading
            thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
            url = f"http://127.0.0.1:{server.server_address[1]}/"
            with self.assertRaises(urllib.error.HTTPError) as caught: urllib.request.urlopen(url)
            self.assertEqual(caught.exception.code, 401)
            with urllib.request.urlopen(url + "?token=secret") as response: self.assertEqual(response.read(), b"ok")
            server.shutdown(); server.server_close(); thread.join(2)


if __name__ == "__main__": unittest.main()
