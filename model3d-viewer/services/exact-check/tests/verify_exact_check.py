"""Constructed OpenCascade acceptance fixtures: penetration, gap and contact."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
from pathlib import Path

import FreeCAD as App
import Part

SERVER = Path(__file__).resolve().parents[1] / "src" / "server.py"
spec = importlib.util.spec_from_file_location("aidc_exact_server", SERVER)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def matrix(tx_m: float):
    return [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, tx_m, 0, 0, 1]


with tempfile.TemporaryDirectory() as folder:
    path = Path(folder) / "fixtures.FCStd"
    doc = App.newDocument("V3ExactFixtures")
    for name in ("A", "B"):
        obj = doc.addObject("PartDesign::Feature", name)
        obj.Shape = Part.makeBox(10, 10, 10)
    doc.recompute(); doc.saveAs(str(path)); App.closeDocument(doc.Name)
    digest = hashlib.sha256(path.read_bytes()).hexdigest()

    def check(offset):
        return module.exact_check(path, {
            "schemaVersion": module.SCHEMA_VERSION, "modelId": "fixture", "modelRevision": "v3", "cadSha256": digest,
            "componentA": {"componentId": "A", "sourceObject": "A", "matrixWorld": matrix(0)},
            "componentB": {"componentId": "B", "sourceObject": "B", "matrixWorld": matrix(offset)},
            "tolerance": 0.0001, "pose": {"joint": offset},
        })

    penetration = check(0.005)
    gap = check(0.011)
    contact = check(0.010)
    assert penetration["status"] == "CONFIRMED" and penetration["commonVolume"] > 0
    assert gap["status"] == "CLEARED" and gap["minimumDistance"] > 0.0001
    assert contact["status"] == "CONFIRMED" and contact["minimumDistance"] == 0
    print(json.dumps({"penetration": penetration, "gap": gap, "contact": contact}, ensure_ascii=False))
