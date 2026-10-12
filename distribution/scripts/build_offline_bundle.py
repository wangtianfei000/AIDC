from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import zipfile
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def sha(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""): value.update(block)
    return value.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(); parser.add_argument("portable", type=Path); parser.add_argument("output", type=Path); parser.add_argument("--pack", type=Path, action="append", default=[])
    args = parser.parse_args(); portable = args.portable.resolve()
    if not (portable / "bin" / "aidc.exe").is_file(): parser.error("portable directory is invalid")
    staging = args.output.parent / (args.output.stem + ".staging")
    shutil.rmtree(staging, ignore_errors=True); shutil.copytree(portable, staging / "product")
    (staging / "packs").mkdir(); packs = []
    for source in args.pack:
        source = source.resolve(); target = staging / "packs" / source.name; shutil.copy2(source, target)
        packs.append({"path": f"packs/{source.name}", "bytes": target.stat().st_size, "sha256": sha(target)})
    for name in ("install.ps1", "uninstall.ps1"):
        shutil.copy2(ROOT / "distribution" / "installer" / name, staging / name)
    manifest = {"schemaVersion": "aidc.offline-bundle/1.0", "createdAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"), "product": "product", "packs": packs}
    (staging / "offline-manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as archive:
        for path in sorted(item for item in staging.rglob("*") if item.is_file()): archive.write(path, path.relative_to(staging).as_posix())
    shutil.rmtree(staging); print(json.dumps({"output": str(args.output.resolve()), "packs": packs, "sha256": sha(args.output)}, ensure_ascii=False)); return 0


if __name__ == "__main__": raise SystemExit(main())
