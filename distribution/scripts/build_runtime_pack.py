from __future__ import annotations

import argparse
import hashlib
import json
import zipfile
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

PRESETS = {
    "freecad": {"packId": "cad-pack", "version": "1.1.3", "entryPoints": {"gui": "bin/FreeCAD.exe", "headless": "bin/FreeCADCmd.exe", "python": "bin/python.exe"}, "healthCheck": {"type": "command", "target": "bin/FreeCADCmd.exe", "arguments": ["--version"], "expected": "FreeCAD", "timeoutSeconds": 60}},
    "openmodelica": {"packId": "modelica-pack", "version": "1.27.1", "entryPoints": {"compiler": "bin/omc.exe", "simulator": "bin/OMSimulator.exe"}, "healthCheck": {"type": "command", "target": "bin/omc.exe", "arguments": ["--version"], "expected": "OpenModelica", "timeoutSeconds": 60}},
    "python-runtime": {"packId": "python-pack", "version": "3.12.9", "entryPoints": {"python": "python.exe"}, "healthCheck": {"type": "command", "target": "python.exe", "arguments": ["--version"], "expected": "Python 3.12.9", "timeoutSeconds": 30}},
}


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""): value.update(block)
    return value.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description="Build an AIDC runtime pack from an audited runtime directory")
    parser.add_argument("dependency", choices=sorted(PRESETS)); parser.add_argument("source", type=Path); parser.add_argument("output", type=Path)
    parser.add_argument("--version"); parser.add_argument("--pack-id")
    args = parser.parse_args(); source = args.source.resolve(); preset = PRESETS[args.dependency]
    if not source.is_dir(): parser.error(f"runtime source does not exist: {source}")
    required_paths = {*preset["entryPoints"].values(), preset["healthCheck"]["target"]}
    missing = sorted(path for path in required_paths if not (source / path).is_file())
    if missing: parser.error("runtime source is missing required entry points: " + ", ".join(missing))
    files = []
    for path in sorted((item for item in source.rglob("*") if item.is_file()), key=lambda item: item.relative_to(source).as_posix()):
        files.append({"path": path.relative_to(source).as_posix(), "size": path.stat().st_size, "sha256": digest(path)})
    manifest = {"schemaVersion": "aidc.runtime-pack/1.0", "packId": args.pack_id or preset["packId"], "dependencyId": args.dependency, "version": args.version or preset["version"], "platform": "windows", "architecture": "x86_64", "createdAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"), "entryPoints": preset["entryPoints"], "healthCheck": preset["healthCheck"], "files": files}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as archive:
        info = zipfile.ZipInfo("pack-manifest.json", (1980, 1, 1, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED; archive.writestr(info, json.dumps(manifest, ensure_ascii=False, indent=2).encode("utf-8"))
        for item in files:
            path = source / item["path"]
            info = zipfile.ZipInfo("payload/" + item["path"], (1980, 1, 1, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED
            with path.open("rb") as stream, archive.open(info, "w", force_zip64=True) as target:
                for block in iter(lambda: stream.read(1024 * 1024), b""): target.write(block)
    print(json.dumps({"output": str(args.output.resolve()), "dependencyId": args.dependency, "version": manifest["version"], "fileCount": len(files), "size": args.output.stat().st_size, "sha256": digest(args.output)}, ensure_ascii=False))
    return 0


if __name__ == "__main__": raise SystemExit(main())
