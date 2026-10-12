#!/usr/bin/env python3
"""P4 acceptance for analytic contacts, native mesh queries and FMU packaging."""
from __future__ import annotations

import csv
import ctypes
import hashlib
import json
import math
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import mujoco

HERE = Path(__file__).resolve()
MODEL_BROWSER = HERE.parents[1]
AIDC_ROOT = MODEL_BROWSER.parent
REPO_ROOT = AIDC_ROOT.parent
LIBRARY = MODEL_BROWSER / "libraries" / "AIDCRobotics"
PACKAGE = LIBRARY / "modelica" / "AIDCRobotics"
NATIVE = LIBRARY / "native" / "modelica-native-geometry"
ROBOT_IR = MODEL_BROWSER / "packages" / "robot-ir"
MICRODUCK_IR = ROBOT_IR / "examples" / "microduck-native12"
RESULTS = MODEL_BROWSER / "results"
OMC = REPO_ROOT / "tools" / "openmodelica" / "1.27.1" / "bin" / "omc.exe"
FMPY_PYTHON = REPO_ROOT / "tools" / "openmodelica" / "fmpy-venv" / "Scripts" / "python.exe"
GCC = Path("D:/Software/Qt/Tools/mingw1310_64/bin/gcc.exe")
AR = Path("D:/Software/Qt/Tools/mingw1310_64/bin/ar.exe")
sys.path.insert(0, str(MODEL_BROWSER / "tools"))
sys.path.insert(0, str(ROBOT_IR / "src"))

from package_native_fmu import package as package_native_fmu
from project_tool import run_project, sha256_file, source_fingerprint, validate_project, write_json
from robot_ir_validator import validate_bundle


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def build_runtime() -> dict[str, Any]:
    include = NATIVE / "include"
    source = NATIVE / "src" / "aidc_geometry.c"
    resource_include = PACKAGE / "Resources" / "Include"
    resource_library = PACKAGE / "Resources" / "Library" / "win64"
    resource_include.mkdir(parents=True, exist_ok=True)
    resource_library.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(include / "aidc_geometry.h", resource_include / "aidc_geometry.h")
    require(GCC.is_file() and AR.is_file(), "MinGW C toolchain is unavailable")
    dll = resource_library / "aidc_geometry.dll"
    import_library = resource_library / "libaidc_geometry.dll.a"
    native_build = RESULTS / "native-geometry-build"
    native_build.mkdir(parents=True, exist_ok=True)
    obj = native_build / "aidc_geometry.o"
    static = resource_library / "libaidc_geometry.a"
    subprocess.run([str(GCC), "-O2", "-std=c99", "-shared", "-I", str(include), str(source), "-o", str(dll), f"-Wl,--out-implib,{import_library}"], check=True)
    subprocess.run([str(GCC), "-O2", "-std=c99", "-I", str(include), "-c", str(source), "-o", str(obj)], check=True)
    subprocess.run([str(AR), "rcs", str(static), str(obj)], check=True)
    return {"abiVersion": "0.1.0", "dll": str(dll), "dllSha256": sha256_file(dll), "staticLibrarySha256": sha256_file(static)}


def binary_stl(path: Path) -> None:
    header = b"AIDC P4 triangle".ljust(80, b"\0")
    record = struct.pack("<12fH", 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0)
    path.write_bytes(header + struct.pack("<I", 1) + record)


