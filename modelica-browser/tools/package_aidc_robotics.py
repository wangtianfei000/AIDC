#!/usr/bin/env python3
"""Create the deterministic standalone AIDCRobotics release archive."""
from __future__ import annotations

import argparse
import hashlib
import json
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIBRARY = ROOT / "libraries" / "AIDCRobotics"
VERSION = "0.3.0"


def main() -> int:
    parser = argparse.ArgumentParser(); parser.add_argument("output", type=Path, nargs="?", default=ROOT / "dist" / f"AIDCRobotics-{VERSION}.zip"); args = parser.parse_args()
    output = args.output.resolve(); output.parent.mkdir(parents=True, exist_ok=True)
    sources = []
    for base in (LIBRARY / "modelica", LIBRARY / "docs", LIBRARY / "native"):
        if base.exists(): sources.extend(path for path in base.rglob("*") if path.is_file() and "results" not in path.parts and "build" not in path.parts and "__pycache__" not in path.parts)
    entries = []
    for path in sorted(sources, key=lambda item: item.relative_to(LIBRARY).as_posix()):
        data = path.read_bytes(); entries.append({"path": path.relative_to(LIBRARY).as_posix(), "sha256": "sha256:" + hashlib.sha256(data).hexdigest(), "bytes": len(data)})
    manifest = {"schemaVersion": "aidc.modelica.library-release/0.1", "name": "AIDCRobotics", "version": VERSION, "modelicaVersion": "3.6", "modelicaStandardLibrary": "4.1.0", "openModelica": "1.27.1", "files": entries}
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(sources, key=lambda item: item.relative_to(LIBRARY).as_posix()):
            info = zipfile.ZipInfo(f"AIDCRobotics-{VERSION}/" + path.relative_to(LIBRARY).as_posix(), (1980, 1, 1, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED; archive.writestr(info, path.read_bytes())
        info = zipfile.ZipInfo(f"AIDCRobotics-{VERSION}/release-manifest.json", (1980, 1, 1, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED; archive.writestr(info, (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode())
    print(json.dumps({"passed": True, "output": str(output), "sha256": "sha256:" + hashlib.sha256(output.read_bytes()).hexdigest(), "fileCount": len(entries) + 1}, ensure_ascii=False))
    return 0


if __name__ == "__main__": raise SystemExit(main())
