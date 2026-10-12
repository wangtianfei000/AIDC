#!/usr/bin/env python3
"""Attach the redistributable geometry runtime and assets to an exported FMU."""
from __future__ import annotations

import argparse
import hashlib
import json
import zipfile
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def package(fmu: Path, library: Path) -> dict:
    fmu = fmu.resolve()
    package_root = library.resolve() / "modelica" / "AIDCRobotics"
    files = {
        "resources/native/win64/aidc_geometry.dll": package_root / "Resources" / "Library" / "win64" / "aidc_geometry.dll",
        "resources/native/include/aidc_geometry.h": package_root / "Resources" / "Include" / "aidc_geometry.h",
        "resources/geometry/unit_cube.obj": package_root / "Resources" / "Data" / "unit_cube.obj",
    }
    missing = [str(path) for path in files.values() if not path.is_file()]
    if missing:
        raise FileNotFoundError("missing native FMU resources: " + ", ".join(missing))
    manifest = {
        "schemaVersion": "aidc.native-geometry-fmu/0.1",
        "abiVersion": "0.1.0",
        "coordinateSystem": "+X forward, +Y left, +Z up",
        "poseLayout": "x,y,z,qx,qy,qz,qw",
        "resources": [{"path": name, "sha256": sha256(path), "bytes": path.stat().st_size} for name, path in files.items()],
    }
    with zipfile.ZipFile(fmu, "a", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        existing = set(archive.namelist())
        for name, path in files.items():
            if name not in existing:
                archive.write(path, name)
        if "resources/aidc-native-manifest.json" not in existing:
            info = zipfile.ZipInfo("resources/aidc-native-manifest.json", date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("fmu", type=Path)
    parser.add_argument("library", type=Path)
    args = parser.parse_args()
    manifest = package(args.fmu, args.library)
    print(json.dumps(manifest, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
