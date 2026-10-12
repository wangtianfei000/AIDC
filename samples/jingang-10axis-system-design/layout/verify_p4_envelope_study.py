"""Verify the P4 preliminary assembly and write a machine-readable report.

Run this file with FreeCADCmd. The checks intentionally distinguish layout
screening from manufacturing or detailed mechanism validation.
"""

from __future__ import annotations

import json
import math
import re
from pathlib import Path

import FreeCAD as App


HERE = Path(__file__).resolve().parent
MODEL = HERE / "jingang-10axis-envelope-study.FCStd"
SYSML = HERE.parent / "architecture.sysml"
OUTPUT = HERE / "p4-envelope-study-validation.json"


def finite_shape(obj):
    if obj.TypeId != "Part::Feature" or not hasattr(obj, "Shape") or obj.Shape.isNull():
        return False
    bb = obj.Shape.BoundBox
    return max(abs(value) for value in (bb.XMin, bb.YMin, bb.ZMin, bb.XMax, bb.YMax, bb.ZMax)) < 1e6


def overlap_volume(a, b):
    return a.Shape.common(b.Shape).Volume


def extraction_is_clear(moving, blockers, travel=115.0, step=5.0):
    samples = []
    for index in range(1, int(travel // step) + 1):
        distance = index * step
        moved = moving.Shape.copy()
        moved.translate(App.Vector(distance, 0, 0))
        collisions = []
        for blocker in blockers:
            volume = moved.common(blocker.Shape).Volume
            if volume > 1e-5:
                collisions.append({"object": blocker.Name, "volume_mm3": volume})
        samples.append({"distance_mm": distance, "collisions": collisions})
    return samples


def planar_leg_pose(hip_deg, knee_deg, ankle_deg, y):
    """Return a conservative sagittal skeleton from the neutral hip-pitch datum."""
    hip = (0.0, y, 145.0)
    thigh = 67.0
    shin = 56.0
    h = math.radians(hip_deg)
    k = math.radians(hip_deg - knee_deg)
    knee = (hip[0] + thigh * math.sin(h), y, hip[2] - thigh * math.cos(h))
    ankle = (knee[0] + shin * math.sin(k), y, knee[2] - shin * math.cos(k))
    foot_pitch = hip_deg - knee_deg + ankle_deg
    return {
        "hip_mm": list(hip),
        "knee_mm": list(knee),
        "ankle_mm": list(ankle),
        "footPitch_deg": foot_pitch,
        "groundClearance_mm": ankle[2] - 17.0,
    }


def main():
    doc = App.openDocument(str(MODEL))
    failures = []
    warnings = []

    expected_ids = re.findall(r'componentId\s*:>\s*String\s*=\s*"([^"]+)"', SYSML.read_text(encoding="utf-8"))
    cad_by_id = {}
    duplicates = []
    for obj in doc.Objects:
        component_id = getattr(obj, "ComponentId", "")
        if not component_id:
            continue
        if component_id in cad_by_id:
            duplicates.append(component_id)
        cad_by_id[component_id] = obj
    missing = sorted(set(expected_ids) - set(cad_by_id))
    extras = sorted(set(cad_by_id) - set(expected_ids))
    if missing or extras or duplicates:
        failures.append("SysML/CAD component ID mapping is not one-to-one")

    root = cad_by_id["SYS-JG10"]
    if root.ManufacturingStatus != "NOT_RELEASED_PRELIMINARY_LAYOUT":
        failures.append("assembly is not marked as preliminary")

    actuators = [obj for cid, obj in cad_by_id.items() if cid.startswith("ACT-")]
    incomplete_joints = [
        obj.ComponentId for obj in actuators
        if not all(name in obj.PropertiesList for name in ("JointAxis", "ZeroReference", "LowerLimit_deg", "UpperLimit_deg"))
    ]
    if len(actuators) != 10 or incomplete_joints:
        failures.append("ten-axis joint contract is incomplete")

    missing_provenance = [
        cid for cid, obj in cad_by_id.items()
        if not getattr(obj, "SourceStatus", "") or not getattr(obj, "VerificationStatus", "")
    ]
    if missing_provenance:
        failures.append("one or more CAD objects lack source/verification status")

    invalid_shapes = [obj.Name for obj in doc.Objects if finite_shape(obj) and not obj.Shape.isValid()]
    if invalid_shapes:
        failures.append("invalid B-Rep shapes found")

    hip_interfaces = []
    for side in ("L", "R"):
        servo = doc.getObject(f"ACT_{side}_HIPPITCH")
        inner = doc.getObject(f"HipBracket_{side}_Inner")
        outer = doc.getObject(f"HipBracket_{side}_Outer")
        ordered = sorted([inner.Shape.BoundBox, servo.Shape.BoundBox, outer.Shape.BoundBox], key=lambda b: b.YMin)
        clearances = [ordered[1].YMin - ordered[0].YMax, ordered[2].YMin - ordered[1].YMax]
        passed = all(abs(value - 1.0) < 1e-6 for value in clearances)
        if not passed:
            failures.append(f"{side} hip planning clearance differs from 1 mm")
        hip_interfaces.append({
            "side": side,
            "sideClearance_mm": clearances,
            "status": "planning-clearance-passed" if passed else "failed",
            "remaining": "holes, shaft, spline, passive support, fastener access and load path are unverified",
        })

    electronics = [doc.getObject(name) for name in ("SYS_POWER", "SYS_COMPUTE", "SYS_MOTION_CONTROL", "SYS_SENSING")]
    electronics_collisions = []
    for index, first in enumerate(electronics):
        for second in electronics[index + 1:]:
            volume = overlap_volume(first, second)
            if volume > 1e-5:
                electronics_collisions.append({"objects": [first.Name, second.Name], "volume_mm3": volume})
    if electronics_collisions:
        failures.append("electronics planning envelopes overlap")

    structure_blockers = [
        doc.getObject(name) for name in ("BodyBottom", "BodyRear", "BodyLeftRail", "BodyRightRail", "BodyTopRear")
    ]
    extraction_results = []
    for moving in electronics[:3]:
        blockers = structure_blockers + [other for other in electronics if other is not moving]
        samples = extraction_is_clear(moving, blockers, float(moving.ExtractionTravel_mm))
        collisions = [sample for sample in samples if sample["collisions"]]
        if collisions:
            failures.append(f"{moving.ComponentId} +X extraction path is obstructed")
        extraction_results.append({
            "componentId": moving.ComponentId,
            "direction": "+X",
            "travel_mm": float(moving.ExtractionTravel_mm),
            "sampleIncrement_mm": 5.0,
            "status": "clear" if not collisions else "obstructed",
            "collisions": collisions,
            "assumption": "front access cover and external keep-out are open",
        })

    poses = {
        "neutral": {"left": (0, 5, 5), "right": (0, 5, 5)},
        "crouch": {"left": (28, 55, 27), "right": (28, 55, 27)},
        "left_step": {"left": (-20, 25, 15), "right": (18, 38, 18)},
    }
    pose_results = []
    for name, sides in poses.items():
        result = {"name": name, "legs": {}}
        for side, angles in sides.items():
            y = 42.0 if side == "left" else -42.0
            result["legs"][side] = planar_leg_pose(*angles, y)
            limits_ok = -35 <= angles[0] <= 35 and 5 <= angles[1] <= 65 and -30 <= angles[2] <= 30
            clearance_ok = result["legs"][side]["groundClearance_mm"] >= 0
            result["legs"][side]["withinResearchLimits"] = limits_ok
            result["legs"][side]["skeletonGroundClear"] = clearance_ok
            if not limits_ok or not clearance_ok:
                failures.append(f"pose {name}/{side} fails planning kinematic screen")
        result["status"] = "planning-screen-passed"
        pose_results.append(result)

    harness_routes = [
        obj for obj in doc.Objects
        if getattr(obj, "GeometryRole", "") == "cable-routing-reserve"
    ]
    bad_routes = [
        obj.Name for obj in harness_routes
        if "MinimumBendRadiusAssumption_mm" not in obj.PropertiesList
        or obj.MinimumBendRadiusAssumption_mm < 12.0
    ]
    if len(harness_routes) < 4 or bad_routes:
        failures.append("cable routing reserves are incomplete")
    warnings.append("Cable routes prove reserved space only; moving slack, clamps, gauge and connector exits remain unverified.")

    mass_objects = [
        obj for obj in doc.Objects
        if obj is not root and "PlanningMass_g" in obj.PropertiesList
        and obj.PlanningMass_g > 0 and "PlanningCOMReference_mm" in obj.PropertiesList
    ]
    total_mass = sum(obj.PlanningMass_g for obj in mass_objects)
    if abs(total_mass - 816.8) > 1e-6 or abs(root.PlanningMass_g - total_mass) > 1e-6:
        failures.append("planning mass does not match the SysML CAD-2 budget")
    if total_mass > root.MassTarget_g:
        warnings.append("Planning mass is 16.8 g above the 800 g target; this is an explicit failed requirement, not a CAD error.")
    envelope = [root.LayoutEnvelope_mm.x, root.LayoutEnvelope_mm.y, root.LayoutEnvelope_mm.z]
    if min(envelope) <= 0 or max(envelope) > 1000:
        failures.append("layout envelope is invalid")

    report = {
        "schemaVersion": "p4-validation-1",
        "model": MODEL.name,
        "passed": not failures,
        "assessmentScope": "preliminary layout screening; not manufacturing release",
        "manufacturingStatus": root.ManufacturingStatus,
        "failures": failures,
        "warnings": warnings,
        "sysmlCadTraceability": {
            "expectedCount": len(expected_ids),
            "cadCount": len(cad_by_id),
            "missing": missing,
            "extras": extras,
            "duplicates": duplicates,
        },
        "kinematicContract": {
            "actuatorCount": len(actuators),
            "incompleteJointObjects": incomplete_joints,
            "coordinateSystem": root.CoordinateSystem,
            "poseScreening": pose_results,
            "scope": "planar skeleton samples within research angle windows; continuous swept-volume collision is still required after interface geometry is frozen",
        },
        "hipInterface": hip_interfaces,
        "electronics": {
            "internalEnvelopeCollisions": electronics_collisions,
            "extractionPaths": extraction_results,
        },
        "harness": {
            "routeReserveCount": len(harness_routes),
            "minimumBendRadiusAssumption_mm": 12.0,
            "badRoutes": bad_routes,
        },
        "massAndEnvelope": {
            "planningMass_g": total_mass,
            "targetMass_g": root.MassTarget_g,
            "margin_g": root.MassTarget_g - total_mass,
            "planningCOM_mm": [root.PlanningCOM_mm.x, root.PlanningCOM_mm.y, root.PlanningCOM_mm.z],
            "layoutEnvelope_mm": envelope,
        },
        "shapeValidation": {"invalidShapes": invalid_shapes},
        "openVerificationItems": [
            "HD-1910 shaft, spline, mounting holes, passive support, connector and cable exit",
            "battery, controller and IMU final SKU, mounting and connector envelopes",
            "continuous motion-envelope collision and harness-flex analysis",
            "fastener tool access, stiffness, strength, tolerances and physical fit",
            "material assignment, detailed BOM and physical mass/COM measurement",
        ],
    }
    OUTPUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({
        "passed": report["passed"],
        "componentIds": report["sysmlCadTraceability"],
        "massAndEnvelope": report["massAndEnvelope"],
        "failures": failures,
    }, ensure_ascii=False))
    App.closeDocument(doc.Name)
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
