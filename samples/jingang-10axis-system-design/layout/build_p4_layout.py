"""Build the detailed P4 assembly from the existing ten-axis mechanical CAD.

The source model contains the designed/printed shapes. This script adds SysML
traceability and provenance without replacing those shapes with envelopes.
"""

from __future__ import annotations

import csv
import hashlib
import json
import shutil
from pathlib import Path

import FreeCAD as App


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
SOURCE = ROOT / "Jingang/apps/models/CAD/Microduck_Final/Microduck_10Axis_Final.FCStd"
STL_DIR = SOURCE.parent / "STL"
STL_MANIFEST = STL_DIR / "STL_manifest.csv"
OUTPUT = HERE / "jingang-10axis-layout.FCStd"
MANIFEST = HERE / "p4-layout-manifest.json"
DETAIL_CSV = HERE / "p4-detailed-parts.csv"


def add_string(obj, name, value, group="P4 Detailed Traceability"):
    if name not in obj.PropertiesList:
        obj.addProperty("App::PropertyString", name, group)
    setattr(obj, name, value)


def finite_bounds(obj):
    if hasattr(obj, "Mesh") and obj.Mesh.CountFacets:
        bb = obj.Mesh.BoundBox
    elif hasattr(obj, "Shape") and not obj.Shape.isNull():
        bb = obj.Shape.BoundBox
    else:
        return None
    values = [bb.XMin, bb.YMin, bb.ZMin, bb.XLength, bb.YLength, bb.ZLength]
    return values if max(abs(value) for value in values) < 1e6 else None


def set_trace(obj, component_id, geometry_status, source, verification):
    add_string(obj, "ComponentId", component_id)
    add_string(obj, "CadRef", f"layout://{component_id}")
    add_string(obj, "GeometryStatus", geometry_status)
    add_string(obj, "GeometrySource", source)
    add_string(obj, "VerificationStatus", verification)
    add_string(obj, "ManufacturingStatus", "NOT_RELEASED_DETAILED_REVIEW_REQUIRED")


def validate_physical_hierarchy(doc, root_name, expected_labels):
    parts = {obj.Name: obj for obj in doc.Objects if obj.TypeId == "App::Part" and obj.Name in expected_labels}
    if set(parts) != set(expected_labels):
        missing = sorted(set(expected_labels) - set(parts))
        raise RuntimeError(f"physical hierarchy nodes missing: {missing}")
    roots = []
    for name, part in parts.items():
        parents = [parent for parent in part.InList if parent.TypeId == "App::Part" and parent.Name in parts]
        if name == root_name:
            if parents:
                raise RuntimeError(f"physical root has a parent: {root_name}")
            roots.append(name)
        elif len(parents) != 1:
            raise RuntimeError(f"physical node must have one parent: {name}; found {len(parents)}")
        if not str(part.Label).strip() or part.Label != expected_labels[name]:
            raise RuntimeError(f"physical label mismatch: {name}: {part.Label!r}")

    visited = set()

    def walk(part):
        if part.Name in visited:
            raise RuntimeError(f"physical hierarchy cycle or duplicate ownership at: {part.Name}")
        visited.add(part.Name)
        for child in part.Group:
            if child.TypeId == "App::Part" and child.Name in parts:
                walk(child)

    if roots != [root_name]:
        raise RuntimeError(f"physical hierarchy must have one root: {roots}")
    walk(parts[root_name])
    if visited != set(parts):
        raise RuntimeError(f"orphan physical nodes: {sorted(set(parts) - visited)}")


