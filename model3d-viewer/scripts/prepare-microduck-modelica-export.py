#!/usr/bin/env python3
"""Prepare the published Microduck hardware model with topology-aligned Robot IR."""
from __future__ import annotations

import json
import os
import shutil
import sys
from pathlib import Path
from typing import Any


HERE = Path(__file__).resolve()
MODEL3D = HERE.parents[1]
AIDC = MODEL3D.parent
SOURCE_GENERATED = MODEL3D / "apps" / "hardware-test" / "dist"
SOURCE_IR = AIDC / "modelica-browser" / "packages" / "robot-ir" / "examples" / "microduck-native12"
OUTPUT = MODEL3D / "test-artifacts" / "microduck-native12-export"
ID_MAP = {"neck": "neck_pitch_link", "head": "head_yaw_link"}

sys.path.insert(0, str(AIDC / "modelica-browser" / "packages" / "robot-ir" / "src"))
from robot_ir_validator import validate_bundle


def remap(value: Any) -> Any:
    if isinstance(value, dict):
        return {key: remap(item) for key, item in value.items()}
    if isinstance(value, list):
        return [remap(item) for item in value]
    return ID_MAP.get(value, value) if isinstance(value, str) else value


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def copy_or_link(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        if destination.stat().st_size == source.stat().st_size:
            return
        destination.unlink()
    try:
        os.link(source, destination)
    except OSError:
        shutil.copy2(source, destination)


def main() -> int:
    manifest = json.loads((SOURCE_GENERATED / "model3d.manifest.json").read_text(encoding="utf-8"))
    generated = OUTPUT / "generated"
    robot_ir = OUTPUT / "robot-ir"
    generated.mkdir(parents=True, exist_ok=True)
    robot_ir.mkdir(parents=True, exist_ok=True)
    for name in ("model3d.manifest.json", "kinematics.json"):
        shutil.copy2(SOURCE_GENERATED / name, generated / name)
    copy_or_link(SOURCE_GENERATED / "visual.glb", generated / "visual.glb")

    for name in ("physics.json", "devices.json", "interfaces.json"):
        document = remap(json.loads((SOURCE_IR / name).read_text(encoding="utf-8")))
        document["modelId"] = manifest["modelId"]
        document["modelRevision"] = manifest["revision"]
        write_json(robot_ir / name, document)
    summary = validate_bundle(robot_ir)
    write_json(OUTPUT / "pose-presets.json", {
        "schemaVersion": "aidc.model3d.pose-presets/0.1",
        "modelId": manifest["modelId"],
        "modelRevision": manifest["revision"],
        "poses": [],
    })
    print(json.dumps({"passed": True, "project": str(OUTPUT), "robotIr": summary}, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
