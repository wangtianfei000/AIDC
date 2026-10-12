from __future__ import annotations

import argparse
import http.cookiejar
import json
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request
from urllib.parse import urljoin
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def run(cli: Path, *arguments: str) -> dict:
    env = os.environ.copy(); env["PATH"] = str(Path(os.environ["SystemRoot"]) / "System32")
    completed = subprocess.run([str(cli), *arguments, "--json"], capture_output=True, text=True, encoding="utf-8", env=env, timeout=180, check=False)
    value = json.loads(completed.stdout)
    if completed.returncode or value["status"] != "succeeded": raise RuntimeError(json.dumps(value, ensure_ascii=False))
    return value


def main() -> int:
    parser = argparse.ArgumentParser(); parser.add_argument("--portable", type=Path, default=ROOT / "distribution" / "out" / "aidc-portable")
    portable = parser.parse_args().portable.resolve(); cli = portable / "bin" / "aidc.exe"
    assert cli.is_file() and not any(path.name == "node_modules" for path in portable.rglob("*"))
    run(cli, "browser", "stop", "--all")
    try:
        with tempfile.TemporaryDirectory(prefix="aidc-p2-") as directory:
            target = Path(directory)
            sysml = target / "architecture.sysml"; shutil.copy2(ROOT / "samples" / "jingang-10axis-system-design" / "architecture.sysml", sysml)
            fixture_script = ROOT / "model3d-viewer" / "tools" / "freecad-publisher" / "tests" / "build_modelica_export_fixture.py"
            fixture = Path(subprocess.run([sys.executable, str(fixture_script)], capture_output=True, text=True, encoding="utf-8", timeout=60, check=True).stdout.strip())
            model3d = target / "model3d"; shutil.copytree(fixture, model3d)
            modelica = target / "modelica"
            run(cli, "sysml", "validate", str(sysml))
            assembly = target / "single-joint.aidc-assembly"; exported = run(cli, "model3d", "export-modelica", str(model3d), str(assembly)); assert exported["data"]["valid"] and (assembly / "assembly.manifest.json").is_file()
            run(cli, "modelica", "generate", str(ROOT / "modelica-browser" / "packages" / "robot-ir" / "examples" / "single-joint"), str(modelica))
            run(cli, "modelica", "validate", str(modelica))
            package = target / "single-joint.aidcmpkg"; run(cli, "modelica", "package", str(modelica), str(package)); assert package.is_file()
            opened = [run(cli, kind, "open", str(path), "--no-browser") for kind, path in (("sysml", sysml), ("model3d", model3d), ("modelica", modelica))]
            results = []
            for item in opened:
                opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
                with opener.open(item["data"]["launchUrl"], timeout=20) as response:
                    results.append({"command": item["command"], "status": response.status, "bytes": len(response.read()), "roles": [process["role"] for process in item["data"]["processes"]]})
                if item["command"] == "model3d.open":
                    with opener.open(urljoin(item["data"]["url"], "model3d.manifest.json"), timeout=20) as response:
                        manifest = json.load(response)
                    assert manifest["modelId"] == "aidc.standard.single-joint"
                    with opener.open(urljoin(item["data"]["url"], f"api/model-projects/{manifest['modelId']}/pose-presets"), timeout=20) as response:
                        assert json.load(response)
            assert all(item["status"] == 200 and item["bytes"] > 100 for item in results)
            assert all("npm" not in " ".join(process["command"]).lower() and "python" not in " ".join(process["command"]).lower() for item in opened for process in item["data"]["processes"])
            print(json.dumps({"passed": True, "portable": str(portable), "nodeModules": False, "browsers": results, "package": str(package)}, ensure_ascii=False))
    finally: run(cli, "browser", "stop", "--all")
    return 0


if __name__ == "__main__": raise SystemExit(main())
