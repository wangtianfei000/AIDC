#!/usr/bin/env python3
"""Product CLI for AIDC Robot IR and Modelica projects."""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve()
ROOT = Path(os.environ.get("AIDC_MODELICA_ROOT", HERE.parents[1]))
ROBOT_IR = ROOT / "packages" / "robot-ir"
ASSEMBLY_PACKAGE = ROOT / "packages" / "assembly-package"
LIBRARY = ROOT / "libraries" / "AIDCRobotics"
sys.path[:0] = [str(HERE.parent), str(ROBOT_IR / "src"), str(ASSEMBLY_PACKAGE / "src")]

from modelica_generator import GENERATOR_VERSION, GenerationError, default_overrides, generate
from project_tool import ContractError, normalized_text_digest, sha256_file, validate_project
from robot_ir_migrations import CURRENT, migrate_bundle
from robot_ir_validator import RobotIRError, validate_bundle
from stl_to_robot_ir import StlImportError, import_stl
from assembly_package_validator import AssemblyPackageError, validate_package

PRODUCT_VERSION = "0.1.0"
REQUEST_SCHEMA = "aidc.modelica.generation-request/0.1"


def emit(value: dict[str, Any], *, error: bool = False) -> None:
    print(json.dumps(value, ensure_ascii=False, indent=2), file=sys.stderr if error else sys.stdout)


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def resolve_bundle(source: Path) -> Path:
    source = source.resolve()
    assembly_manifest = source / "assembly.manifest.json"
    if assembly_manifest.is_file():
        validate_package(source, require_simulation_ready=True)
        return source
    candidates = [source, source / "robot-ir", source / "robot-ir" / "source"]
    candidates += [path.parent for path in source.rglob("physics.json") if len(path.relative_to(source).parts) <= 5]
    valid = []
    for candidate in candidates:
        if candidate not in valid and all((candidate / f"{name}.json").is_file() for name in ("physics", "devices", "interfaces")):
            valid.append(candidate)
    if not valid:
        raise ContractError(f"未在已发布模型中找到 Robot IR: {source}")
    if len(valid) > 1:
        shortest = min(valid, key=lambda item: len(item.parts))
        nested = [item for item in valid if item != shortest and shortest not in item.parents]
        if nested:
            raise ContractError("发布目录包含多个 Robot IR，请直接指定目标目录")
        return shortest
    return valid[0]


def has_legacy_schema(bundle: Path) -> bool:
    return any(read_json(bundle / f"{kind}.json").get("schemaVersion") != current for kind, current in CURRENT.items())


def diagnostics(exc: Exception) -> list[dict[str, Any]]:
    if isinstance(exc, GenerationError):
        return [{"severity": "error", "code": "AIDC-GEN-001", "message": str(exc), "source": {"path": exc.document, "jsonPointer": exc.pointer}}]
    lines = str(exc).splitlines() or [exc.__class__.__name__]
    result = []
    for line in lines:
        source: dict[str, Any] = {}
        if "#/" in line:
            path, rest = line.split("#/", 1)
            source = {"path": path, "jsonPointer": "/" + rest.split(": ", 1)[0]}
        result.append({"severity": "error", "code": "AIDC-VALIDATION-001", "message": line, **({"source": source} if source else {})})
    return result


