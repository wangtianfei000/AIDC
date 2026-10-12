from __future__ import annotations

import json
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from aidc_core.runtime import RuntimeManager


def request(url: str) -> tuple[int, bytes]:
    try:
        with urllib.request.urlopen(url, timeout=10) as response: return response.status, response.read()
    except urllib.error.HTTPError as error: return error.code, error.read()


def main() -> int:
    targets = {
        "sysml": ROOT / "samples" / "jingang-10axis-system-design" / "architecture.sysml",
        "model3d": ROOT / "samples" / "jingang-10axis-system-design" / "model3d",
        "modelica": ROOT / "samples" / "modelica-t2-single-joint",
    }
    with tempfile.TemporaryDirectory(prefix="aidc-p1-") as directory:
        manager = RuntimeManager(ROOT, Path(directory))
        opened: list[dict] = []
        try:
            for kind, target in targets.items():
                instance = manager.start(kind, target, launch_browser=False); opened.append(instance)
                status, _ = request(instance["url"])
                assert status == 401, (kind, status)
                status, payload = request(instance["launchUrl"])
                assert status == 200 and payload, (kind, status)
                reused = manager.start(kind, target, launch_browser=False)
                assert reused["reused"] and reused["instanceId"] == instance["instanceId"]
            assert len({item["port"] for item in opened}) == 3
            running = [item for item in manager.status() if item["status"] == "running"]
            assert len(running) == 3
            commands = json.dumps(running, ensure_ascii=False)
            assert "AIDC_SESSION_TOKEN" not in commands and "?token=" not in commands
        finally:
            stopped = manager.stop_all()
        assert len(stopped) == 3
        assert not [item for item in manager.status() if item["status"] == "running"]
        print(json.dumps({"passed": True, "browsers": [{"kind": item["kind"], "port": item["port"], "processes": len(item["processes"])} for item in opened], "reused": 3, "stopped": 3}, ensure_ascii=False))
    return 0


if __name__ == "__main__": raise SystemExit(main())
