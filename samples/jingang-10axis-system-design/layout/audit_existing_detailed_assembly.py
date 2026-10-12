"""Audit the existing detailed ten-axis CAD before adopting it as the P4 baseline."""

from __future__ import annotations

import json
from pathlib import Path

import FreeCAD as App


ROOT = Path(__file__).resolve().parents[4]
SOURCE = ROOT / "Jingang/apps/models/CAD/Microduck_Final/Microduck_10Axis_Final.FCStd"
OUTPUT = Path(__file__).resolve().parent / "detailed-assembly-audit.json"


def bounds(obj):
    if hasattr(obj, "Mesh") and obj.Mesh.CountFacets:
        bb = obj.Mesh.BoundBox
    elif hasattr(obj, "Shape") and not obj.Shape.isNull():
        bb = obj.Shape.BoundBox
    else:
        return None
    values = [bb.XMin, bb.YMin, bb.ZMin, bb.XLength, bb.YLength, bb.ZLength]
    if max(abs(value) for value in values) > 1e6:
        return None
    return values


def main():
    doc = App.openDocument(str(SOURCE))
    objects = []
    for obj in doc.Objects:
        bb = bounds(obj)
        if bb is None:
            continue
        objects.append({
            "name": obj.Name,
            "label": obj.Label,
            "type": obj.TypeId,
            "parents": [parent.Name for parent in obj.InList if parent.TypeId == "App::Part"],
            "bounds": bb,
            "facets": obj.Mesh.CountFacets if hasattr(obj, "Mesh") else None,
            "solidCount": len(obj.Shape.Solids) if hasattr(obj, "Shape") and not obj.Shape.isNull() else None,
            "properties": {
                key: str(getattr(obj, key))
                for key in (
                    "MotorModel", "HardwareModel", "GeometryProvenance", "FitStatus",
                    "BodyLength", "BodyWidth", "BodyThickness", "OverallThickness",
                    "MountHolePitchLong", "MountHolePitchShort", "HoleDiameter",
                ) if key in obj.PropertiesList
            },
        })
    servos = [obj for obj in objects if obj["name"].startswith("Mesh_xl")]
    printable = [obj for obj in objects if obj["type"] in ("Mesh::Feature", "PartDesign::Feature") and not obj["name"].startswith("Mesh_xl")]
    report = {
        "source": str(SOURCE.relative_to(ROOT)).replace("\\", "/"),
        "sourceBytes": SOURCE.stat().st_size,
        "documentObjects": len(doc.Objects),
        "finiteGeometryObjects": len(objects),
        "servoCount": len(servos),
        "printableOrCustomGeometryCount": len(printable),
        "servos": servos,
        "objects": objects,
    }
    OUTPUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("documentObjects", "finiteGeometryObjects", "servoCount", "printableOrCustomGeometryCount")}, ensure_ascii=False))
    App.closeDocument(doc.Name)


if __name__ == "__main__":
    main()
