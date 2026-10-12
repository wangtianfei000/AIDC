"""Create a deterministic V0 robot model package archive."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


def add_file(archive: ZipFile, source: Path, archive_name: str):
    info = ZipInfo(archive_name, date_time=(2026, 9, 19, 0, 0, 0))
    info.compress_type = ZIP_DEFLATED
    info.external_attr = 0o100644 << 16
    archive.writestr(info, source.read_bytes())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sample", type=Path, required=True)
    parser.add_argument("--contracts", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    sample = args.sample.resolve()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    files = [
        (sample / "model3d/generated/model3d.manifest.json", "model3d.manifest.json"),
        (sample / "model3d/generated/visual.glb", "visual.glb"),
        (sample / "model3d/generated/collision.glb", "collision.glb"),
        (sample / "model3d/generated/kinematics.json", "kinematics.json"),
        (sample / "model3d/generated/publish-report.json", "reports/publish-report.json"),
        (sample / "model3d/generated/validation-report.json", "reports/validation-report.json"),
        (sample / "model3d/assembly.sequence.json", "assembly.sequence.json"),
        (sample / "model3d/pose-presets.json", "pose-presets.json"),
        (sample / "model3d/simulation/neutral-frame.json", "simulation/neutral-frame.json"),
    ]
    for schema in sorted(args.contracts.resolve().glob("*.json")):
        files.append((schema, f"schemas/{schema.name}"))
    missing = [str(source) for source, _ in files if not source.is_file()]
    if missing:
        raise SystemExit("发布包缺少文件:\n" + "\n".join(missing))
    with ZipFile(output, "w") as archive:
        for source, archive_name in files:
            add_file(archive, source, archive_name)
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix(output.suffix + ".sha256").write_text(f"{digest}  {output.name}\n", encoding="utf-8")
    print(f"{output} ({output.stat().st_size} bytes, sha256={digest})")


if __name__ == "__main__":
    main()
