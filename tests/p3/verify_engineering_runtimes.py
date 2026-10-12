from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def run(*arguments: str, timeout: int = 300) -> dict:
    environment = os.environ.copy(); environment["PATH"] = str(Path(os.environ["SystemRoot"]) / "System32")
    completed = subprocess.run([sys.executable, str(ROOT / "aidc.py"), *arguments, "--json"], cwd=ROOT, env=environment, capture_output=True, text=True, encoding="utf-8", timeout=timeout, check=False)
    result = json.loads(completed.stdout)
    if completed.returncode or result["status"] != "succeeded": raise RuntimeError(json.dumps(result, ensure_ascii=False))
    return result


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="aidc-p3-tools-") as directory:
        target = Path(directory)
        project = target / "modelica"
        shutil.copytree(ROOT / "modelica-browser" / "generated" / "p2-single-joint", project, ignore=shutil.ignore_patterns("results"))
        simulation = run("modelica", "simulate", str(project), timeout=300)
        run_dir = Path(simulation["data"]["output"])
        assert (run_dir / "run-manifest.json").is_file() and (run_dir / "series.csv").is_file()

        cad = ROOT / "samples" / "jingang-10axis-system-design" / "layout" / "jingang-10axis-layout.FCStd"
        checked = run("freecad", "check", str(cad), timeout=180)
        published = target / "published"
        run("model3d", "publish", str(ROOT / "samples" / "jingang-10axis-system-design" / "model3d" / "publish.config.json"), str(published), timeout=900)
        expected = {"visual.glb", "collision.glb", "kinematics.json", "model3d.manifest.json", "publish-report.json"}
        assert expected.issubset({item.name for item in published.iterdir()})
        print(json.dumps({"passed": True, "pathIsolation": True, "modelicaRun": str(run_dir), "freecadObjects": checked["data"]["output"].splitlines()[0], "publishedFiles": sorted(expected)}, ensure_ascii=False))
    return 0


if __name__ == "__main__": raise SystemExit(main())
