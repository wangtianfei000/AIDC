#!/usr/bin/env python3
"""P1 acceptance for the AIDCRobotics mechanical and device library."""
from __future__ import annotations

import csv
import json
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
AIDC_ROOT = MODEL_BROWSER.parent
REPO_ROOT = AIDC_ROOT.parent
LIBRARY = MODEL_BROWSER / "libraries" / "AIDCRobotics"
T2 = AIDC_ROOT / "samples" / "modelica-t2-single-joint"
RESULTS = MODEL_BROWSER / "results"
OMC = REPO_ROOT / "tools" / "openmodelica" / "1.27.1" / "bin" / "omc.exe"
sys.path.insert(0, str(MODEL_BROWSER / "tools"))

from project_tool import run_project, source_fingerprint, validate_project, write_json


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def read_rows(path: Path) -> list[dict[str, float]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return [{key: float(value) for key, value in row.items()} for row in csv.DictReader(handle)]


def check_components() -> dict[str, object]:
    classes = [
        "AIDCRobotics.Mechanics.RigidBody3D",
        "AIDCRobotics.Mechanics.FixedBase3D",
        "AIDCRobotics.Mechanics.FloatingBase3D",
        "AIDCRobotics.Mechanics.FixedJoint3D",
        "AIDCRobotics.Mechanics.RevoluteJoint3D",
        "AIDCRobotics.Mechanics.PrismaticJoint3D",
        "AIDCRobotics.Mechanics.RotationalJointLoss",
        "AIDCRobotics.Mechanics.RotationalBacklash",
        "AIDCRobotics.Actuators.LimitedPositionServoAxis",
        "AIDCRobotics.Actuators.LimitedDcMotor",
        "AIDCRobotics.Electrical.SimpleBatteryBus",
        "AIDCRobotics.Sensors.IdealJointSensor",
        "AIDCRobotics.Sensors.ImuSignalAdapter",
        "AIDCRobotics.Profiles.ArticulatedRobot",
        "AIDCRobotics.Profiles.FloatingBaseRobot",
        "AIDCRobotics.Profiles.BatteryPoweredRobot",
        "AIDCRobotics.Profiles.ImuEquippedRobot",
    ]
    source = (LIBRARY / "modelica" / "AIDCRobotics" / "package.mo").as_posix()
    lines = [
        'setCommandLineOptions("--std=3.4");',
        "loadModel(Modelica);",
        f'loadFile("{source}");',
        *[f"checkModel({name});" for name in classes],
        "getErrorString();",
    ]
    with tempfile.NamedTemporaryFile("w", suffix=".mos", encoding="utf-8", delete=False) as handle:
        handle.write("\n".join(lines))
        script = Path(handle.name)
    try:
        completed = subprocess.run([str(OMC), str(script)], capture_output=True, text=True, encoding="utf-8", errors="replace", check=False)
    finally:
        script.unlink(missing_ok=True)
    output = completed.stdout + completed.stderr
    require(completed.returncode == 0, f"OMC component checks exited {completed.returncode}: {output}")
    require("Error:" not in output, f"OMC component checks contain errors: {output}")
    for name in classes:
        require(f"Check of {name} completed successfully" in output, f"component check missing: {name}")
    return {"checkedClasses": classes, "count": len(classes)}


def compare_t2(p1_run: Path, t2_run: Path) -> dict[str, object]:
    p1 = read_rows(p1_run / "series.csv")
    t2 = read_rows(t2_run / "series.csv")
    variables = [
        "jointAngle", "jointSpeed", "windingTemperature", "electricalEnergy",
        "referenceAngle", "trackingError", "loadTorque", "jointTorque",
        "armatureCurrent", "sourceVoltage", "busVoltage", "driveVoltage", "electricalPower",
    ]
    require(len(p1) == len(t2), f"sample count mismatch: P1={len(p1)} T2={len(t2)}")
    max_errors = {name: 0.0 for name in variables}
    max_time_error = 0.0
    for left, right in zip(p1, t2):
        max_time_error = max(max_time_error, abs(left["time"] - right["time"]))
        for name in variables:
            max_errors[name] = max(max_errors[name], abs(left[name] - right[name]))
    max_error = max(max_errors.values())
    require(max_time_error <= 1e-12, f"time grid mismatch: {max_time_error}")
    require(max_error <= 1e-9, f"P1 and T2 trajectories differ: {max_errors}")
    return {
        "p1Run": p1_run.name,
        "t2Run": t2_run.name,
        "sampleCount": len(p1),
        "maximumTimeError_s": max_time_error,
        "maximumAbsoluteError": max_error,
        "absoluteErrorByVariable": max_errors,
        "finalJointAngle_rad": p1[-1]["jointAngle"],
        "finalBusVoltage_V": p1[-1]["busVoltage"],
    }


def main() -> int:
    started = time.perf_counter()
    validate_project(LIBRARY)
    validate_project(T2)
    source_digest, source_files = source_fingerprint(LIBRARY)
    component_check = check_components()
    settings = {
        "startTime": 0.0, "stopTime": 3.0, "outputInterval": 0.002,
        "relativeTolerance": 1e-7, "solver": "dassl", "solverFlags": {},
    }
    p1_run = run_project(LIBRARY, simulation_settings=settings, model_class="AIDCRobotics.Examples.HD1910PositionStep")
    mechanics_run = run_project(
        LIBRARY,
        simulation_settings={"startTime": 0.0, "stopTime": 0.01, "outputInterval": 0.001, "relativeTolerance": 1e-8, "solver": "dassl", "solverFlags": {}},
        model_class="AIDCRobotics.Examples.FixedBodyAssembly",
    )
    device_run = run_project(
        LIBRARY,
        simulation_settings={"startTime": 0.0, "stopTime": 0.01, "outputInterval": 0.001, "relativeTolerance": 1e-8, "solver": "dassl", "solverFlags": {}},
        model_class="AIDCRobotics.Conformance.P1DeviceInterfaces",
    )
    t2_run = run_project(T2, simulation_settings=settings, model_class="AIDCT2.PositionStep")
    parity = compare_t2(p1_run, t2_run)
    summary = {
        "schemaVersion": "1.0",
        "passed": True,
        "completedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "durationSeconds": time.perf_counter() - started,
        "standard": {"id": "org.aidc.robotics", "version": "0.2.0", "revision": "0.2.0-p1"},
        "library": {"path": str(LIBRARY), "sourceDigest": source_digest, "sourceFileCount": len(source_files)},
        "components": component_check,
        "deviceConformanceRun": device_run.name,
        "mechanicsAssemblyRun": mechanics_run.name,
        "t2Parity": parity,
        "scope": "P1 mechanics, joint nonidealities, position servo, battery, joint/IMU sensors, HD-1910 data and capability profiles.",
    }
    RESULTS.mkdir(parents=True, exist_ok=True)
    write_json(RESULTS / "p1-robotics-summary.json", summary)
    print(json.dumps({"passed": True, "summary": str(RESULTS / "p1-robotics-summary.json"), "maximumAbsoluteError": parity["maximumAbsoluteError"]}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
