from __future__ import annotations

import http.cookiejar
import json
import sys
import tempfile
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from aidc_core.runtime import RuntimeManager


def post(opener: urllib.request.OpenerDirector, url: str, action: str, client_id: str) -> dict:
    request = urllib.request.Request(
        f"{url}session/client/{action}",
        data=json.dumps({"clientId": client_id}).encode(),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with opener.open(request, timeout=10) as response:
        return json.loads(response.read())


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="aidc-lifecycle-") as directory:
        manager = RuntimeManager(ROOT, Path(directory))
        results: list[dict] = []
        try:
            for kind in ("sysml", "model3d", "modelica"):
                instance = manager.start(kind, launch_browser=False)
                opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
                with opener.open(instance["launchUrl"], timeout=20) as response:
                    page = response.read()
                    assert b"data-aidc-session-client" in page, f"{kind}: lifecycle client was not injected"
                client_id = f"verify-{kind}"
                assert post(opener, instance["url"], "register", client_id)["clients"] == 1
                assert post(opener, instance["url"], "unregister", client_id)["clients"] == 0

                pids = [int(item["pid"]) for item in instance["processes"]]
                deadline = time.monotonic() + 20
                while any(RuntimeManager._alive(pid) for pid in pids) and time.monotonic() < deadline:
                    time.sleep(0.2)
                remaining = [pid for pid in pids if RuntimeManager._alive(pid)]
                assert not remaining, f"{kind}: processes remained after page close: {remaining}"
                status = next(item for item in manager.status() if item["instanceId"] == instance["instanceId"])
                assert status["status"] == "stopped", status
                results.append({"kind": kind, "processesStopped": len(pids)})
        finally:
            manager.stop_all()
        print(json.dumps({"passed": True, "results": results}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
