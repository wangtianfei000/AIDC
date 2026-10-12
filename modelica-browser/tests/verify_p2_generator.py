#!/usr/bin/env python3
"""P2 acceptance for the Robot IR to Modelica vertical slice."""
from __future__ import annotations

import csv
import json
import shutil
import sys
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
AIDC_ROOT = MODEL_BROWSER.parent
SOURCE = MODEL_BROWSER / "packages" / "robot-ir" / "examples" / "single-joint"
OUTPUT = MODEL_BROWSER / "generated" / "p2-single-joint"
MANUAL = AIDC_ROOT / "samples" / "modelica-p2-manual-single-joint"
RESULTS = MODEL_BROWSER / "results"
sys.path.insert(0, str(MODEL_BROWSER / "tools"))

from modelica_generator import GenerationError, generate
from project_tool import read_json, run_project, validate_project, write_json


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def rows(path: Path) -> list[dict[str, float]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return [{key: float(value) for key, value in row.items()} for row in csv.DictReader(handle)]


def compare(generated_run: Path, manual_run: Path) -> dict[str, Any]:
    generated, manual = rows(generated_run / "series.csv"), rows(manual_run / "series.csv")
    variables = ["jointPosition", "jointVelocity", "appliedTorque"]
    require(len(generated) == len(manual), f"sample count mismatch: {len(generated)} != {len(manual)}")
    errors = {name: 0.0 for name in variables}
    time_error = 0.0
    for left, right in zip(generated, manual):
        time_error = max(time_error, abs(left["time"] - right["time"]))
        for name in variables:
            errors[name] = max(errors[name], abs(left[name] - right[name]))
    maximum = max(errors.values())
    require(time_error <= 1e-12, f"time grid differs by {time_error}")
    require(maximum <= 1e-10, f"generated/manual trajectories differ: {errors}")
    return {
        "generatedRun": generated_run.name, "manualRun": manual_run.name,
        "sampleCount": len(generated), "maximumTimeError_s": time_error,
        "maximumAbsoluteError": maximum, "absoluteErrorByVariable": errors,
        "finalJointPosition_rad": generated[-1]["jointPosition"],
    }


def verify_rebuild() -> dict[str, Any]:
    RESULTS.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="p2-rebuild-", dir=RESULTS) as temporary:
        root = Path(temporary) / "project"
        first = generate(SOURCE, root)
        override_path = root / "overrides" / "user-overrides.json"
        override = read_json(override_path)
        override["commandStepTorque_Nm"] = 0.08
        write_json(override_path, override)
        customized = generate(SOURCE, root)
        preserved = override_path.read_bytes()
        for item in list(root.iterdir()):
            if item.name == "overrides":
                continue
            if item.is_dir():
                shutil.rmtree(item)
            else:
                item.unlink()
        rebuilt = generate(SOURCE, root)
        require(override_path.read_bytes() == preserved, "user override changed during clean rebuild")
        require(customized["generationDigest"] == rebuilt["generationDigest"], "clean rebuild is not deterministic")
        validate_project(root)
        return {
            "defaultGenerationDigest": first["generationDigest"],
            "customizedGenerationDigest": customized["generationDigest"],
            "rebuiltGenerationDigest": rebuilt["generationDigest"],
            "overridePreserved": True,
        }


def verify_diagnostic() -> dict[str, str]:
    with tempfile.TemporaryDirectory(prefix="p2-invalid-", dir=RESULTS) as temporary:
        root = Path(temporary)
        for name in ("physics.json", "devices.json", "interfaces.json"):
            shutil.copyfile(SOURCE / name, root / name)
        physics = read_json(root / "physics.json")
        physics["joints"][0]["type"] = "prismatic"
        write_json(root / "physics.json", physics)
        try:
            generate(root, root / "output")
        except GenerationError as exc:
            require(exc.document == "physics.json" and exc.pointer == "/joints/0/type", "diagnostic location is not precise")
            return {"document": exc.document, "jsonPointer": exc.pointer, "message": str(exc)}
        raise AssertionError("unsupported P2 joint did not fail")


def main() -> int:
    started = time.perf_counter()
    lock = generate(SOURCE, OUTPUT)
    loaded = validate_project(OUTPUT)
    validate_project(MANUAL)
    package_text = loaded["modelFile"].read_text(encoding="utf-8")
    for marker in (
        "AIDCRobotics.Mechanics.RigidBody3D", "AIDCRobotics.Mechanics.RevoluteJoint3D",
        "AIDCRobotics.Actuators.IdealTorque", "Icon(graphics", "Diagram(coordinateSystem", "experiment(",
    ):
        require(marker in package_text, f"generated package is missing {marker}")
    require((OUTPUT / "robot-ir" / "generation.lock.json").is_file(), "generation lock is missing")
    rebuild = verify_rebuild()
    diagnostic = verify_diagnostic()
    settings = {"startTime": 0.0, "stopTime": 2.0, "outputInterval": 0.001, "relativeTolerance": 1e-8, "solver": "dassl", "solverFlags": {}}
    generated_run = run_project(OUTPUT, simulation_settings=settings)
    manual_run = run_project(MANUAL, simulation_settings=settings)
    parity = compare(generated_run, manual_run)
    summary = {
        "schemaVersion": "1.0", "passed": True,
        "completedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "durationSeconds": time.perf_counter() - started,
        "generator": lock["generator"], "output": str(OUTPUT), "package": lock["package"],
        "generationDigest": lock["generationDigest"],
        "generatedStructure": {"rigidBodies": 1, "revoluteJoints": 1, "standardActuators": 1},
        "generatedArtifacts": [item["path"] for item in lock["outputs"]],
        "rebuild": rebuild, "diagnosticLocalization": diagnostic, "manualParity": parity,
        "scope": "P2 Robot IR vertical slice with parameters, connections, icon, experiment, project entry, lock and user override preservation.",
    }
    write_json(RESULTS / "p2-generator-summary.json", summary)
    print(json.dumps({"passed": True, "summary": str(RESULTS / "p2-generator-summary.json"), "maximumAbsoluteError": parity["maximumAbsoluteError"]}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
