from __future__ import annotations

import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MCP = ROOT / "distribution" / "out" / "aidc-portable" / "bin" / "aidc-mcp.exe"


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="aidc-p4-frozen-") as directory:
        environment = os.environ.copy(); environment["LOCALAPPDATA"] = directory; environment["PATH"] = str(Path(os.environ["SystemRoot"]) / "System32")
        requests = [
            {"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"acceptance","version":"1"}}},
            {"jsonrpc":"2.0","id":2,"method":"tools/list"},
            {"jsonrpc":"2.0","id":3,"method":"resources/list"},
            {"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"aidc_system_capabilities","arguments":{}}},
        ]
        completed = subprocess.run([str(MCP)], cwd=MCP.parents[2], env=environment, input="".join(json.dumps(item)+"\n" for item in requests), capture_output=True, text=True, encoding="utf-8", timeout=60, check=False)
        assert completed.returncode == 0, completed.stderr
        responses = [json.loads(line) for line in completed.stdout.splitlines()]
        assert responses[0]["result"]["serverInfo"]["version"] == "0.4.0"
        assert len(responses[1]["result"]["tools"]) == 17 and len(responses[2]["result"]["resources"]) == 4
        assert responses[3]["result"]["structuredContent"]["status"] == "succeeded"
        print(json.dumps({"passed":True,"frozenMcp":str(MCP),"toolCount":17,"skillResources":4,"pathIsolation":True},ensure_ascii=False))
    return 0


if __name__ == "__main__": raise SystemExit(main())