def request_digest(bundle: Path, output: Path, package: str | None, profile: str) -> str:
    digest = hashlib.sha256()
    for name in ("physics.json", "devices.json", "interfaces.json"):
        digest.update(name.encode() + b"\0" + (bundle / name).read_bytes())
    physics = read_json(bundle / "physics.json")
    for asset in sorted(physics.get("geometryAssets", []), key=lambda item: item["assetId"]):
        asset_path = bundle / asset["path"]
        digest.update(asset["assetId"].encode() + b"\0" + asset_path.read_bytes())
    override = output / "overrides" / "user-overrides.json"
    override_value = read_json(override) if override.is_file() else default_overrides(profile)
    override_bytes = json.dumps(override_value, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode()
    digest.update(b"overrides\0" + override_bytes)
    digest.update(json.dumps({"package": package, "profile": profile, "generatorVersion": GENERATOR_VERSION}, sort_keys=True).encode())
    for path in sorted((LIBRARY / "modelica").rglob("*")):
        if path.is_file():
            digest.update(path.relative_to(LIBRARY).as_posix().encode() + b"\0" + hashlib.sha256(path.read_bytes()).digest())
    return "sha256:" + digest.hexdigest()


def outputs_intact(output: Path) -> bool:
    lock_path = output / "robot-ir" / "generation.lock.json"
    if not lock_path.is_file():
        return False
    try:
        lock = read_json(lock_path)
        def matches(item: dict[str, str]) -> bool:
            path = output / item["path"]
            if not path.is_file(): return False
            try: digest = normalized_text_digest(path)
            except UnicodeDecodeError: digest = sha256_file(path)
            return "sha256:" + digest == item["sha256"]
        return all(matches(item) for item in lock["outputs"])
    except Exception:
        return False


def choose_profile(bundle: Path, requested: str) -> str:
    if requested != "auto":
        return requested
    physics = read_json(bundle / "physics.json")
    return "p2" if len(physics.get("bodies", [])) == 2 and len(physics.get("joints", [])) == 1 and physics["joints"][0].get("type") == "revolute" else "p3"


def command_generate(args: argparse.Namespace) -> int:
    stl_temp: tempfile.TemporaryDirectory[str] | None = None
    stl_import = None
    if args.source.is_file() and args.source.suffix.lower() == ".stl":
        stl_temp = tempfile.TemporaryDirectory(prefix="aidc-stl-import-")
        bundle = Path(stl_temp.name)
        stl_import = import_stl(args.source, bundle, length_unit=args.stl_unit, density_kg_m3=args.density, root_type=args.root_type)
    else:
        bundle = resolve_bundle(args.source)
    migration = None
    temp: tempfile.TemporaryDirectory[str] | None = None
    try:
        if has_legacy_schema(bundle):
            if not args.migrate:
                raise ContractError("Robot IR schema 不是当前版本；使用 --migrate 执行确定性迁移")
            temp = tempfile.TemporaryDirectory(prefix="aidc-ir-migrate-")
            migrated = Path(temp.name)
            migration = migrate_bundle(bundle, migrated)
            bundle = migrated
        profile = choose_profile(bundle, args.profile)
        digest = request_digest(bundle, args.output.resolve(), args.package, profile)
        request_file = args.output.resolve() / "generation.request.json"
        previous = read_json(request_file) if request_file.is_file() else {}
        if not args.force and previous.get("requestDigest") == digest and outputs_intact(args.output.resolve()):
            lock = read_json(args.output.resolve() / "robot-ir" / "generation.lock.json")
            emit({"passed": True, "command": "generate", "cacheStatus": "hit", "output": str(args.output.resolve()), "generationDigest": lock["generationDigest"], "requestDigest": digest, "profile": profile, "stlImport": stl_import})
            return 0
        lock = generate(bundle, args.output, args.package, profile)
        request = {"schemaVersion": REQUEST_SCHEMA, "productVersion": PRODUCT_VERSION, "generatorVersion": GENERATOR_VERSION, "requestDigest": digest, "profile": profile, "package": lock["package"], "migration": migration}
        write_json(request_file, request)
        emit({"passed": True, "command": "generate", "cacheStatus": "miss", "output": str(args.output.resolve()), "generationDigest": lock["generationDigest"], "requestDigest": digest, "profile": profile, "migration": migration, "stlImport": stl_import})
        return 0
    finally:
        if temp:
            temp.cleanup()
        if stl_temp:
            stl_temp.cleanup()


def command_validate(args: argparse.Namespace) -> int:
    source = args.source.resolve()
    if (source / "model-project.json").is_file():
        loaded = validate_project(source)
        result = {"valid": True, "kind": "modelica-project", "projectId": loaded["project"]["projectId"]}
    elif (source / "assembly.manifest.json").is_file():
        result = validate_package(source)
    else:
        bundle = resolve_bundle(source)
        if has_legacy_schema(bundle):
            if not args.migrate:
                raise ContractError("Robot IR schema 不是当前版本；使用 --migrate 验证迁移结果")
            with tempfile.TemporaryDirectory(prefix="aidc-ir-validate-") as directory:
                report = migrate_bundle(bundle, Path(directory))
                result = {**validate_bundle(Path(directory)), "kind": "robot-ir", "migration": report}
        else:
            result = {**validate_bundle(bundle), "kind": "robot-ir"}
    emit({"passed": True, "command": "validate", **result, "diagnostics": []})
    return 0


def _csv_path(path: Path) -> Path | None:
    if path.is_file() and path.suffix.lower() == ".csv":
        return path
    for candidate in (path / "series.csv", path / "build" / "ModelRun_res.csv"):
        if candidate.is_file():
            return candidate
    return None


def _read_series(path: Path) -> tuple[list[str], list[list[float]]]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.reader(handle))
    if not rows:
        raise ContractError(f"空 CSV: {path}")
    return rows[0], [[float(cell) for cell in row] for row in rows[1:] if row]