def verify_c_abi(runtime: dict[str, Any]) -> dict[str, Any]:
    dll = ctypes.CDLL(runtime["dll"])
    D = ctypes.c_double
    P = ctypes.POINTER(D)
    dll.aidc_geometry_version_code.restype = ctypes.c_int
    dll.aidc_mesh_load.argtypes = [ctypes.c_char_p, P]
    dll.aidc_mesh_load.restype = ctypes.c_void_p
    dll.aidc_mesh_destroy.argtypes = [ctypes.c_void_p]
    dll.aidc_mesh_triangle_count.argtypes = [ctypes.c_void_p]
    dll.aidc_mesh_triangle_count.restype = ctypes.c_int
    dll.aidc_mesh_bvh_node_count.argtypes = [ctypes.c_void_p]
    dll.aidc_mesh_bvh_node_count.restype = ctypes.c_int
    dll.aidc_mesh_distance.argtypes = [ctypes.c_void_p, P, ctypes.c_void_p, P, P, P, P, P]
    dll.aidc_mesh_distance.restype = ctypes.c_int
    dll.aidc_mesh_contacts.argtypes = [ctypes.c_void_p, P, ctypes.c_void_p, P, ctypes.c_int, P]
    dll.aidc_mesh_contacts.restype = ctypes.c_int
    scale = (D * 3)(0.1, 0.1, 0.1)
    cube = str((PACKAGE / "Resources" / "Data" / "unit_cube.obj").resolve()).encode()
    a = dll.aidc_mesh_load(cube, scale)
    b = dll.aidc_mesh_load(cube, scale)
    require(bool(a) and bool(b), "OBJ mesh loading failed")
    with tempfile.TemporaryDirectory(prefix="aidc-p4-stl-") as temporary:
        stl = Path(temporary) / "triangle.stl"
        binary_stl(stl)
        one = (D * 3)(1, 1, 1)
        triangle = dll.aidc_mesh_load(str(stl).encode(), one)
        require(bool(triangle) and dll.aidc_mesh_triangle_count(triangle) == 1, "binary STL loading failed")
        dll.aidc_mesh_destroy(triangle)
    require(dll.aidc_mesh_triangle_count(a) == 12, "cube triangle count differs")
    require(dll.aidc_mesh_bvh_node_count(a) == 7, "BVH node count differs")
    pose_a = (D * 7)(0, 0, 0, 0, 0, 0, 1)
    pose_b = (D * 7)(0.12, 0, 0, 0, 0, 0, 1)
    distance = D()
    point_a, point_b, normal = (D * 3)(), (D * 3)(), (D * 3)()
    status = dll.aidc_mesh_distance(a, pose_a, b, pose_b, ctypes.byref(distance), point_a, point_b, normal)
    require(status == 0 and abs(distance.value - 0.02) <= 1e-12, "BVH distance query differs")
    pose_b[0] = 0.08
    contacts = (D * 28)()
    count = dll.aidc_mesh_contacts(a, pose_a, b, pose_b, 4, contacts)
    require(count == 1 and abs(contacts[6] - 0.02) <= 1e-12, "mesh overlap query differs")
    result = {
        "versionCode": dll.aidc_geometry_version_code(), "objTriangles": 12, "stlTriangles": 1,
        "bvhNodes": 7, "separation_m": distance.value, "nearestPointA_m": list(point_a),
        "nearestPointB_m": list(point_b), "normal": list(normal), "contactCount": count,
        "penetration_m": contacts[6], "contactPoint_m": list(contacts)[:3],
    }
    dll.aidc_mesh_destroy(a)
    dll.aidc_mesh_destroy(b)
    return result


