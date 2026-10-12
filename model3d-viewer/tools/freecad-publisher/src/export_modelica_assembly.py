#!/usr/bin/env python3
"""Export a minimal AIDC assembly package from a Model3D project and reviewed Robot IR."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import tempfile
from pathlib import Path
from typing import Any

import numpy as np
import trimesh

HERE = Path(__file__).resolve()
AIDC_ROOT = HERE.parents[4]
MODEL_BROWSER = AIDC_ROOT / "modelica-browser"
sys.path[:0] = [
    str(MODEL_BROWSER / "packages" / "robot-ir" / "src"),
    str(MODEL_BROWSER / "packages" / "assembly-package" / "src"),
]
from assembly_package_validator import validate_package
from robot_ir_validator import validate_bundle


class ExportError(RuntimeError):
    pass


def read_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception as exc:
        raise ExportError(f"无法读取 {path}: {exc}") from exc


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value: bool, message: str) -> None:
    if not value:
        raise ExportError(message)


def normalized_joint(item: dict[str, Any]) -> tuple[str, str, str, str]:
    return item["jointId"], item["type"], item["parentBodyId"], item["childBodyId"]


def visual_asset_path(manifest_path: Path, manifest: dict[str, Any]) -> Path:
    asset = next((item for item in manifest.get("assets", []) if item.get("role") == "visual"), None)
    require(asset is not None, "三维模型清单缺少 visual 资产")
    relative = Path(asset["uri"])
    require(not relative.is_absolute() and ".." not in relative.parts, f"三维视觉资产路径非法: {asset['uri']}")
    path = (manifest_path.parent / relative).resolve()
    require(manifest_path.parent == path.parent or manifest_path.parent in path.parents, f"三维视觉资产越过发布目录: {asset['uri']}")
    require(path.is_file(), f"三维视觉资产不存在: {asset['uri']}")
    require(hashlib.sha256(path.read_bytes()).hexdigest() == asset["sha256"], f"三维视觉资产摘要不符: {asset['uri']}")
    return path


def export_body_geometry(manifest_path: Path, manifest: dict[str, Any], physics: dict[str, Any], staging: Path) -> None:
    """Create one body-local STL per rigid body and bind it into Robot IR."""
    visual = visual_asset_path(manifest_path, manifest)
    scene = trimesh.load(visual, force="scene")
    components: dict[str, list[dict[str, Any]]] = {}
    for component in manifest.get("components", []):
        components.setdefault(component["bodyId"], []).append(component)
    manifest_bodies = {item["bodyId"]: item for item in manifest["bodies"]}

    assets = list(physics.get("geometryAssets", []))
    asset_ids = {item["assetId"] for item in assets}
    for body in physics["bodies"]:
        if body.get("visualAssetId"):
            continue
        body_id = body["bodyId"]
        body_node = manifest_bodies[body_id]["nodeName"]
        require(body_node in scene.graph.nodes, f"GLB 缺少刚体节点: {body_node}")
        body_world = scene.graph[body_node][0]
        pieces = []
        for component in components.get(body_id, []):
            node = component["nodeName"]
            if node not in scene.graph.nodes:
                continue
            component_world, geometry_name = scene.graph[node]
            if geometry_name is None or geometry_name not in scene.geometry:
                continue
            mesh = scene.geometry[geometry_name].copy()
            mesh.apply_transform(np.linalg.inv(body_world) @ component_world)
            pieces.append(mesh)
        require(pieces, f"刚体 {body_id} 没有可导出的三维组件")
        merged = trimesh.util.concatenate(pieces)
        relative = Path("geometry") / f"{body_id}.stl"
        destination = staging / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        merged.export(destination, file_type="stl")
        asset_id = f"visual-{body_id}"
        require(asset_id not in asset_ids, f"自动生成的几何 assetId 冲突: {asset_id}")
        assets.append({"assetId": asset_id, "path": relative.as_posix(), "format": "stl", "scale": [1, 1, 1]})
        asset_ids.add(asset_id)
        body["visualAssetId"] = asset_id
    physics["geometryAssets"] = assets


def export_package(model3d_manifest: Path, robot_ir: Path, output: Path) -> dict[str, Any]:
    model3d_manifest, robot_ir, output = model3d_manifest.resolve(), robot_ir.resolve(), output.resolve()
    manifest = read_json(model3d_manifest)
    ir_summary = validate_bundle(robot_ir)
    physics = read_json(robot_ir / "physics.json")

    source_bodies = {item["bodyId"] for item in manifest.get("bodies", [])}
    target_bodies = {item["bodyId"] for item in physics["bodies"]}
    require(source_bodies == target_bodies, f"三维模型与 Robot IR 刚体集合不一致: only3d={sorted(source_bodies-target_bodies)}, onlyIR={sorted(target_bodies-source_bodies)}")
    source_joints = {normalized_joint(item) for item in manifest.get("joints", [])}
    target_joints = {normalized_joint(item) for item in physics["joints"]}
    require(source_joints == target_joints, "三维模型与 Robot IR 关节类型或父子关系不一致")
    require(manifest.get("rootBodyId") == physics["rootBodyId"], "三维模型与 Robot IR 根刚体不一致")

    if output.exists():
        try:
            existing = validate_package(output, require_simulation_ready=True)
            existing_physics = read_json(output / "physics.json")
            geometry_complete = all(body.get("visualAssetId") for body in existing_physics["bodies"])
            if existing["modelId"] == physics["modelId"] and existing["revision"] == physics["modelRevision"] and geometry_complete:
                return {**existing, "output": str(output), "cacheStatus": "hit", "geometryAssetCount": len(existing_physics.get("geometryAssets", []))}
            if existing["modelId"] == physics["modelId"] and existing["revision"] == physics["modelRevision"]:
                require(output.name.endswith(".aidc-assembly"), f"拒绝替换非装配包目录: {output}")
                shutil.rmtree(output)
            else:
                raise ExportError(f"输出目录已经存在且模型身份不同: {output}")
        except Exception:
            if output.exists():
                raise ExportError(f"输出目录已经存在且不是相同的有效装配包: {output}")

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f".{output.name}-", dir=output.parent) as directory:
        staging = Path(directory)
        for name in ("devices.json", "interfaces.json"):
            shutil.copy2(robot_ir / name, staging / name)
        for asset in physics.get("geometryAssets", []):
            relative = Path(asset["path"])
            require(not relative.is_absolute() and ".." not in relative.parts, f"Robot IR 几何路径非法: {asset['path']}")
            source = (robot_ir / relative).resolve()
            require(robot_ir == source or robot_ir in source.parents, f"Robot IR 几何越过输入目录: {asset['path']}")
            require(source.is_file(), f"Robot IR 几何不存在: {asset['path']}")
            destination = staging / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)

        export_body_geometry(model3d_manifest, manifest, physics, staging)
        write_json(staging / "physics.json", physics)

        payload = sorted((path for path in staging.rglob("*") if path.is_file()), key=lambda path: path.relative_to(staging).as_posix())
        source_info = manifest.get("source", {})
        source_format = Path(str(source_info.get("cadUri", ""))).suffix.lstrip(".") or "other"
        source_format = source_format if source_format in {"FCStd", "STEP", "JT", "glTF", "URDF"} else source_format.upper()
        if source_format not in {"FCStd", "STEP", "JT", "glTF", "URDF"}:
            source_format = "other"
        package_manifest = {
            "schemaVersion": "aidc.robot.assembly/1.0",
            "packageId": f"{physics['modelId']}.{physics['modelRevision']}",
            "modelId": physics["modelId"],
            "revision": physics["modelRevision"],
            "name": manifest.get("name") or physics["modelId"],
            "source": {
                "format": source_format,
                "publisher": "AIDC Model3D Viewer",
                "publisherVersion": "1.0.0",
                **({"uri": str(source_info["cadUri"])} if source_info.get("cadUri") else {}),
                **({"sha256": source_info["cadSha256"]} if source_info.get("cadSha256") else {}),
            },
            "files": [{"path": path.relative_to(staging).as_posix(), "sha256": sha256(path), "byteLength": path.stat().st_size} for path in payload],
        }
        write_json(staging / "assembly.manifest.json", package_manifest)
        summary = validate_package(staging, require_simulation_ready=True)
        os.replace(staging, output)
    return {**summary, "output": str(output), "cacheStatus": "miss", "sourceModelId": manifest.get("modelId"), "sourceRevision": manifest.get("revision"), "geometryAssetCount": len(physics.get("geometryAssets", [])), "robotIr": ir_summary}


def main() -> int:
    parser = argparse.ArgumentParser(description="从三维模型工程导出最小 AIDC 装配包")
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--robot-ir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        print(json.dumps({"passed": True, **export_package(args.manifest, args.robot_ir, args.output)}, ensure_ascii=False, indent=2))
        return 0
    except Exception as exc:
        print(json.dumps({"passed": False, "error": str(exc)}, ensure_ascii=False, indent=2), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
