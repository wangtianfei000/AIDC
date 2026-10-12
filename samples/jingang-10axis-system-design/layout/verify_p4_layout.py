"""Validate the detailed P4 geometry baseline and record detailed-design open items."""

from __future__ import annotations

import csv
import hashlib
import json
import re
from pathlib import Path

import FreeCAD as App
import Mesh


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
MODEL = HERE / "jingang-10axis-layout.FCStd"
SYSML = HERE.parent / "architecture.sysml"
STL_DIR = ROOT / "Jingang/apps/models/CAD/Microduck_Final/STL"
STL_MANIFEST = STL_DIR / "STL_manifest.csv"
SOURCE_VALIDATION = STL_DIR / "STL_validation.json"
OUTPUT = HERE / "p4-validation.json"


def main():
    doc = App.openDocument(str(MODEL))
    geometry_failures = []
    blockers = []

    expected_ids = re.findall(r'componentId\s*:>\s*String\s*=\s*"([^"]+)"', SYSML.read_text(encoding="utf-8"))
    cad_ids = {}
    duplicates = []
    for obj in doc.Objects:
        cid = getattr(obj, "ComponentId", "")
        if not cid:
            continue
        if cid in cad_ids:
            duplicates.append(cid)
        cad_ids[cid] = obj
    missing = sorted(set(expected_ids) - set(cad_ids))
    extras = sorted(set(cad_ids) - set(expected_ids))
    if missing or extras or duplicates:
        geometry_failures.append("SysML/CAD component IDs are not one-to-one")

    servos = [obj for obj in doc.Objects if obj.Name.startswith("Mesh_xl")]
    servo_results = []
    for servo in servos:
        bb = servo.Mesh.BoundBox
        size = sorted([round(bb.XLength, 3), round(bb.YLength, 3), round(bb.ZLength, 3)])
        size_ok = all(abs(a - b) <= 0.02 for a, b in zip(size, [20.0, 29.0, 34.0]))
        contract_ok = all(name in servo.PropertiesList for name in ("ComponentId", "JointAxis", "ResearchZero", "ResearchLimits_deg"))
        if not size_ok or not contract_ok:
            geometry_failures.append(f"servo geometry/contract mismatch: {servo.Name}")
        servo_results.append({
            "name": servo.Name,
            "componentId": getattr(servo, "ComponentId", ""),
            "sizeSorted_mm": size,
            "facets": servo.Mesh.CountFacets,
            "geometryStatus": servo.GeometryStatus,
            "sizePassed": size_ok,
            "jointContractPassed": contract_ok,
        })
    if len(servos) != 10:
        geometry_failures.append(f"expected 10 servo objects, got {len(servos)}")

    print_results = []
    with STL_MANIFEST.open("r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    for row in rows:
        path = STL_DIR / row["file"]
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        mesh = Mesh.Mesh(str(path))
        bb = mesh.BoundBox
        actual = [bb.XLength, bb.YLength, bb.ZLength]
        expected = [float(row[key]) for key in ("size_x_mm", "size_y_mm", "size_z_mm")]
        size_ok = all(abs(a - b) <= 0.02 for a, b in zip(actual, expected))
        hash_ok = digest == row["sha256"]
        if not size_ok or not hash_ok:
            geometry_failures.append(f"print part mismatch: {row['file']}")
        print_results.append({
            "file": row["file"],
            "quantity": int(row["quantity"]),
            "size_mm": actual,
            "hashPassed": hash_ok,
            "sizePassed": size_ok,
            "watertightRecorded": row["watertight"] == "True",
            "singleSolidRecorded": row["single_solid"] == "True",
        })
    source_validation = json.loads(SOURCE_VALIDATION.read_text(encoding="utf-8"))
    if len(rows) != 30 or not source_validation.get("all_watertight") or not source_validation.get("all_single_solid"):
        geometry_failures.append("printable STL set is incomplete or failed source validation")

    status_by_id = {cid: getattr(obj, "GeometryStatus", "") for cid, obj in cad_ids.items()}
    if "CONFIGURATION_CONFLICT" in status_by_id.get("SYS-COMPUTE", ""):
        blockers.append("SYS-COMPUTE uses a Raspberry Pi Zero 2 W mesh while Radxa Zero 3W is also named in project references")
    if "CONFIGURATION_CONFLICT" in status_by_id.get("SYS-MOTION-CONTROL", ""):
        blockers.append("SYS-MOTION-CONTROL uses an RPI Robot HAT mesh while OpenRB-150 is also named as the first-machine controller")
    if "PLANNING_ENVELOPE" in status_by_id.get("SYS-SENSING", ""):
        blockers.append("SYS-SENSING is still a 25.8x15.5 mm planning envelope without a frozen IMU SKU")
    blockers.append("HD-1910 nominal drawing geometry still requires physical spline/thread/blind-hole and sample-fit verification")

    report = {
        "schemaVersion": "p4-detailed-validation-2",
        "model": MODEL.name,
        "geometryChecksPassed": not geometry_failures,
        "p4ExitReady": not geometry_failures,
        "manufacturingStatus": "NOT_RELEASED_DETAILED_REVIEW_REQUIRED",
        "geometryFailures": geometry_failures,
        "releaseBlockers": blockers,
        "sysmlCadTraceability": {
            "expectedCount": len(expected_ids),
            "cadCount": len(cad_ids),
            "missing": missing,
            "extras": extras,
            "duplicates": duplicates,
        },
        "servoGeometry": {
            "count": len(servos),
            "nominalSizeSet_mm": [20, 29, 34],
            "results": servo_results,
        },
        "printableParts": {
            "files": len(rows),
            "physicalParts": sum(int(row["quantity"]) for row in rows),
            "allWatertight": source_validation.get("all_watertight"),
            "allSingleSolid": source_validation.get("all_single_solid"),
            "results": print_results,
        },
        "configurationGeometryStatus": status_by_id,
    }
    OUTPUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({
        "geometryChecksPassed": report["geometryChecksPassed"],
        "p4ExitReady": report["p4ExitReady"],
        "componentIds": report["sysmlCadTraceability"],
        "servos": len(servos),
        "printFiles": len(rows),
        "physicalParts": report["printableParts"]["physicalParts"],
        "releaseBlockers": blockers,
    }, ensure_ascii=False))
    App.closeDocument(doc.Name)
    if geometry_failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