def compare_series(left: Path, right: Path, variables: list[str], abs_tol: float, rel_tol: float) -> dict[str, Any]:
    lh, lr = _read_series(left); rh, rr = _read_series(right)
    if len(lr) != len(rr):
        raise ContractError(f"采样点数量不同: {len(lr)} != {len(rr)}")
    common = [name for name in lh if name in rh and name != "time"]
    selected = variables or common
    missing = [name for name in selected if name not in lh or name not in rh]
    if missing:
        raise ContractError(f"变量不存在: {', '.join(missing)}")
    metrics = []
    passed = True
    for name in ["time", *selected]:
        li, ri = lh.index(name), rh.index(name)
        max_abs = max((abs(a[li] - b[ri]) for a, b in zip(lr, rr)), default=0.0)
        scale = max((abs(a[li]) for a in lr), default=0.0)
        tolerance = abs_tol + rel_tol * scale
        item_passed = max_abs <= tolerance
        passed &= item_passed
        metrics.append({"name": name, "maxAbsoluteError": max_abs, "tolerance": tolerance, "passed": item_passed})
    return {"kind": "time-series", "left": str(left), "right": str(right), "sampleCount": len(lr), "variables": metrics, "passed": passed}


def compare_structure(left: Path, right: Path) -> dict[str, Any]:
    def locate(path: Path) -> Path:
        candidates = [path / "robot-ir" / "generated-structure.json", path / "generated-structure.json"]
        found = next((item for item in candidates if item.is_file()), None)
        if not found: raise ContractError(f"缺少 generated-structure.json: {path}")
        return found
    a, b = read_json(locate(left)), read_json(locate(right))
    fields = ("rootBodyId", "rootType", "bodies", "joints", "actuators", "namedPoses", "contactGeometries")
    differences = [{"field": field, "left": a.get(field), "right": b.get(field)} for field in fields if a.get(field) != b.get(field)]
    return {"kind": "generated-structure", "passed": not differences, "differences": differences}


def command_compare(args: argparse.Namespace) -> int:
    left_csv, right_csv = _csv_path(args.left), _csv_path(args.right)
    result = compare_series(left_csv, right_csv, args.variable, args.abs_tol, args.rel_tol) if left_csv and right_csv else compare_structure(args.left, args.right)
    emit({"command": "compare", **result}, error=not result["passed"])
    return 0 if result["passed"] else 2


def _copy_project(project: Path, staging: Path, include_results: bool) -> None:
    excluded = {".cache", "dist", "node_modules", "__pycache__"}
    if not include_results: excluded.add("results")
    for source in project.rglob("*"):
        relative = source.relative_to(project)
        if any(part in excluded for part in relative.parts): continue
        target = staging / relative
        if source.is_dir(): target.mkdir(parents=True, exist_ok=True)
        elif source.is_file(): target.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(source, target)