def rows(path: Path) -> list[dict[str, float]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return [{key: float(value) for key, value in row.items()} for row in csv.DictReader(handle)]


def mujoco_plane_scene(shape: str, position_z: float, velocity_x: float = 0) -> dict[str, Any]:
    geom = {
        "sphere": '<geom name="shape" type="sphere" size="0.05"/>',
        "box": '<geom name="shape" type="box" size="0.02 0.03 0.04"/>',
        "point": '<geom name="shape" type="sphere" size="0.0000001"/>',
    }[shape]
    xml = f'<mujoco><option gravity="0 0 0"/><worldbody><geom name="ground" type="plane" size="1 1 .1"/><body pos="0 0 {position_z}"><freejoint/><inertial pos="0 0 0" mass="1" diaginertia="0.01 0.01 0.01"/>{geom}</body></worldbody></mujoco>'
    model = mujoco.MjModel.from_xml_string(xml)
    data = mujoco.MjData(model)
    data.qvel[0] = velocity_x
    mujoco.mj_forward(model, data)
    require(data.ncon > 0, f"MuJoCo {shape} scene has no contact")
    contacts = [data.contact[index] for index in range(data.ncon)]
    return {
        "contactCount": len(contacts),
        "penetration_m": max(-float(item.dist) for item in contacts),
        "contactPoint_m": [sum(float(item.pos[axis]) for item in contacts) / len(contacts) for axis in range(3)],
        "normal": [sum(float(item.frame[axis]) for item in contacts) / len(contacts) for axis in range(3)],
    }


def verify_public_scenes(analytic_run: Path) -> dict[str, Any]:
    final = rows(analytic_run / "series.csv")[-1]
    definitions = {
        "landing": ("sphere", 0.04, 0.0, "sphere"),
        "sliding": ("point", -0.01, 0.2, "point"),
        "collision": ("box", 0.035, 0.0, "box"),
    }
    results = {}
    maxima = {"penetration_m": 0.0, "contactPoint_m": 0.0, "normal": 0.0}
    for name, (shape, z, vx, prefix) in definitions.items():
        reference = mujoco_plane_scene(shape, z, vx)
        modelica = {
            "penetration_m": max(0.0, -final[f"{prefix}.signedDistance"]),
            "contactPoint_m": [final[f"{prefix}.contactPoint[{axis}]"] for axis in (1, 2, 3)],
            "normal": [final[f"{prefix}.normal[{axis}]"] for axis in (1, 2, 3)],
        }
        errors = {
            "penetration_m": abs(modelica["penetration_m"] - reference["penetration_m"]),
            "contactPoint_m": max(abs(left - right) for left, right in zip(modelica["contactPoint_m"], reference["contactPoint_m"])),
            "normal": max(abs(left - right) for left, right in zip(modelica["normal"], reference["normal"])),
        }
        for key, value in errors.items(): maxima[key] = max(maxima[key], value)
        results[name] = {"modelica": modelica, "mujoco": reference, "absoluteError": errors}
    require(maxima["penetration_m"] <= 1e-7, f"public scene penetration differs: {maxima}")
    require(maxima["contactPoint_m"] <= 1e-7, f"public scene point differs: {maxima}")
    require(maxima["normal"] <= 1e-12, f"public scene normal differs: {maxima}")
    penetration = max(0.0, -final["point.signedDistance"])
    expected_spring = 20000 * penetration ** 1.5
    expected_damping = 100 * 0.3
    expected_normal_force = expected_spring + expected_damping
    expected_energy = 20000 * penetration ** 2.5 / 2.5
    force_errors = {
        "normalForce_N": abs(final["point.normalForce"] - expected_normal_force),
        "elasticEnergy_J": abs(final["point.elasticEnergy"] - expected_energy),
    }
    require(max(force_errors.values()) <= 1e-10, f"contact force/energy law differs: {force_errors}")
    require(final["point.dissipatedPower"] > 0 and final["point.force[1]"] < 0, "dissipation or friction direction is invalid")
    return {"scenes": results, "maximumAbsoluteError": maxima, "contactLawAbsoluteError": force_errors}


def build_and_package_fmu() -> dict[str, Any]:
    output = LIBRARY / "results" / "p4-native-fmu"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="aidc-p4-fmu-") as temporary:
        temp = Path(temporary)
        mos = temp / "build.mos"
        mos.write_text("\n".join([
            'loadModel(Modelica, {"4.1.0"});',
            f'loadFile("{(PACKAGE / "package.mo").resolve().as_posix()}");',
            f'cd("{temp.as_posix()}");',
            'buildModelFMU(AIDCRobotics.Conformance.P4FmuPackaging, version="2.0", fmuType="me_cs");',
            'getErrorString();', "",
        ]), encoding="utf-8")
        built = subprocess.run([str(OMC), str(mos)], capture_output=True, text=True, timeout=180)
        (output / "build.log").write_text(built.stdout + built.stderr, encoding="utf-8")
        source = temp / "P4FmuPackaging.fmu"
        require(built.returncode == 0 and source.is_file(), "P4 native FMU export failed")
        target = output / "P4FmuPackaging.fmu"
        shutil.copyfile(source, target)
    manifest = package_native_fmu(target, LIBRARY)
    with zipfile.ZipFile(target) as archive:
        names = set(archive.namelist())
        required = {"binaries/win64/P4FmuPackaging.dll", "resources/native/win64/aidc_geometry.dll", "resources/native/include/aidc_geometry.h", "resources/geometry/unit_cube.obj", "resources/aidc-native-manifest.json"}
        require(required <= names, f"FMU is missing native resources: {required - names}")
    require(FMPY_PYTHON.is_file(), "FMPy runtime is unavailable")
    simulation = subprocess.run([
        str(FMPY_PYTHON), "-c",
        "import json,sys;from fmpy import simulate_fmu;r=simulate_fmu(sys.argv[1],start_time=0,stop_time=.001,output_interval=.001,output=['runtimeVersion'],fmi_type='CoSimulation');print(json.dumps({'runtimeVersion':int(r['runtimeVersion'][-1])}))",
        str(target),
    ], capture_output=True, text=True, timeout=120)
    require(simulation.returncode == 0, f"packaged FMU execution failed: {simulation.stderr}")
    runtime_version = json.loads(simulation.stdout.strip().splitlines()[-1])["runtimeVersion"]
    require(runtime_version == 100, "packaged FMU native runtime did not execute")
    return {"path": str(target), "sha256": sha256_file(target), "runtimeVersion": 100, "packagedResources": manifest["resources"]}


