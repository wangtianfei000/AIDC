#!/usr/bin/env python3
"""Import the frozen Grammar, generated ANTLR sources, and parser implementation."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path


DEFAULT_REFERENCE_ROOT = Path(r"D:\Document\7.AWorks\BlockSim\2.Development\SysML")
DEFAULT_REFERENCE_BUILD = (
    DEFAULT_REFERENCE_ROOT / "build" / "Desktop_Qt_6_8_1_MinGW_64_bit-Debug"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def content_sha256(path: Path) -> str:
    data = path.read_bytes()
    if b"\0" not in data:
        try:
            data.decode("utf-8")
            data = data.replace(b"\r\n", b"\n")
        except UnicodeDecodeError:
            pass
    return hashlib.sha256(data).hexdigest()


def copy_file(source: Path, target: Path, expected_sha256: str) -> dict[str, object]:
    actual = sha256(source)
    if actual != expected_sha256:
        raise SystemExit(
            f"Local source no longer matches the P0 baseline: {source}\n"
            f"expected {expected_sha256}\nactual   {actual}"
        )
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    return {
        "target": target.as_posix(),
        "sizeBytes": target.stat().st_size,
        "sourceSha256": actual,
        "contentSha256": content_sha256(target),
    }


def main() -> int:
    runtime_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference-root", type=Path, default=DEFAULT_REFERENCE_ROOT)
    parser.add_argument("--reference-build", type=Path, default=DEFAULT_REFERENCE_BUILD)
    parser.add_argument(
        "--baseline",
        type=Path,
        default=runtime_root / "migration" / "reference-baseline" / "baseline-manifest.json",
    )
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    destination = runtime_root / "engine" / "parser"
    if destination.exists() and not args.force:
        raise SystemExit(f"P2 parser sources already exist; refusing to overwrite without --force: {destination}")
    if destination.exists():
        shutil.rmtree(destination)

    baseline = json.loads(args.baseline.resolve().read_text(encoding="utf-8"))
    records_by_path = {record["path"]: record for record in baseline["files"]}
    source_paths = [
        "parser/grammar/SysML.g4",
        "parser/src/parser/SysMLParser.cpp",
        "parser/src/parser/SysMLParser.h",
        "parser/src/visitor/ModelBuilderVisitor.cpp",
        "parser/src/visitor/ModelBuilderVisitor.h",
        "parser/src/serializer/SysMLSerializer.cpp",
        "parser/src/serializer/SysMLSerializer.h",
    ]

    imported: list[dict[str, object]] = []
    reference_root = args.reference_root.resolve()
    for relative in source_paths:
        record = records_by_path.get(relative)
        if record is None:
            raise SystemExit(f"P0 baseline does not contain required P2 source: {relative}")
        target_relative = Path("engine") / Path(relative)
        copied = copy_file(
            reference_root / relative,
            runtime_root / target_relative,
            record["sha256"],
        )
        copied["source"] = relative
        copied["target"] = target_relative.as_posix()
        copied["kind"] = "maintained-source"
        imported.append(copied)

    generated_root = args.reference_build.resolve() / "parser" / "generated"
    for record in baseline["generatedParser"]:
        name = record["path"]
        target_relative = Path("engine/parser/generated") / name
        copied = copy_file(
            generated_root / name,
            runtime_root / target_relative,
            record["sha256"],
        )
        copied["source"] = f"parser/generated/{name}"
        copied["target"] = target_relative.as_posix()
        copied["kind"] = "antlr-generated"
        imported.append(copied)

    aggregate = hashlib.sha256()
    for record in sorted(imported, key=lambda item: str(item["target"])):
        aggregate.update(str(record["target"]).encode("utf-8"))
        aggregate.update(b"\0")
        aggregate.update(str(record["contentSha256"]).encode("ascii"))
        aggregate.update(b"\n")

    manifest = {
        "schemaVersion": "aidc.sysml-p2-import/0.1",
        "engineVersion": "0.2.0",
        "importedAt": datetime.now(timezone.utc).isoformat(),
        "baselineCommit": baseline["source"]["commit"],
        "baselineCapturedAt": baseline["capturedAt"],
        "antlrToolVersion": "4.13.2",
        "sourceDigest": aggregate.hexdigest(),
        "excluded": [
            {
                "path": "parser/src/annotation/AnnotationParser.cpp",
                "reason": "Depends on desktop architecture metadata and is not used by the runtime parser path.",
            }
        ],
        "files": imported,
    }
    output = runtime_root / "engine" / "P2-IMPORT.json"
    output.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "imported": len(imported),
                "sourceDigest": manifest["sourceDigest"],
                "manifest": str(output),
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
