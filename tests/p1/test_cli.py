from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import threading
import time
import unittest
import http.cookiejar
import urllib.error
import urllib.request
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from aidc_core.gateway import GatewayServer
from aidc_core.registry import ComponentRegistry
from aidc_core.result import CommandResult
from aidc_core.runtime import RuntimeManager


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
        self.assertEqual(document["data"]["contractVersion"], "0.6.0")
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
            thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
            url = f"http://127.0.0.1:{server.server_address[1]}/"
            try:
                with self.assertRaises(urllib.error.HTTPError) as caught: urllib.request.urlopen(url)
                self.assertEqual(caught.exception.code, 401)
                with urllib.request.urlopen(url + "?token=secret") as response:
                    page = response.read()
                    self.assertIn(b"ok", page)
                    self.assertIn(b"data-aidc-session-client", page)
            finally:
                server.shutdown(); server.server_close(); thread.join(2)

    def test_gateway_stops_when_last_browser_client_leaves(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); (root / "index.html").write_text("<html><body>ok</body></html>", encoding="utf-8")
            server = GatewayServer(("127.0.0.1", 0), "secret", root, None, [], None, disconnect_grace=0.2, heartbeat_timeout=2, registration_timeout=2)
            thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
            base = f"http://127.0.0.1:{server.server_address[1]}"
            opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
            with opener.open(base + "/?token=secret") as response:
                self.assertIn(b"data-aidc-session-client", response.read())

            def client(action: str, client_id: str) -> dict:
                request = urllib.request.Request(
                    base + "/session/client/" + action,
                    data=json.dumps({"clientId": client_id}).encode(),
                    headers={"Content-Type": "application/json"},
                    method="POST",
                )
                with opener.open(request) as response: return json.loads(response.read())

            self.assertEqual(client("register", "first")["clients"], 1)
            self.assertEqual(client("register", "second")["clients"], 2)
            self.assertEqual(client("unregister", "first")["clients"], 1)
            time.sleep(0.4)
            self.assertTrue(thread.is_alive(), "一个页面仍连接时网关不应退出")
            self.assertEqual(client("unregister", "second")["clients"], 0)
            thread.join(3)
            self.assertFalse(thread.is_alive(), "最后一个页面关闭后网关应退出")
            server.server_close()

    @patch("aidc_core.runtime.os.name", "nt")
    @patch("aidc_core.runtime.subprocess.run")
    def test_windows_process_probe_does_not_decode_localized_tasklist_output(self, run):
        run.return_value = subprocess.CompletedProcess([], 0, stdout=b"\xd0\xc5\xcf\xa2: no matching task\r\n", stderr=b"")
        self.assertFalse(RuntimeManager._alive(987654))
        self.assertNotIn("text", run.call_args.kwargs)


if __name__ == "__main__": unittest.main()
