#!/usr/bin/env python3
"""Build a small, schema-valid Model3D project used by the browser-to-Modelica test."""
from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

import trimesh
from jsonschema import Draft202012Validator


HERE = Path(__file__).resolve()
MODEL3D = HERE.parents[3]
AIDC = MODEL3D.parent
FIXTURE = MODEL3D / "test-artifacts" / "modelica-export-single-joint"
IR_SOURCE = AIDC / "modelica-browser" / "packages" / "robot-ir" / "examples" / "single-joint"
SCHEMAS = MODEL3D / "packages" / "contracts" / "schemas"


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate(document: dict, schema: str, path: Path) -> None:
    definition = json.loads((SCHEMAS / schema).read_text(encoding="utf-8"))
    errors = sorted(Draft202012Validator(definition).iter_errors(document), key=lambda item: list(item.absolute_path))
    if errors:
        raise RuntimeError("\n".join(f"{path}#/{'/'.join(map(str, e.absolute_path))}: {e.message}" for e in errors))


def build() -> Path:
    generated = FIXTURE / "generated"
    robot_ir = FIXTURE / "robot-ir"
    generated.mkdir(parents=True, exist_ok=True)
    robot_ir.mkdir(parents=True, exist_ok=True)
    for name in ("physics.json", "devices.json", "interfaces.json"):
        shutil.copy2(IR_SOURCE / name, robot_ir / name)

    scene = trimesh.Scene()
    scene.add_geometry(trimesh.creation.box(extents=(0.18, 0.14, 0.08)), node_name="base", geom_name="base-geometry")
    scene.add_geometry(
        trimesh.creation.box(extents=(0.26, 0.05, 0.05)),
        node_name="link",
        geom_name="link-geometry",
        parent_node_name="base",
        transform=trimesh.transformations.translation_matrix((0.13, 0.0, 0.0)),
    )
    visual = generated / "visual.glb"
    visual.write_bytes(scene.export(file_type="glb"))

    bodies = [
        {"bodyId": "base", "label": "Base", "nodeName": "base", "translation": [0, 0, 0], "rotation": [0, 0, 0, 1]},
        {"bodyId": "link", "label": "Link", "nodeName": "link", "parentBodyId": "base", "translation": [0, 0, 0], "rotation": [0, 0, 0, 1]},
    ]
    joints = [{
        "jointId": "joint-1", "type": "revolute", "parentBodyId": "base", "childBodyId": "link",
        "axis": [0, 0, 1], "neutralValue": 0, "zeroOffset": 0,
        "limit": {"lower": -1.5707963267948966, "upper": 1.5707963267948966, "velocity": 10},
        "drive": {"channel": "jointTorqueCommand", "sign": 1, "scale": 1},
    }]
    kinematics = {
        "schemaVersion": "aidc.model3d.kinematics/0.1", "modelId": "aidc.standard.single-joint",
        "modelRevision": "p0-r1", "rootBodyId": "base", "bodies": bodies, "joints": joints,
    }
    kinematics_path = generated / "kinematics.json"
    write_json(kinematics_path, kinematics)
    manifest = {
        "schemaVersion": "aidc.model3d.package/0.1", "modelId": "aidc.standard.single-joint", "revision": "p0-r1",
        "name": "AIDC Single Joint Export Fixture", "manufacturingStatus": "TEST_ONLY",
        "coordinateSystem": {"lengthUnit": "m", "angleUnit": "rad", "forward": "+X", "left": "+Y", "up": "+Z", "quaternionOrder": "xyzw"},
        "source": {"cadUri": "layout/single-joint.FCStd", "publisherVersion": "test-fixture/1.0"},
        "assets": [
            {"role": "visual", "uri": "visual.glb", "mediaType": "model/gltf-binary", "sha256": digest(visual), "byteLength": visual.stat().st_size},
            {"role": "kinematics", "uri": "kinematics.json", "mediaType": "application/json", "sha256": digest(kinematics_path), "byteLength": kinematics_path.stat().st_size},
        ],
        "rootBodyId": "base", "bodies": bodies,
        "components": [
            {"componentId": "base", "label": "Base", "nodeName": "base", "bodyId": "base", "geometryStatus": "test", "sourceObject": "base"},
            {"componentId": "link", "label": "Link", "nodeName": "link", "bodyId": "link", "geometryStatus": "test", "sourceObject": "link"},
        ],
        "productStructure": [
            {"nodeId": "base", "label": "Base", "nodeType": "collection", "nodeName": "base", "bodyId": "base"},
            {"nodeId": "link", "label": "Link", "nodeType": "part", "parentNodeId": "base", "nodeName": "link", "bodyId": "link"},
        ],
        "joints": joints,
        "statistics": {"sourceObjectCount": 2, "exportedGeometryCount": 2, "triangleCount": 24, "componentCount": 2, "jointCount": 1, "visualBounds": {"minimum": [-0.09, -0.07, -0.04], "maximum": [0.26, 0.07, 0.04]}},
    }
    manifest_path = generated / "model3d.manifest.json"
    validate(manifest, "model-package.schema.json", manifest_path)
    validate(kinematics, "kinematics.schema.json", kinematics_path)
    write_json(manifest_path, manifest)
    poses = {"schemaVersion": "aidc.model3d.pose-presets/0.1", "modelId": manifest["modelId"], "modelRevision": manifest["revision"], "poses": []}
    validate(poses, "pose-presets.schema.json", FIXTURE / "pose-presets.json")
    write_json(FIXTURE / "pose-presets.json", poses)
    return FIXTURE


if __name__ == "__main__":
    print(build())
