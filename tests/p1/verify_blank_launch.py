from __future__ import annotations

import json
import sys
import tempfile
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
                with urllib.request.urlopen(instance["launchUrl"], timeout=10) as response:
                    assert response.status == 200
                    assert response.read(), kind
        finally:
            stopped = manager.stop_all()
        assert len(stopped) == len(opened)
        print(json.dumps({"passed": True, "kinds": [item["kind"] for item in opened]}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
