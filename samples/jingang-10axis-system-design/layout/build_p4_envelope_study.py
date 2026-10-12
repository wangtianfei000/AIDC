"""Build the P4 preliminary FreeCAD assembly.

Run with FreeCADCmd. Geometry is deliberately a mixture of sourced envelopes and
planning proxies. It is not a manufacturing model.
"""

from __future__ import annotations

import json
from pathlib import Path

import FreeCAD as App
import Part


HERE = Path(__file__).resolve().parent
OUTPUT = HERE / "jingang-10axis-envelope-study.FCStd"
MANIFEST = HERE / "p4-envelope-study-manifest.json"

COLORS = {
    "structure": (0.72, 0.75, 0.80),
    "actuator": (0.18, 0.55, 0.72),
    "power": (0.90, 0.60, 0.16),
    "compute": (0.25, 0.68, 0.42),
    "control": (0.55, 0.42, 0.75),
    "sensor": (0.92, 0.35, 0.34),
    "harness": (0.88, 0.78, 0.22),
    "datum": (0.88, 0.20, 0.20),
    "envelope": (0.50, 0.65, 0.85),
}


def add_string(obj, name, value, group="P4 Traceability"):
    if name not in obj.PropertiesList:
        obj.addProperty("App::PropertyString", name, group)
    setattr(obj, name, value)


def add_float(obj, name, value, group="P4 Engineering"):
    if name not in obj.PropertiesList:
        obj.addProperty("App::PropertyFloat", name, group)
    setattr(obj, name, float(value))


def add_vector(obj, name, value, group="P4 Engineering"):
    if name not in obj.PropertiesList:
        obj.addProperty("App::PropertyVector", name, group)
    setattr(obj, name, App.Vector(*value))


def decorate(obj, *, component_id="", parent_id="", source_status="",
             verification_status="", geometry_role="planning-proxy",
             mass_g=0.0, com=None, assembly_relation=""):
    add_string(obj, "ComponentId", component_id)
    add_string(obj, "ParentComponentId", parent_id)
    add_string(obj, "SourceStatus", source_status)
    add_string(obj, "VerificationStatus", verification_status)
    add_string(obj, "GeometryRole", geometry_role)
    add_string(obj, "AssemblyRelation", assembly_relation)
    add_string(obj, "CadRef", f"layout://{component_id}" if component_id else "")
    add_string(obj, "ManufacturingStatus", "NOT_RELEASED_PRELIMINARY_LAYOUT")
    add_float(obj, "PlanningMass_g", mass_g)
    if com is not None:
        add_vector(obj, "PlanningCOMReference_mm", com)


def add_part(doc, parent, name, label, component_id, parent_id, relation, status):
    obj = doc.addObject("App::Part", name)
    obj.Label = label
    decorate(
        obj,
        component_id=component_id,
        parent_id=parent_id,
        source_status=status,
        verification_status="assembly hierarchy established; detailed interfaces pending",
        geometry_role="assembly-container",
        assembly_relation=relation,
    )
    if parent:
        parent.addObject(obj)
    return obj


def add_feature(doc, parent, name, label, shape, category, *, component_id="",
                parent_id="", source_status="planning proxy", verification_status="",
                geometry_role="planning-proxy", mass_g=0.0, com=None,
                assembly_relation="", transparency=0):
    obj = doc.addObject("Part::Feature", name)
    obj.Label = label
    obj.Shape = shape
    decorate(
        obj,
        component_id=component_id,
        parent_id=parent_id,
        source_status=source_status,
        verification_status=verification_status,
        geometry_role=geometry_role,
        mass_g=mass_g,
        com=com,
        assembly_relation=assembly_relation,
    )
    add_string(obj, "LayoutCategory", category)
    if parent:
        parent.addObject(obj)
    try:
        obj.ViewObject.ShapeColor = COLORS[category]
        obj.ViewObject.LineColor = (0.15, 0.18, 0.22)
        obj.ViewObject.Transparency = transparency
    except Exception:
        pass
    return obj


def box(x, y, z, sx, sy, sz):
    return Part.makeBox(sx, sy, sz, App.Vector(x, y, z))


def cylinder_between(a, b, radius):
    start = App.Vector(*a)
    end = App.Vector(*b)
    direction = end.sub(start)
    return Part.makeCylinder(radius, direction.Length, start, direction)


