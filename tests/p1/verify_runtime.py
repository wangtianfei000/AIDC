from __future__ import annotations

import json
import shutil
import sys
import tempfile
import http.cookiejar
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
    with tempfile.TemporaryDirectory(prefix="aidc-p1-") as directory:
        model3d = Path(directory) / "model3d"
        generated = model3d / "generated"; generated.mkdir(parents=True)
        shutil.copy2(ROOT / "model3d-viewer" / "legacy" / "sysml-public-model3d" / "jingang" / "model3d.manifest.json", generated / "model3d.manifest.json")
        targets = {
            "sysml": ROOT / "samples" / "jingang-10axis-system-design" / "architecture.sysml",
            "model3d": model3d,
            "modelica": ROOT / "samples" / "modelica-t2-single-joint",
        }
        manager = RuntimeManager(ROOT, Path(directory))
        opened: list[dict] = []
        clients: list[urllib.request.OpenerDirector] = []
        try:
            for kind, target in targets.items():
                instance = manager.start(kind, target, launch_browser=False); opened.append(instance)
                status, _ = request(instance["url"])
                assert status == 401, (kind, status)
                opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar())); clients.append(opener)
                with opener.open(instance["launchUrl"], timeout=10) as response:
                    assert response.status == 200 and response.read(), (kind, response.status)
                registration = urllib.request.Request(
                    instance["url"] + "session/client/register",
                    data=json.dumps({"clientId": f"p1-{kind}"}).encode(),
                    headers={"Content-Type": "application/json"},
                    method="POST",
                )
                with opener.open(registration, timeout=10) as response: assert json.loads(response.read())["clients"] == 1
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
