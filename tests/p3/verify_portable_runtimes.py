from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PORTABLE = ROOT / "distribution" / "out" / "aidc-portable"
OMC_RUNTIME = ROOT.parent / "tools" / "openmodelica" / "1.27.1"
FREECAD_RUNTIME = ROOT.parent / "tools" / "freecad" / "1.1.3"


def junction(link: Path, target: Path) -> None:
    if link.exists():
        raise RuntimeError(f"Refusing to replace existing runtime path: {link}")
    completed = subprocess.run(
        ["powershell", "-NoProfile", "-Command", "& { param([string]$link,[string]$target) New-Item -ItemType Junction -Path $link -Target $target | Out-Null }", str(link), str(target)],
        capture_output=True,
        text=True,
        encoding="utf-8",
        timeout=30,
        check=False,
    )
    if completed.returncode:
        raise RuntimeError(completed.stderr)


def run(*arguments: str, timeout: int = 300) -> dict:
    environment = os.environ.copy()
    environment["PATH"] = str(Path(os.environ["SystemRoot"]) / "System32")
    completed = subprocess.run(
        [str(PORTABLE / "bin" / "aidc.exe"), *arguments, "--json"],
        cwd=ROOT,
        env=environment,
        capture_output=True,
        text=True,
        encoding="utf-8",
        timeout=timeout,
        check=False,
    )
    result = json.loads(completed.stdout)
    if completed.returncode or result["status"] != "succeeded":
        raise RuntimeError(json.dumps(result, ensure_ascii=False))
    return result


def main() -> int:
    runtimes = PORTABLE / "runtimes"
    runtimes.mkdir(exist_ok=True)
    links = [runtimes / "openmodelica", runtimes / "freecad"]
    if not (PORTABLE / "bin" / "aidc.exe").is_file():
        raise RuntimeError("Build the portable distribution before running this verification")
    if not (OMC_RUNTIME / "bin" / "omc.exe").is_file() or not (FREECAD_RUNTIME / "bin" / "FreeCADCmd.exe").is_file():
        raise RuntimeError("Engineering OMC and FreeCAD runtimes are unavailable")
    junction(links[0], OMC_RUNTIME)
    junction(links[1], FREECAD_RUNTIME)
    try:
        with tempfile.TemporaryDirectory(prefix="aidc-p3-portable-") as directory:
            project = Path(directory) / "modelica"
            shutil.copytree(ROOT / "modelica-browser" / "generated" / "p2-single-joint", project, ignore=shutil.ignore_patterns("results"))
            dependencies = run("deps", "status")
            simulation = run("modelica", "simulate", str(project), timeout=300)
            cad = ROOT / "samples" / "jingang-10axis-system-design" / "layout" / "jingang-10axis-layout.FCStd"
            checked = run("freecad", "check", str(cad), timeout=180)
            installed = {item["id"]: item for item in dependencies["data"]["dependencies"]}
            assert installed["openmodelica"]["available"] and installed["freecad"]["available"]
            assert (Path(simulation["data"]["output"]) / "run-manifest.json").is_file()
            print(json.dumps({"passed": True, "frozenCli": True, "pathIsolation": True, "freecadObjects": checked["data"]["output"].splitlines()[0]}, ensure_ascii=False))
    finally:
        for link in links:
            if link.is_dir():
                os.rmdir(link)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
