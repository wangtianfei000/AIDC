#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import shutil
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve()
PACKAGE = HERE.parents[1]
ROOT = HERE.parents[3]
sys.path.insert(0, str(PACKAGE / "src"))
sys.path.insert(0, str(ROOT / "tools"))
from assembly_package_validator import AssemblyPackageError, validate_package
from aidc_modelica import resolve_bundle


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def file_record(root: Path, relative: str) -> dict[str, object]:
    path = root / relative
    return {"path": relative, "sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "byteLength": path.stat().st_size}


def main() -> int:
    source_ir = ROOT / "packages" / "robot-ir" / "examples" / "single-joint"
    with tempfile.TemporaryDirectory(prefix="aidc-assembly-test-") as directory:
        root = Path(directory)
        for name in ("physics.json", "devices.json", "interfaces.json"):
            shutil.copy2(source_ir / name, root / name)
        (root / "geometry").mkdir()
        (root / "geometry" / "base.stl").write_bytes(b"solid base\nendsolid base\n")
        (root / "geometry" / "link.stl").write_bytes(b"solid link\nendsolid link\n")
        physics = json.loads((root / "physics.json").read_text(encoding="utf-8"))
        physics["geometryAssets"] = [
            {"assetId": "base.visual", "path": "geometry/base.stl", "format": "stl", "scale": [1, 1, 1]},
            {"assetId": "link.visual", "path": "geometry/link.stl", "format": "stl", "scale": [1, 1, 1]},
        ]
        physics["bodies"][0]["visualAssetId"] = "base.visual"
        physics["bodies"][1]["visualAssetId"] = "link.visual"
        write_json(root / "physics.json", physics)
        paths = ["physics.json", "devices.json", "interfaces.json", "geometry/base.stl", "geometry/link.stl"]
        manifest = {
            "schemaVersion": "aidc.robot.assembly/1.0",
            "packageId": "aidc.standard.single-joint.p0-r1",
            "modelId": "aidc.standard.single-joint",
            "revision": "p0-r1",
            "name": "Single Joint Assembly",
            "source": {"format": "STEP", "publisher": "assembly-contract-test", "publisherVersion": "1.0.0"},
            "files": [file_record(root, path) for path in paths],
        }
        write_json(root / "assembly.manifest.json", manifest)
        summary = validate_package(root, require_simulation_ready=True)
        assert summary["bodyCount"] == 2 and summary["jointCount"] == 1 and summary["simulationReady"]
        assert resolve_bundle(root) == root.resolve()
        (root / "geometry" / "base.stl").write_bytes(b"tampered")
        try:
            validate_package(root)
        except AssemblyPackageError as exc:
            assert "长度不符" in str(exc) or "摘要不符" in str(exc)
        else:
            raise AssertionError("被修改的装配资源必须被拒绝")
    print("minimal assembly package tests passed: five payload files accepted, tampering rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