def command_package(args: argparse.Namespace) -> int:
    project = args.project.resolve(); validate_project(project)
    output = args.output.resolve(); output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="aidc-package-") as directory:
        staging = Path(directory) / "project"; staging.mkdir()
        _copy_project(project, staging, args.include_results)
        dependency_target = staging / "dependencies" / "AIDCRobotics" / "modelica" / "AIDCRobotics"
        shutil.copytree(LIBRARY / "modelica" / "AIDCRobotics", dependency_target, dirs_exist_ok=True)
        project_data = read_json(staging / "model-project.json")
        for dependency in project_data["modelica"]["libraries"]:
            if dependency["name"] == "AIDCRobotics": dependency["path"] = "dependencies/AIDCRobotics/modelica/AIDCRobotics/package.mo"
        write_json(staging / "model-project.json", project_data)
        validate_project(staging)
        entries = []
        for path in sorted((item for item in staging.rglob("*") if item.is_file()), key=lambda item: item.relative_to(staging).as_posix()):
            entries.append({"path": path.relative_to(staging).as_posix(), "sha256": "sha256:" + sha256_file(path), "bytes": path.stat().st_size})
        manifest = {"schemaVersion": "aidc.modelica.package/0.1", "productVersion": PRODUCT_VERSION, "projectId": project_data["projectId"], "entryPoint": "model-project.json", "files": entries}
        write_json(staging / "package-manifest.json", manifest)
        with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for path in sorted((item for item in staging.rglob("*") if item.is_file()), key=lambda item: item.relative_to(staging).as_posix()):
                info = zipfile.ZipInfo(path.relative_to(staging).as_posix(), (1980, 1, 1, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, path.read_bytes())
    emit({"passed": True, "command": "package", "output": str(output), "sha256": "sha256:" + sha256_file(output), "fileCount": len(entries) + 1})
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="aidc-modelica", description="AIDC Modelica product CLI")
    parser.add_argument("--version", action="version", version=f"aidc-modelica {PRODUCT_VERSION}")
    commands = parser.add_subparsers(dest="command", required=True)
    generate_parser = commands.add_parser("generate", help="从机器人装配发布包、Robot IR 或测试用单零件 STL 生成工程")
    generate_parser.add_argument("source", type=Path); generate_parser.add_argument("output", type=Path)
    generate_parser.add_argument("--package"); generate_parser.add_argument("--profile", choices=("auto", "p2", "p3"), default="auto")
    generate_parser.add_argument("--migrate", action="store_true"); generate_parser.add_argument("--force", action="store_true")
    generate_parser.add_argument("--stl-unit", choices=("mm", "m"), default="mm")
    generate_parser.add_argument("--density", type=float, default=1000.0, help="STL 材料密度，kg/m3")
    generate_parser.add_argument("--root-type", choices=("fixed", "floating"), default="fixed")
    generate_parser.set_defaults(func=command_generate)
    validate_parser = commands.add_parser("validate", help="验证机器人装配包、Robot IR 或 Modelica 工程")
    validate_parser.add_argument("source", type=Path); validate_parser.add_argument("--migrate", action="store_true"); validate_parser.set_defaults(func=command_validate)
    compare_parser = commands.add_parser("compare", help="比较生成结构或仿真时序")
    compare_parser.add_argument("left", type=Path); compare_parser.add_argument("right", type=Path); compare_parser.add_argument("--variable", action="append", default=[])
    compare_parser.add_argument("--abs-tol", type=float, default=1e-8); compare_parser.add_argument("--rel-tol", type=float, default=1e-6); compare_parser.set_defaults(func=command_compare)
    package_parser = commands.add_parser("package", help="生成可移植 .aidcmpkg")
    package_parser.add_argument("project", type=Path); package_parser.add_argument("output", type=Path); package_parser.add_argument("--include-results", action="store_true"); package_parser.set_defaults(func=command_package)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    try:
        if getattr(args, "abs_tol", 0) < 0 or getattr(args, "rel_tol", 0) < 0: raise ContractError("比较容差不能为负数")
        return args.func(args)
    except (ContractError, RobotIRError, AssemblyPackageError, GenerationError, StlImportError, OSError, ValueError, KeyError) as exc:
        emit({"passed": False, "command": getattr(args, "command", None), "diagnostics": diagnostics(exc)}, error=True)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