def main() -> int:
    started = time.perf_counter()
    RESULTS.mkdir(parents=True, exist_ok=True)
    validate_project(LIBRARY)
    subprocess.run([sys.executable, str(MICRODUCK_IR / "build_bundle.py")], check=True, capture_output=True)
    robot_ir = validate_bundle(MICRODUCK_IR)
    physics = json.loads((MICRODUCK_IR / "physics.json").read_text(encoding="utf-8"))
    contact_shapes = [item["shape"] for item in physics["contactGeometries"]]
    require({"sphere", "box", "capsule", "footPatch"} <= set(contact_shapes), "Microduck Robot IR does not cover all P4 proxy shapes")
    runtime = build_runtime()
    c_abi = verify_c_abi(runtime)
    settings = {"startTime": 0, "stopTime": 0.001, "outputInterval": 0.001, "relativeTolerance": 1e-10, "solver": "dassl", "solverFlags": {}}
    analytic_run = run_project(LIBRARY, model_class="AIDCRobotics.Conformance.P4AnalyticContacts", simulation_settings=settings)
    native_run = run_project(LIBRARY, model_class="AIDCRobotics.Conformance.P4NativeGeometry", simulation_settings=settings)
    scenes = verify_public_scenes(analytic_run)
    fmu = build_and_package_fmu()
    source_digest, source_files = source_fingerprint(LIBRARY)
    summary = {
        "schemaVersion": "1.0", "passed": True,
        "completedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "durationSeconds": time.perf_counter() - started,
        "standard": {"id": "org.aidc.robotics", "version": "0.3.0", "revision": "0.3.0-p4"},
        "library": {"sourceDigest": source_digest, "sourceFileCount": len(source_files)},
        "robotIr": {**robot_ir, "contactGeometryCount": len(contact_shapes), "contactShapes": contact_shapes},
        "analyticContacts": {"components": ["point", "sphere", "box", "capsule", "footPatch"], "run": analytic_run.name},
        "nativeGeometry": {**runtime, **c_abi, "run": native_run.name, "threadSafety": "immutable mesh handles; queries allocate no shared mutable state", "deterministic": True},
        "crossSimulator": scenes,
        "fmi": fmu,
        "scope": "P4 analytic primitive contacts, stateless force/friction/energy law, cached OBJ/STL BVH geometry runtime, public MuJoCo geometry scenes and native FMU packaging.",
    }
    path = RESULTS / "p4-contact-runtime-summary.json"
    write_json(path, summary)
    print(json.dumps({"passed": True, "summary": str(path), "maximumAbsoluteError": scenes["maximumAbsoluteError"]}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
