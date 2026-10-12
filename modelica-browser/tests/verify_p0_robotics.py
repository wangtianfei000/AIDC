#!/usr/bin/env python3
"""P0 acceptance for AIDCRobotics foundation and Robot IR contracts."""
from __future__ import annotations

import csv
import hashlib
import json
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
AIDC_ROOT = MODEL_BROWSER.parent
REPO_ROOT = AIDC_ROOT.parent
LIBRARY = MODEL_BROWSER / "libraries" / "AIDCRobotics"
ROBOT_IR = MODEL_BROWSER / "packages" / "robot-ir"
EXAMPLE_IR = ROBOT_IR / "examples" / "single-joint"
RESULTS = MODEL_BROWSER / "results"
OMC = REPO_ROOT / "tools" / "openmodelica" / "1.27.1" / "bin" / "omc.exe"
sys.path.insert(0, str(MODEL_BROWSER / "tools"))
sys.path.insert(0, str(ROBOT_IR / "src"))

from project_tool import read_json, run_project, sha256_file, source_fingerprint, validate_project, write_json
from robot_ir_validator import validate_bundle


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def final_row(path: Path) -> dict[str, str]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    require(bool(rows), f"empty simulation series: {path}")
    return rows[-1]


def digest_tree(root: Path) -> str:
    digest = hashlib.sha256()
    for path in sorted((item for item in root.rglob("*") if item.is_file()), key=lambda item: item.relative_to(root).as_posix()):
        digest.update(path.relative_to(root).as_posix().encode())
        digest.update(path.read_bytes())
    return "sha256:" + digest.hexdigest()


def build_fmu(native_run: Path) -> dict[str, Any]:
    output = LIBRARY / "results" / "p0-interface"
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    package_file = LIBRARY / "modelica" / "AIDCRobotics" / "package.mo"
    mos = output / "build-fmu.mos"
    mos.write_text("\n".join([
        'loadModel(Modelica, {"4.1.0"});',
        f'loadFile("{package_file.resolve().as_posix()}");',
        f'cd("{output.resolve().as_posix()}");',
        'buildModelFMU(AIDCRobotics.Examples.SingleJoint, version="2.0", fmuType="me_cs");',
        'getErrorString();',
        "",
    ]), encoding="utf-8")
    built = subprocess.run([str(OMC), str(mos)], capture_output=True, text=True)
    (output / "build.log").write_text(built.stdout + built.stderr, encoding="utf-8")
    fmu = output / "SingleJoint.fmu"
    require(built.returncode == 0 and fmu.is_file(), "AIDCRobotics FMI 2.0 export failed")

    from fmpy import read_model_description, simulate_fmu

    description = read_model_description(str(fmu))
    requested = ["jointPosition", "jointVelocity", "appliedTorque", "valid"]
    variables = {variable.name: variable for variable in description.modelVariables}
    require(set(requested) <= set(variables), "FMU is missing standard joint outputs")
    result = simulate_fmu(str(fmu), start_time=0, stop_time=2, output_interval=0.001, output=requested, fmi_type="CoSimulation")
    fmu_final = {name: float(result[name][-1]) for name in requested}
    native = final_row(native_run / "series.csv")
    native_final = {name: float(native[name]) for name in requested}
    errors = {name: abs(fmu_final[name] - native_final[name]) for name in requested}
    require(max(errors.values()) <= 1e-5, f"FMU and native simulation differ: {errors}")
    return {
        "path": str(fmu),
        "sha256": sha256_file(fmu),
        "fmiVersion": description.fmiVersion,
        "supportsModelExchange": description.modelExchange is not None,
        "supportsCoSimulation": description.coSimulation is not None,
        "outputs": [
            {"name": name, "causality": variables[name].causality, "unit": getattr(variables[name], "unit", None)}
            for name in requested
        ],
        "absoluteError": errors,
    }


def main() -> int:
    started = time.perf_counter()
    validate_project(LIBRARY)
    source_digest, source_files = source_fingerprint(LIBRARY)
    ir_summary = validate_bundle(EXAMPLE_IR)

    subprocess.run([sys.executable, str(ROBOT_IR / "tests" / "validate_robot_ir.py")], check=True)
    simulation = {"startTime": 0.0, "stopTime": 2.0, "outputInterval": 0.001, "relativeTolerance": 1e-8, "solver": "dassl", "solverFlags": {}}
    example_run = run_project(LIBRARY, simulation_settings=simulation, model_class="AIDCRobotics.Examples.SingleJoint")
    conformance_run = run_project(LIBRARY, simulation_settings=simulation, model_class="AIDCRobotics.Conformance.P0SingleJoint")
    final = final_row(example_run / "series.csv")
    position = float(final["jointPosition"])
    torque = float(final["appliedTorque"])
    require(abs(position - 0.25) <= 1e-4, f"single-joint static response mismatch: {position}")
    require(abs(torque - 0.5) <= 1e-9, f"single-joint torque mismatch: {torque}")
    require(float(final["valid"]) == 1.0, "single-joint diagnostic is invalid")
    fmu = build_fmu(example_run)

    summary = {
        "schemaVersion": "1.0",
        "passed": True,
        "completedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "durationSeconds": time.perf_counter() - started,
        "standard": {"id": "org.aidc.robotics", "version": "0.1.0", "revision": "0.1.0-p0"},
        "library": {"path": str(LIBRARY), "sourceDigest": source_digest, "sourceFileCount": len(source_files)},
        "robotIr": {**ir_summary, "bundleDigest": digest_tree(EXAMPLE_IR)},
        "nativeSimulation": {
            "exampleRun": example_run.name,
            "conformanceRun": conformance_run.name,
            "finalPosition_rad": position,
            "expectedPosition_rad": 0.25,
            "finalAppliedTorque_Nm": torque,
        },
        "fmi": fmu,
        "scope": "P0 foundation: version/coordinate metadata, public joint interfaces, Robot IR schemas, semantic validator, one executable component and FMI smoke test.",
    }
    RESULTS.mkdir(parents=True, exist_ok=True)
    write_json(RESULTS / "p0-robotics-summary.json", summary)
    print(json.dumps({"passed": True, "summary": str(RESULTS / "p0-robotics-summary.json")}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