def main():
    doc = App.openDocument(str(SOURCE))

    # Physical assembly names and containment belong to the CAD model.  The
    # browser publisher reads these App::Part labels and Group relationships;
    # it must not maintain a second hierarchy in configuration.
    physical_labels = {
        "Microduck": "整机",
        "Body_trunk_base": "躯干总成",
        "Body_yaw2roll": "左髋偏航总成",
        "Body_hip_l": "左髋关节",
        "Body_upper_leg_left": "左大腿总成",
        "Body_leg": "左小腿总成",
        "Body_ankle_left": "左踝与足部",
        "Body_neck": "颈部总成",
        "Body_neck_pitch": "头部俯仰总成",
        "Body_yaw_roll_motion": "头部偏航总成",
        "Body_jaw_soft": "头部外壳与载荷",
        "Body_bearing_roll": "右髋偏航总成",
        "Body_hip_l_2": "右髋关节",
        "Body_upper_leg_right": "右大腿总成",
        "Body_leg_2": "右小腿总成",
        "Body_ankle_right": "右踝与足部",
    }
    for name, label in physical_labels.items():
        part = doc.getObject(name)
        if part is None or part.TypeId != "App::Part":
            raise RuntimeError(f"physical App::Part missing: {name}")
        part.Label = label
        add_string(part, "PhysicalStructureRole", "ASSEMBLY_NODE")

    component_map = {
        "SYS-JG10": ("Microduck", "DETAILED_ASSEMBLY_BASELINE"),
        "ASM-BODY": ("Body_trunk_base", "DETAILED_CUSTOM_GEOMETRY"),
        "ASM-LEG-L": ("Body_yaw2roll", "DETAILED_CUSTOM_GEOMETRY"),
        "ACT-L-HIP-ROLL": ("Mesh_xl332", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ACT-L-HIP-PITCH": ("Mesh_xl333", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ACT-L-KNEE-PITCH": ("Mesh_xl334", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ACT-L-ANKLE-PITCH": ("Mesh_xl335", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ASM-LEG-R": ("Body_bearing_roll", "DETAILED_CUSTOM_GEOMETRY"),
        "ACT-R-HIP-ROLL": ("Mesh_xl341", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ACT-R-HIP-PITCH": ("Mesh_xl342", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ACT-R-KNEE-PITCH": ("Mesh_xl343", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ACT-R-ANKLE-PITCH": ("Mesh_xl344", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ASM-HEAD-NECK": ("Body_neck", "DETAILED_CUSTOM_GEOMETRY"),
        "ACT-NECK-PITCH": ("Mesh_xl336", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "ACT-HEAD-YAW": ("Mesh_xl338", "DIMENSION_DRAWING_RECONSTRUCTION"),
        "SYS-POWER": ("Battery_6V4", "MEASURED_ENVELOPE_NO_SKU"),
        "SYS-COMPUTE": ("Mesh_pcb__raspberry_pi_zero_2_w", "REFERENCE_COMPONENT_MESH_CONFIGURATION_CONFLICT"),
        "SYS-MOTION-CONTROL": ("Mesh_elec_rpi_robot_hat_pcb", "REFERENCE_COMPONENT_MESH_CONFIGURATION_CONFLICT"),
        "SYS-SENSING": ("IMU_Board_Envelope", "PLANNING_ENVELOPE_SKU_UNRESOLVED"),
    }
    for component_id, (name, status) in component_map.items():
        obj = doc.getObject(name)
        if obj is None:
            raise RuntimeError(f"source object missing for {component_id}: {name}")
        if status == "DIMENSION_DRAWING_RECONSTRUCTION":
            source = "user-supplied HD-1910 dimension drawing; 34x20x23 mm body, 29 mm overall, 30x16 mm M2 pattern"
            verification = "nominal body and hole pattern modeled; spline, thread, blind-hole depth and physical fit require sample verification"
        elif status == "DETAILED_CUSTOM_GEOMETRY":
            source = "Microduck_10Axis_Final.FCStd and validated printable STL set"
            verification = "shape and exported mesh recorded; material, tolerance, load and physical fit still require engineering release"
        elif component_id == "SYS-POWER":
            source = "existing 6.4 V 1800 mAh physical-envelope record, 19x37x68 mm; SKU not recorded"
            verification = "measured envelope represented; connector, cell construction and procurement identity unresolved"
        elif component_id == "SYS-COMPUTE":
            source = "existing Raspberry Pi Zero 2 W reference mesh"
            verification = "geometry is detailed, but system configuration conflicts with Radxa Zero 3W references and must be selected"
        elif component_id == "SYS-MOTION-CONTROL":
            source = "existing Pollen RPI Robot HAT reference mesh"
            verification = "geometry is detailed, but implementation documents also name OpenRB-150; controller architecture must be selected"
        elif component_id == "SYS-SENSING":
            source = "25.8x15.5 mm IMU planning envelope"
            verification = "final IMU SKU, mounting holes, connector, axes and component height unresolved"
        else:
            source = "Microduck_10Axis_Final.FCStd detailed assembly"
            verification = "detailed baseline assembled; release blockers are recorded in p4-validation.json"
        set_trace(obj, component_id, status, source, verification)

    joint_contract = {
        "Mesh_xl332": ("local hip roll", -18, 18),
        "Mesh_xl333": ("local hip pitch", -35, 35),
        "Mesh_xl334": ("local knee pitch", 5, 65),
        "Mesh_xl335": ("local ankle pitch", -30, 30),
        "Mesh_xl341": ("local hip roll", -18, 18),
        "Mesh_xl342": ("local hip pitch", -35, 35),
        "Mesh_xl343": ("local knee pitch", 5, 65),
        "Mesh_xl344": ("local ankle pitch", -30, 30),
        "Mesh_xl336": ("local neck pitch", 5, 35),
        "Mesh_xl338": ("moving local head yaw", -60, 60),
    }
    for name, (axis, lower, upper) in joint_contract.items():
        obj = doc.getObject(name)
        add_string(obj, "JointAxis", axis)
        add_string(obj, "ResearchZero", "source CAD neutral placement; physical calibration pending")
        add_string(obj, "ResearchLimits_deg", f"{lower} .. {upper}")

    # Attach exact exported-part identity to every CAD object listed by the
    # manufacturing manifest. Combined STL rows intentionally annotate all
    # contributors; numbered ring instances inherit the matching base row.
    stl_rows = []
    with STL_MANIFEST.open("r", encoding="utf-8-sig", newline="") as handle:
        for row in csv.DictReader(handle):
            stl_path = STL_DIR / row["file"]
            digest = hashlib.sha256(stl_path.read_bytes()).hexdigest()
            if digest != row["sha256"]:
                raise RuntimeError(f"STL hash mismatch: {row['file']}")
            stl_rows.append(row)
            for object_name in row["cad_objects"].split("+"):
                candidates = [obj for obj in doc.Objects if obj.Name == object_name or obj.Name.startswith(object_name + "0")]
                for obj in candidates:
                    add_string(obj, "ExportedPartFile", row["file"])
                    add_string(obj, "ExportedPartSHA256", row["sha256"])
                    add_string(obj, "ExportedPartSize_mm", f"{row['size_x_mm']} x {row['size_y_mm']} x {row['size_z_mm']}")
                    add_string(obj, "GeometryStatus", "VALIDATED_PRINTABLE_STL")
                    add_string(obj, "ManufacturingStatus", "PRINT_GEOMETRY_REVIEWED_NOT_ASSEMBLY_RELEASED")

    root = doc.getObject("Microduck")
    add_string(root, "ConfigurationStatus", "DETAILED_GEOMETRY_BASELINE_WITH_ELECTRONICS_BLOCKERS")
    add_string(root, "CoordinateSystem", "source Microduck CAD coordinates; joint axes are local")
    doc.recompute()
    validate_physical_hierarchy(doc, "Microduck", physical_labels)
    doc.saveAs(str(OUTPUT))

    geometry = []
    for obj in doc.Objects:
        bb = finite_bounds(obj)
        if bb is None or obj.TypeId == "App::Part":
            continue
        geometry.append({
            "name": obj.Name,
            "label": obj.Label,
            "type": obj.TypeId,
            "componentId": getattr(obj, "ComponentId", ""),
            "geometryStatus": getattr(obj, "GeometryStatus", "SOURCE_MODEL_GEOMETRY"),
            "exportedPartFile": getattr(obj, "ExportedPartFile", ""),
            "bounds": bb,
            "facets": obj.Mesh.CountFacets if hasattr(obj, "Mesh") else None,
        })
    manifest = {
        "schemaVersion": "p4-detailed-assembly-2",
        "sourceModel": str(SOURCE.relative_to(ROOT)).replace("\\", "/"),
        "file": OUTPUT.name,
        "manufacturingStatus": "NOT_RELEASED_DETAILED_REVIEW_REQUIRED",
        "componentMap": {cid: name for cid, (name, _) in component_map.items()},
        "geometryObjectCount": len(geometry),
        "printFileCount": len(stl_rows),
        "geometry": geometry,
        "printFiles": stl_rows,
        "knownConfigurationBlockers": [
            "select Raspberry Pi Zero 2 W versus Radxa Zero 3W compute configuration",
            "select RPI Robot HAT versus OpenRB-150 motion-control architecture",
            "select exact IMU SKU and replace the planning envelope",
            "verify HD-1910 spline, thread, blind-hole depth and sample fit",
        ],
    }
    MANIFEST.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    shutil.copyfile(STL_MANIFEST, DETAIL_CSV)
    print(json.dumps({
        "file": OUTPUT.name,
        "geometryObjects": len(geometry),
        "printFiles": len(stl_rows),
        "componentIds": len(component_map),
    }, ensure_ascii=False))
    App.closeDocument(doc.Name)
    reopened = App.openDocument(str(OUTPUT))
    validate_physical_hierarchy(reopened, "Microduck", physical_labels)
    App.closeDocument(reopened.Name)


if __name__ == "__main__":
    main()