def finite_shape(obj):
    # App::Part exposes an aggregate Shape containing all descendants. It is a
    # hierarchy container, not an additional physical solid, so exclude it.
    if obj.TypeId != "Part::Feature" or not hasattr(obj, "Shape") or obj.Shape.isNull():
        return False
    bb = obj.Shape.BoundBox
    return max(abs(value) for value in (bb.XMin, bb.YMin, bb.ZMin, bb.XMax, bb.YMax, bb.ZMax)) < 1e6


def create_servo(doc, parent, name, component_id, center, joint_axis, limits, parent_id):
    # Public envelope: 34 x 20 x 23 mm. The 29 mm X envelope includes
    # provisional symmetric axial protrusions and is not an interface model.
    x, y, z = center
    shape = box(x - 14.5, y - 10.0, z - 17.0, 29.0, 20.0, 34.0)
    obj = add_feature(
        doc, parent, name, f"{component_id} | HD-1910-C001 sourced envelope",
        shape, "actuator", component_id=component_id, parent_id=parent_id,
        source_status="public 34x20x23 mm envelope and 22.5+/-2 g mass range; 29 mm axial overall is provisional",
        verification_status="body envelope available; shaft, spline, passive support, holes, connector and cable exit unverified",
        geometry_role="sourced-envelope-with-interface-assumptions", mass_g=22.5,
        com=center, assembly_relation="moving",
    )
    add_string(obj, "JointAxis", joint_axis)
    add_string(obj, "ZeroReference", "0 deg research neutral datum; physical calibration mark pending")
    add_float(obj, "LowerLimit_deg", limits[0])
    add_float(obj, "UpperLimit_deg", limits[1])
    add_float(obj, "BodyLength_mm", 34.0)
    add_float(obj, "BodyWidth_mm", 20.0)
    add_float(obj, "BodyThickness_mm", 23.0)
    add_float(obj, "ProvisionalOverallAxial_mm", 29.0)
    return obj


def add_axis(doc, parent, name, center, axis):
    direction = {"X": App.Vector(1, 0, 0), "Y": App.Vector(0, 1, 0), "Z": App.Vector(0, 0, 1)}[axis]
    origin = App.Vector(*center).sub(direction.multiply(18))
    return add_feature(
        doc, parent, name, f"{name} | research joint datum",
        Part.makeCylinder(1.2, 36, origin, direction), "datum",
        source_status="research kinematic axis from SysML architecture",
        verification_status="axis direction defined; physical shaft datum and zero require hardware drawing and jig verification",
        geometry_role="joint-datum", transparency=15,
    )


