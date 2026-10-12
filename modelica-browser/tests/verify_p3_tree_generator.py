#!/usr/bin/env python3
"""P3 acceptance for fixed/floating arbitrary tree Modelica generation."""
from __future__ import annotations

import csv
import json
import shutil
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
SOURCE = MODEL_BROWSER / "packages" / "robot-ir" / "examples" / "microduck-native12"
SINGLE = MODEL_BROWSER / "packages" / "robot-ir" / "examples" / "single-joint"
OUTPUT = MODEL_BROWSER / "generated" / "microduck-native12"
RESULTS = MODEL_BROWSER / "results"
sys.path.insert(0, str(MODEL_BROWSER / "tools"))
sys.path.insert(0, str(MODEL_BROWSER / "packages" / "robot-ir" / "src"))

from modelica_generator import generate
from project_tool import read_json, run_project, validate_project, write_json
from robot_ir_validator import validate_bundle


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def csv_rows(path: Path) -> list[dict[str, float]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return [{key: float(value) for key, value in row.items()} for row in csv.DictReader(handle)]


def verify_structure(lock: dict[str, Any]) -> dict[str, Any]:
    physics = read_json(SOURCE / "physics.json")
    structure = read_json(OUTPUT / "robot-ir" / "generated-structure.json")
    require(lock["structure"] == {
        "rootType": "floating", "bodyCount": 13, "jointCount": 12,
        "actuatorCount": 12, "totalMass_kg": 0.6608121798488,
    }, f"unexpected lock structure: {lock['structure']}")
    require(structure["rootBodyId"] == physics["rootBodyId"], "root body was not preserved")
    require(structure["rootInitialTranslation_m"] == physics["rootInitialTranslation_m"], "root pose was not preserved")
    require(abs(structure["totalMass_kg"] - sum(item["mass_kg"] for item in physics["bodies"])) <= 1e-15, "total mass differs")
    for source, generated in zip(physics["bodies"], structure["bodies"]):
        for field in ("bodyId", "mass_kg", "centerOfMass_m", "inertia_kg_m2"):
            require(generated[field] == source[field], f"body {source['bodyId']} field {field} differs")
    for source, generated in zip(physics["joints"], structure["joints"]):
        for field in ("jointId", "type", "parentBodyId", "childBodyId", "translation_m", "rotation_xyzw", "axis", "limit"):
            require(generated[field] == source[field], f"joint {source['jointId']} field {field} differs")
    package = (OUTPUT / "modelica" / "MicroduckNative12" / "package.mo").read_text(encoding="utf-8")
    require(package.count("AIDCRobotics.Mechanics.RigidBody3D ") == 13, "generated rigid body count differs")
    require(package.count("AIDCRobotics.Mechanics.RevoluteJoint3D ") == 12, "generated revolute count differs")
    require(package.count("Placement(visible = false") == package.count("Modelica.Mechanics.MultiBody.Visualizers.FixedShape "), "CAD visualization helpers must stay out of the connection diagram")
    require(".frame_b, mount" in package, "generated child joints must leave the parent body's right-side frame")
    require(package.count("annotation(Line(points") >= 37, "connection diagram annotations are incomplete")
    require("{{-56, 0}, {-55, 0}, {-55, 80}}" in package, "branch connection must use an orthogonal route")
    require([pose["name"] for pose in structure["namedPoses"]] == ["stand"], "stand named pose is missing")
    return {
        "rootType": structure["rootType"], "bodyCount": len(structure["bodies"]),
        "jointCount": len(structure["joints"]), "actuatorCount": len(structure["actuators"]),
        "totalMass_kg": structure["totalMass_kg"],
        "massComInertiaFieldsChecked": 13, "namedPoses": ["stand"],
    }


def verify_named_pose(run_dir: Path) -> dict[str, Any]:
    rows = csv_rows(run_dir / "series.csv")
    expected = {
        "trunk": [0.0, 0.0, 0.116719591352],
        "left_foot": [0.0874220581987461, 0.09164254833027018, 0.0803840280654301],
        "head": [0.04428350769733885, -0.03049160434112445, 0.1643272180501759],
        "right_foot": [-0.04496053125003936, -0.002533141742281746, 0.07721531096220136],
    }
    body_order = [item["bodyId"] for item in read_json(SOURCE / "physics.json")["bodies"]]
    errors: dict[str, float] = {}
    # A named pose defines the initial assembly state.  Once uniform gravity is
    # enabled the floating robot is expected to move, so compare the first
    # emitted sample rather than the end of the dynamics run.
    initial = rows[0]
    for body_id, target in expected.items():
        index = body_order.index(body_id) + 1
        actual = [initial[f"bodyPosition[{index},{axis}]"] for axis in (1, 2, 3)]
        errors[body_id] = max(abs(left - right) for left, right in zip(actual, target))
    maximum_pose_error = max(errors.values())
    require(maximum_pose_error <= 1e-12, f"named stand pose differs: {errors}")
    physics = read_json(SOURCE / "physics.json")
    joint_errors = {}
    for index, joint in enumerate(physics["joints"], 1):
        expected_joint = joint["initialPosition_rad"] + joint.get("zeroOffset_rad", 0)
        joint_errors[joint["jointId"]] = abs(initial[f"jointPosition[{index}]"] - expected_joint)
    maximum_joint_error = max(joint_errors.values())
    require(maximum_joint_error <= 1e-12, f"stand joint coordinates differ: {joint_errors}")
    return {
        "name": "stand", "sampleCount": len(rows), "checkedBodies": list(expected),
        "maximumPositionError_m": maximum_pose_error,
        "maximumJointCoordinateError": maximum_joint_error,
    }


def build_mixed_tree(root: Path) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    for name in ("physics.json", "devices.json", "interfaces.json"):
        shutil.copyfile(SINGLE / name, root / name)
    physics = read_json(root / "physics.json")
    link = physics["bodies"][1]
    physics["modelId"] = "aidc.acceptance.mixed-tree"
    physics["modelRevision"] = "p3-r1"
    physics["rootType"] = "fixed"
    physics["rootInitialTranslation_m"] = [0.01, 0.02, 0.03]
    physics["rootInitialRotation_xyzw"] = [0, 0, 0.7071067811865475, 0.7071067811865476]
    slider = {**link, "bodyId": "slider", "parentBodyId": "link", "mass_kg": 0.2}
    leaf = {**link, "bodyId": "fixed-leaf", "parentBodyId": "slider", "mass_kg": 0.1}
    physics["bodies"].extend([slider, leaf])
    prismatic = {**physics["joints"][0], "jointId": "slide-1", "type": "prismatic", "parentBodyId": "link", "childBodyId": "slider", "translation_m": [0.1, 0, 0], "initialPosition_rad": 0.01, "limit": {"lower": 0, "upper": 0.02, "velocity": 1, "effort": 10}}
    fixed = {**physics["joints"][0], "jointId": "fixed-1", "type": "fixed", "parentBodyId": "slider", "childBodyId": "fixed-leaf", "translation_m": [0.1, 0, 0], "initialPosition_rad": 0}
    physics["joints"].extend([prismatic, fixed])
    physics["contactDefaults"]["excludedBodyPairs"].extend([["link", "slider"], ["slider", "fixed-leaf"]])
    write_json(root / "physics.json", physics)
    for name in ("devices.json", "interfaces.json"):
        value = read_json(root / name)
        value["modelId"] = physics["modelId"]
        value["modelRevision"] = physics["modelRevision"]
        write_json(root / name, value)
    return root


def verify_joint_types() -> dict[str, Any]:
    with tempfile.TemporaryDirectory(prefix="p3-mixed-tree-", dir=RESULTS) as temporary:
        source = build_mixed_tree(Path(temporary) / "source")
        output = Path(temporary) / "output"
        validate_bundle(source)
        generate(source, output, "MixedTree", "p3")
        validate_project(output)
        text = (output / "modelica" / "MixedTree" / "package.mo").read_text(encoding="utf-8")
        for marker in ("RevoluteJoint3D", "PrismaticJoint3D", "FixedJoint3D"):
            require(marker in text, f"mixed tree is missing {marker}")
        run = run_project(output, simulation_settings={"startTime": 0, "stopTime": 0.001, "outputInterval": 0.001, "relativeTolerance": 1e-8, "solver": "dassl", "solverFlags": {}})
        return {"rootType": "fixed", "nonIdentityRootPose": True, "bodyCount": 4, "jointCount": 3, "jointTypes": ["revolute", "prismatic", "fixed"], "omcRun": run.name}


def verify_rebuild() -> dict[str, Any]:
    with tempfile.TemporaryDirectory(prefix="p3-rebuild-", dir=RESULTS) as temporary:
        output = Path(temporary) / "output"
        first = generate(SOURCE, output, "MicroduckNative12", "p3")
        preserved = (output / "overrides" / "user-overrides.json").read_bytes()
        second = generate(SOURCE, output, "MicroduckNative12", "p3")
        require(first["generationDigest"] == second["generationDigest"], "P3 clean rebuild is not deterministic")
        require((output / "overrides" / "user-overrides.json").read_bytes() == preserved, "P3 override changed during rebuild")
        return {"generationDigest": second["generationDigest"], "overridePreserved": True}


def main() -> int:
    started = time.perf_counter()
    RESULTS.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, str(SOURCE / "build_bundle.py")], check=True)
    ir = validate_bundle(SOURCE)
    lock = generate(SOURCE, OUTPUT, "MicroduckNative12", "p3")
    validate_project(OUTPUT)
    structure = verify_structure(lock)
    rebuild = verify_rebuild()
    mixed = verify_joint_types()
    run = run_project(OUTPUT, simulation_settings={"startTime": 0, "stopTime": 0.02, "outputInterval": 0.001, "relativeTolerance": 1e-8, "solver": "dassl", "solverFlags": {}})
    pose = verify_named_pose(run)
    summary = {
        "schemaVersion": "1.0", "passed": True,
        "completedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "durationSeconds": time.perf_counter() - started,
        "generator": lock["generator"], "generationDigest": lock["generationDigest"],
        "robotIr": ir, "structure": structure, "namedPose": pose,
        "jointTypeCoverage": mixed, "rebuild": rebuild, "omcRun": run.name,
        "scope": "P3 fixed/floating arbitrary tree generation and Microduck Native12 structural/pose acceptance.",
    }
    path = RESULTS / "p3-tree-generator-summary.json"
    write_json(path, summary)
    print(json.dumps({"passed": True, "summary": str(path), "maximumPositionError_m": pose["maximumPositionError_m"]}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
