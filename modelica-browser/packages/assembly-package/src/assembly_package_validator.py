#!/usr/bin/env python3
"""Validate the minimal AIDC robot assembly delivery package."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from pathlib import Path
from typing import Any

from jsonschema import Draft202012Validator

HERE = Path(__file__).resolve()
MODEL_BROWSER = Path(os.environ.get("AIDC_MODELICA_ROOT", HERE.parents[3]))
PACKAGE_ROOT = Path(os.environ.get("AIDC_ASSEMBLY_PACKAGE_ROOT", MODEL_BROWSER / "packages" / "assembly-package"))
sys.path.insert(0, str(MODEL_BROWSER / "packages" / "robot-ir" / "src"))
from robot_ir_validator import validate_bundle


class AssemblyPackageError(RuntimeError):
    pass


def require(value: bool, message: str) -> None:
    if not value:
        raise AssemblyPackageError(message)


def read_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception as exc:
        raise AssemblyPackageError(f"无法读取 {path}: {exc}") from exc


def resolve_file(root: Path, relative: str) -> Path:
    path = (root / relative).resolve()
    require(path == root or root in path.parents, f"文件路径越过装配包边界: {relative}")
    return path


def validate_package(root: Path, *, require_simulation_ready: bool = False) -> dict[str, Any]:
    root = root.resolve()
    manifest_path = root / "assembly.manifest.json"
    require(manifest_path.is_file(), f"装配包缺少唯一入口 assembly.manifest.json: {root}")
    manifest = read_json(manifest_path)
    schema = read_json(PACKAGE_ROOT / "schemas" / "assembly-manifest.schema.json")
    errors = sorted(Draft202012Validator(schema).iter_errors(manifest), key=lambda item: list(item.absolute_path))
    if errors:
        raise AssemblyPackageError("\n".join(f"{manifest_path}#/{'/'.join(map(str, error.absolute_path))}: {error.message}" for error in errors))

    files: dict[str, dict[str, Any]] = {}
    for item in manifest["files"]:
        require(item["path"] not in files, f"重复的文件路径: {item['path']}")
        files[item["path"]] = item
        path = resolve_file(root, item["path"])
        require(path.is_file(), f"文件不存在: {item['path']}")
        require(path.stat().st_size == item["byteLength"], f"文件长度不符: {item['path']}")
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], f"文件摘要不符: {item['path']}")

    required = {"physics.json", "devices.json", "interfaces.json"}
    require(required <= set(files), "装配包必须包含 physics.json、devices.json 和 interfaces.json")
    ir_summary = validate_bundle(root)
    physics = read_json(root / "physics.json")
    require(ir_summary["modelId"] == manifest["modelId"] and physics["modelRevision"] == manifest["revision"], "装配清单与 Robot IR 身份不一致")

    geometry_paths = {asset["path"] for asset in physics.get("geometryAssets", [])}
    for relative in geometry_paths:
        require(relative in files, f"physics.json 引用的几何未登记到清单: {relative}")
    declared_geometry = {relative for relative in files if relative.startswith("geometry/")}
    require(declared_geometry == geometry_paths, "geometry/ 只能包含 physics.json 实际引用的运行时网格")

    actual_files = {path.relative_to(root).as_posix() for path in root.rglob("*") if path.is_file() and path.name != "assembly.manifest.json"}
    require(actual_files == set(files), "装配包含有未登记文件或清单登记不完整")

    return {"valid": True, "kind": "robot-assembly-package", "modelId": manifest["modelId"], "revision": manifest["revision"], "bodyCount": ir_summary["bodyCount"], "jointCount": ir_summary["jointCount"], "fileCount": len(files), "simulationReady": True, "robotIr": ir_summary}


def main() -> int:
    parser = argparse.ArgumentParser(description="校验最小 AIDC 机器人装配发布包")
    parser.add_argument("package", type=Path)
    parser.add_argument("--simulation-ready", action="store_true", help="为兼容旧调用保留；装配包始终必须仿真就绪")
    args = parser.parse_args()
    try:
        print(json.dumps(validate_package(args.package, require_simulation_ready=args.simulation_ready), ensure_ascii=False, indent=2))
        return 0
    except Exception as exc:
        print(json.dumps({"valid": False, "error": str(exc)}, ensure_ascii=False, indent=2))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