def main():
    if OUTPUT.exists():
        OUTPUT.unlink()
    doc = App.newDocument("JingangNative10_P4")

    root = add_part(
        doc, None, "SYS_JG10", "SYS-JG10 | Jingang Native10 | PRELIMINARY",
        "SYS-JG10", "", "fixed",
        "P4 planning assembly; not released for manufacturing",
    )
    add_string(root, "CoordinateSystem", "+X forward, +Y left, +Z up")
    add_string(root, "Configuration", "8 leg axes + 2 head/neck axes")
    add_float(root, "MassTarget_g", 800.0)
    add_float(root, "PlanningMass_g", 816.8)
    add_float(root, "MassMargin_g", -16.8)

    body = add_part(doc, root, "ASM_BODY", "ASM-BODY | torso frame", "ASM-BODY", "SYS-JG10", "fixed",
                    "program-generated structural planning geometry; dimensions not manufacturing frozen")
    left = add_part(doc, root, "ASM_LEG_L", "ASM-LEG-L | left leg", "ASM-LEG-L", "SYS-JG10", "moving",
                    "P4 neutral-pose skeleton and sourced actuator envelopes")
    right = add_part(doc, root, "ASM_LEG_R", "ASM-LEG-R | right leg", "ASM-LEG-R", "SYS-JG10", "moving",
                     "P4 neutral-pose skeleton and sourced actuator envelopes")
    head = add_part(doc, root, "ASM_HEAD_NECK", "ASM-HEAD-NECK | head and neck", "ASM-HEAD-NECK", "SYS-JG10", "moving",
                    "P4 planning geometry; camera/head SKU not selected")

    # Open torso frame leaves service and extraction space in +X.
    frame_shapes = [
        ("BodyBottom", box(-45, -62, 195, 90, 124, 4)),
        ("BodyRear", box(-45, -62, 195, 4, 124, 80)),
        ("BodyLeftRail", box(-41, 58, 199, 82, 4, 76)),
        ("BodyRightRail", box(-41, -62, 199, 82, 4, 76)),
        ("BodyTopRear", box(-41, -58, 271, 82, 116, 4)),
    ]
    for name, shape in frame_shapes:
        add_feature(doc, body, name, f"{name} | structure planning solid", shape, "structure",
                    parent_id="ASM-BODY", source_status="CAD-2 planning dimension",
                    verification_status="material, wall thickness, fasteners and stiffness not verified",
                    geometry_role="structural-planning-solid")

    # Category mass points keep the SysML CAD-2 budget traceable. Shape volume is not used as mass.
    structure_budget = doc.addObject("App::FeaturePython", "StructureMassBudget")
    structure_budget.Label = "Structure and foot mass budget | 349.8 g"
    decorate(structure_budget, parent_id="SYS-JG10", source_status="CAD-2 planning estimate",
             verification_status="requires CAD material assignment and physical weighing",
             geometry_role="mass-budget", mass_g=349.8, com=(0, 0, 115))
    root.addObject(structure_budget)

    joint_specs = {
        "L": [
            ("HipRoll", "ACT-L-HIP-ROLL", (0, 42, 180), "X", (-18, 18)),
            ("HipPitch", "ACT-L-HIP-PITCH", (0, 42, 145), "Y", (-35, 35)),
            ("KneePitch", "ACT-L-KNEE-PITCH", (0, 42, 78), "Y", (5, 65)),
            ("AnklePitch", "ACT-L-ANKLE-PITCH", (4, 42, 22), "Y", (-30, 30)),
        ],
        "R": [
            ("HipRoll", "ACT-R-HIP-ROLL", (0, -42, 180), "X", (-18, 18)),
            ("HipPitch", "ACT-R-HIP-PITCH", (0, -42, 145), "Y", (-35, 35)),
            ("KneePitch", "ACT-R-KNEE-PITCH", (0, -42, 78), "Y", (5, 65)),
            ("AnklePitch", "ACT-R-ANKLE-PITCH", (4, -42, 22), "Y", (-30, 30)),
        ],
    }
    for side, parent in (("L", left), ("R", right)):
        parent_id = f"ASM-LEG-{side}"
        specs = joint_specs[side]
        for short, cid, center, axis, limits in specs:
            create_servo(doc, parent, f"ACT_{side}_{short.upper()}", cid, center, axis, limits, parent_id)
            add_axis(doc, parent, f"Axis_{side}_{short}", center, axis)
        # Link proxies and feet. Contacts at joint envelopes are intentional.
        centers = [s[2] for s in specs]
        for index, (a, b) in enumerate(zip(centers[1:-1], centers[2:]), start=1):
            add_feature(doc, parent, f"Link_{side}_{index}", f"{side} leg link {index} | planning solid",
                        cylinder_between(a, b, 7), "structure", parent_id=parent_id,
                        source_status="kinematic planning length",
                        verification_status="section, material, bearings and fasteners unverified",
                        geometry_role="link-planning-solid")
        foot_y = 19 if side == "L" else -64
        add_feature(doc, parent, f"Foot_{side}", f"{side} foot | planning envelope",
                    box(-38, foot_y, 0, 92, 45, 8), "structure", parent_id=parent_id,
                    source_status="P4 support-polygon planning envelope",
                    verification_status="sole, stiffness, contact material and fasteners unverified",
                    geometry_role="foot-planning-solid")

    # Hip bracket interface check: 1 mm clearance on each side of the 20 mm servo body.
    for side, y in (("L", 42), ("R", -42)):
        parent = left if side == "L" else right
        parent_id = f"ASM-LEG-{side}"
        # HD-1910 planning envelope is 20 mm wide. Keep a visible 1 mm
        # clearance to each 2 mm bracket plate instead of modelling contact.
        for suffix, py in (("Inner", y - 13), ("Outer", y + 11)):
            plate = add_feature(doc, parent, f"HipBracket_{side}_{suffix}",
                                f"{side} hip bracket {suffix} | provisional interface",
                                box(-20, py, 160, 40, 2, 38), "structure", parent_id=parent_id,
                                source_status="P4 1 mm side-clearance assumption",
                                verification_status="plate spacing checked; holes, bearing seat, shaft and screw access unverified",
                                geometry_role="interface-planning-solid")
            add_float(plate, "NominalSideClearance_mm", 1.0)

    # Head and neck: two sourced actuator envelopes and a light head proxy.
    create_servo(doc, head, "ACT_NECK_PITCH", "ACT-NECK-PITCH", (0, 0, 292), "Y", (5, 35), "ASM-HEAD-NECK")
    add_axis(doc, head, "Axis_NeckPitch", (0, 0, 292), "Y")
    create_servo(doc, head, "ACT_HEAD_YAW", "ACT-HEAD-YAW", (0, 0, 330), "Z", (-60, 60), "ASM-HEAD-NECK")
    add_axis(doc, head, "Axis_HeadYaw", (0, 0, 330), "Z")
    add_feature(doc, head, "HeadEnvelope", "Head/camera envelope | planning proxy",
                box(-25, -22, 350, 50, 44, 35), "envelope", parent_id="ASM-HEAD-NECK",
                source_status="design reserve; final camera and shell not selected",
                verification_status="mass, lens, cable, fixation and field of view unverified",
                geometry_role="planning-envelope", transparency=35)

    power = add_feature(doc, root, "SYS_POWER", "SYS-POWER | battery reserve",
                        box(-35.5, -19, 202, 71, 38, 22), "power",
                        component_id="SYS-POWER", parent_id="SYS-JG10",
                        source_status="NP-F550 class design reserve; actual battery SKU not selected",
                        verification_status="envelope is planning reserve; connector, holder, capacity, protection and extraction grip unverified",
                        geometry_role="planning-envelope", mass_g=110.0, com=(0, 0, 213), assembly_relation="fixed")
    add_string(power, "ExtractionDirection", "+X after front access is open")
    add_float(power, "ExtractionTravel_mm", 115.0)

    compute = add_feature(doc, root, "SYS_COMPUTE", "SYS-COMPUTE | Radxa class envelope",
                          box(-32.5075, -15.753, 231, 65.015, 31.506, 12.987), "compute",
                          component_id="SYS-COMPUTE", parent_id="SYS-JG10",
                          source_status="Radxa ZERO 3W V1.11 published STEP bounding envelope recorded in CAD-2 audit; detailed STEP not bundled here",
                          verification_status="actual board revision, headers, heatsink, connectors and mounts unverified",
                          geometry_role="sourced-bounding-envelope", mass_g=30.0, com=(0, 0, 237.4935), assembly_relation="fixed")
    add_string(compute, "ExtractionDirection", "+X after front access is open")
    add_float(compute, "ExtractionTravel_mm", 115.0)

    control = add_feature(doc, root, "SYS_MOTION_CONTROL", "SYS-MOTION-CONTROL | controller reserve",
                          box(-25, -12.5, 248, 50, 25, 8), "control",
                          component_id="SYS-MOTION-CONTROL", parent_id="SYS-JG10",
                          source_status="P4 board-space reserve; SKU not selected",
                          verification_status="connectors, cooling, mounting and EMC unverified",
                          geometry_role="planning-envelope", mass_g=25.0, com=(0, 0, 252), assembly_relation="fixed")
    add_string(control, "ExtractionDirection", "+X after front access is open")
    add_float(control, "ExtractionTravel_mm", 115.0)

    sensing = add_feature(doc, root, "SYS_SENSING", "SYS-SENSING | IMU reserve",
                          box(-10, -5, 262, 20, 10, 6), "sensor",
                          component_id="SYS-SENSING", parent_id="SYS-JG10",
                          source_status="P4 IMU bridge space reserve; SKU not selected",
                          verification_status="sensor axes, vibration isolation, calibration and connector unverified",
                          geometry_role="planning-envelope", mass_g=7.0, com=(0, 0, 265), assembly_relation="fixed")

    # Cable-route reservations are layout guides, not manufactured harness geometry.
    harness = add_part(doc, root, "HarnessReserve", "Harness routing reserve", "", "SYS-JG10", "fixed",
                       "routing space only; wire gauges, connectors and bend radii pending")
    for name, a, b in [
        ("TrunkLeft", (30, 28, 217), (4, 42, 145)),
        ("TrunkRight", (30, -28, 217), (4, -42, 145)),
        ("TrunkHead", (24, 0, 235), (4, 0, 292)),
        ("LogicBus", (-20, 0, 237), (0, 0, 252)),
    ]:
        route = add_feature(doc, harness, name, f"{name} | cable routing reserve",
                            cylinder_between(a, b, 2.2), "harness",
                            source_status="P4 routing reserve",
                            verification_status="connector orientation, gauge, clamp points and moving slack unverified",
                            geometry_role="cable-routing-reserve", transparency=10)
        add_float(route, "ReservedRadius_mm", 2.2)
        add_float(route, "MinimumBendRadiusAssumption_mm", 12.0)

    # Remaining planning categories close the traceable 816.8 g SysML budget.
    bearing_budget = doc.addObject("App::FeaturePython", "BearingFastenerMassBudget")
    bearing_budget.Label = "Bearings and fasteners mass budget | 45 g"
    decorate(bearing_budget, parent_id="SYS-JG10", source_status="CAD-2 allocation",
             verification_status="BOM and physical mass unverified", geometry_role="mass-budget",
             mass_g=45.0, com=(0, 0, 95))
    root.addObject(bearing_budget)
    harness_budget = doc.addObject("App::FeaturePython", "HarnessConnectorMassBudget")
    harness_budget.Label = "Harness and connectors mass budget | 25 g"
    decorate(harness_budget, parent_id="SYS-JG10", source_status="CAD-2 allocation",
             verification_status="routing and physical mass unverified", geometry_role="mass-budget",
             mass_g=25.0, com=(0, 0, 155))
    root.addObject(harness_budget)

    doc.recompute()

    mass_objects = [
        o for o in doc.Objects
        if o is not root
        and "PlanningMass_g" in o.PropertiesList
        and o.PlanningMass_g > 0
        and "PlanningCOMReference_mm" in o.PropertiesList
    ]
    total_mass = sum(o.PlanningMass_g for o in mass_objects)
    weighted = App.Vector(0, 0, 0)
    for obj in mass_objects:
        weighted = weighted.add(obj.PlanningCOMReference_mm.multiply(obj.PlanningMass_g))
    com = weighted.multiply(1.0 / total_mass)
    add_vector(root, "PlanningCOM_mm", (com.x, com.y, com.z))
    add_float(root, "PlanningMass_g", total_mass)

    shapes = [o.Shape for o in doc.Objects if finite_shape(o)]
    compound = Part.makeCompound(shapes)
    bounds = compound.BoundBox
    add_vector(root, "LayoutEnvelope_mm", (bounds.XLength, bounds.YLength, bounds.ZLength))

    doc.recompute()
    doc.saveAs(str(OUTPUT))

    entries = []
    for obj in doc.Objects:
        if not finite_shape(obj):
            continue
        bb = obj.Shape.BoundBox
        entries.append({
            "name": obj.Name,
            "label": obj.Label,
            "componentId": getattr(obj, "ComponentId", ""),
            "parentComponentId": getattr(obj, "ParentComponentId", ""),
            "category": getattr(obj, "LayoutCategory", "structure"),
            "geometryRole": getattr(obj, "GeometryRole", ""),
            "sourceStatus": getattr(obj, "SourceStatus", ""),
            "bounds": [bb.XMin, bb.YMin, bb.ZMin, bb.XLength, bb.YLength, bb.ZLength],
        })
    manifest = {
        "schemaVersion": "p4-layout-1",
        "coordinateSystem": "+X forward, +Y left, +Z up",
        "manufacturingStatus": "NOT_RELEASED_PRELIMINARY_LAYOUT",
        "freecadVersion": App.Version(),
        "file": OUTPUT.name,
        "planningMass_g": total_mass,
        "massTarget_g": 800.0,
        "massMargin_g": 800.0 - total_mass,
        "planningCOM_mm": [com.x, com.y, com.z],
        "layoutEnvelope_mm": [bounds.XLength, bounds.YLength, bounds.ZLength],
        "objects": entries,
    }
    MANIFEST.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({k: manifest[k] for k in ("file", "planningMass_g", "massMargin_g", "planningCOM_mm", "layoutEnvelope_mm")}, ensure_ascii=False))
    App.closeDocument(doc.Name)


if __name__ == "__main__":
    main()
