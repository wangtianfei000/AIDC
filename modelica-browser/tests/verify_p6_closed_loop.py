#!/usr/bin/env python3
"""P6 end-to-end Robot IR -> portable Modelica simulation acceptance."""
from __future__ import annotations

import csv
import hashlib
import json
import math
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / "tools" / "aidc_modelica.py"
PROJECT_TOOL = ROOT / "tools" / "project_tool.py"
EXAMPLES = ROOT / "packages" / "robot-ir" / "examples"
OUTPUT_ROOT = ROOT / "generated" / "p6-closed-loop"
DIST = ROOT / "dist"
SUMMARY = ROOT / "results" / "p6-closed-loop-summary.json"

CASES = (
    {"name": "microduck-native12", "expected": {"rootType": "floating", "bodyCount": 13, "jointCount": 12, "actuatorCount": 12}},
    {"name": "reference-arm5", "expected": {"rootType": "fixed", "bodyCount": 5, "jointCount": 4, "actuatorCount": 4}},
)


def invoke(script: Path, *arguments: object, expected: int = 0) -> dict[str, Any]:
    process = subprocess.run(
        [sys.executable, str(script), *(str(item) for item in arguments)],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if process.returncode != expected:
        raise AssertionError(
            f"command failed ({process.returncode}): {script.name} {' '.join(map(str, arguments))}\n"
            f"stdout:\n{process.stdout}\nstderr:\n{process.stderr}"
        )
    payload = process.stdout if expected == 0 else process.stderr
    return json.loads(payload)


def sha256(path: Path) -> str:
    return "sha256:" + hashlib.sha256(path.read_bytes()).hexdigest()


def inspect_series(run_dir: Path) -> dict[str, Any]:
    path = run_dir / "series.csv"
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        reader = csv.reader(handle)
        header = next(reader)
        rows = [[float(value) for value in row] for row in reader if row]
    assert rows and len(rows) > 1, f"simulation has insufficient samples: {path}"
    assert header[0] == "time", f"first series column is not time: {header[0]}"
    times = [row[0] for row in rows]
    assert all(right >= left for left, right in zip(times, times[1:])), "simulation time moves backwards"
    assert times[-1] > times[0], "simulation time did not advance"
    assert all(math.isfinite(value) for row in rows for value in row), "simulation contains non-finite values"
    return {
        "sampleCount": len(rows),
        "variableCount": len(header) - 1,
        "startTime": times[0],
        "finalTime": times[-1],
        "seriesSha256": sha256(path),
    }


def verify_case(case: dict[str, Any]) -> dict[str, Any]:
    name = case["name"]
    source = EXAMPLES / name
    output = OUTPUT_ROOT / name
    package = DIST / f"{name}-p6.aidcmpkg"
    build_bundle = source / "build_bundle.py"
    if build_bundle.is_file():
        subprocess.run([sys.executable, str(build_bundle)], cwd=ROOT, check=True)

    if output.exists():
        shutil.rmtree(output)
    validated = invoke(CLI, "validate", source)
    first = invoke(CLI, "generate", source, output)
    override = output / "overrides" / "user-overrides.json"
    override_digest = sha256(override)
    second = invoke(CLI, "generate", source, output)
    assert first["cacheStatus"] == "miss" and second["cacheStatus"] == "hit"
    assert first["generationDigest"] == second["generationDigest"]
    assert first["requestDigest"] == second["requestDigest"]
    assert sha256(override) == override_digest, "generator rewrote the user override"

    lock = json.loads((output / "robot-ir" / "generation.lock.json").read_text(encoding="utf-8"))
    for key, expected in case["expected"].items():
        assert lock["structure"][key] == expected, f"{name}: unexpected {key}"

    project_validation = invoke(CLI, "validate", output)
    original_run_result = invoke(PROJECT_TOOL, "run", output)
    original_run = Path(original_run_result["runDir"])
    original_series = inspect_series(original_run)

    packaged = invoke(CLI, "package", output, package)
    first_package_digest = sha256(package)
    packaged_again = invoke(CLI, "package", output, package)
    assert sha256(package) == first_package_digest == packaged_again["sha256"], "portable package is not deterministic"

    with tempfile.TemporaryDirectory(prefix=f"aidc-p6-{name}-") as directory:
        portable = Path(directory) / "project"
        portable.mkdir()
        with zipfile.ZipFile(package) as archive:
            archive.extractall(portable)
        portable_validation = invoke(CLI, "validate", portable)
        portable_run_result = invoke(PROJECT_TOOL, "run", portable)
        portable_run = Path(portable_run_result["runDir"])
        portable_series = inspect_series(portable_run)
        comparison = invoke(CLI, "compare", original_run, portable_run, "--abs-tol", 0, "--rel-tol", 0)
        assert comparison["passed"]
        portable_manifest = json.loads((portable_run / "run-manifest.json").read_text(encoding="utf-8"))
        assert Path(directory) in portable_run.parents, "portable run did not execute outside the AIDC workspace"

    original_manifest = json.loads((original_run / "run-manifest.json").read_text(encoding="utf-8"))
    return {
        "name": name,
        "robotIr": {key: lock["structure"][key] for key in case["expected"]},
        "validation": {"robotIr": validated["passed"], "project": project_validation["passed"], "portableProject": portable_validation["passed"]},
        "generation": {"generationDigest": first["generationDigest"], "requestDigest": first["requestDigest"], "secondRunCacheStatus": second["cacheStatus"], "overridePreserved": True},
        "originalRun": {"runId": original_manifest["runId"], "sourceDigest": original_manifest["sourceDigest"], "toolchain": original_manifest["toolchain"], **original_series},
        "portablePackage": {"path": package.relative_to(ROOT).as_posix(), "sha256": packaged["sha256"], "fileCount": packaged["fileCount"], "deterministic": True},
        "portableRun": {"runId": portable_manifest["runId"], "sourceDigest": portable_manifest["sourceDigest"], "toolchain": portable_manifest["toolchain"], **portable_series},
        "trajectoryComparison": {"passed": comparison["passed"], "sampleCount": comparison["sampleCount"], "variableCount": len(comparison["variables"]) - 1, "maximumAbsoluteError": max(item["maxAbsoluteError"] for item in comparison["variables"])},
    }


def main() -> int:
    started = time.perf_counter()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    DIST.mkdir(parents=True, exist_ok=True)
    SUMMARY.parent.mkdir(parents=True, exist_ok=True)
    generator_source = (ROOT / "tools" / "modelica_generator.py").read_text(encoding="utf-8").lower()
    assert "microduck" not in generator_source and "reference-arm5" not in generator_source and "reference.arm5" not in generator_source
    cases = [verify_case(case) for case in CASES]
    summary = {
        "schemaVersion": "aidc.verification.p6.closed-loop/0.1",
        "passed": True,
        "completedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "durationSeconds": time.perf_counter() - started,
        "scope": "Robot IR validation, deterministic generation, OMC native simulation, portable packaging, out-of-workspace replay and full-trajectory reproduction. Training interfaces and GPU compilation are excluded.",
        "trainingInterfaceImplemented": False,
        "cases": cases,
    }
    SUMMARY.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
