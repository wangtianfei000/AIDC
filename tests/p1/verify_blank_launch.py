from __future__ import annotations

import json
import http.cookiejar
import re
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from aidc_core.runtime import RuntimeManager


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="aidc-blank-launch-") as directory:
        manager = RuntimeManager(ROOT, Path(directory))
        opened: list[dict] = []
        try:
            for kind in ("sysml", "model3d", "modelica"):
                instance = manager.start(kind, launch_browser=False)
                opened.append(instance)
                assert instance["target"] is None, instance
                print(f"checking {kind}: {instance['url']}", flush=True)
                opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
                with opener.open(instance["launchUrl"], timeout=10) as response:
                    assert response.status == 200
                    page = response.read().decode("utf-8")
                    assert page, kind
                if kind == "sysml":
                    script = re.search(r'<script[^>]+src="\.([^\"]+\.js)"', page)
                    assert script, "SysML 页面没有 JavaScript 入口"
                    with opener.open(instance["url"].rstrip("/") + script.group(1), timeout=10) as response:
                        assert response.status == 200
                        assert "javascript" in response.headers.get_content_type(), response.headers.get("Content-Type")
                        assert response.read(), "SysML JavaScript 入口为空"
                if kind == "model3d":
                    try:
                        opener.open(instance["url"] + "model3d.manifest.json", timeout=10)
                    except urllib.error.HTTPError as error:
                        assert error.code == 404, error.code
                    else:
                        raise AssertionError("空三维浏览器不应返回伪造的模型清单")
        finally:
            stopped = manager.stop_all()
        assert len(stopped) == len(opened)
        print(json.dumps({"passed": True, "kinds": [item["kind"] for item in opened]}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
