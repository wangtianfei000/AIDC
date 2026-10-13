#!/usr/bin/env python3
"""Import the P1 model layer and ANTLR runtime from the frozen local baseline."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path


DEFAULT_REFERENCE_ROOT = Path(r"D:\Document\7.AWorks\BlockSim\2.Development\SysML")
DEFAULT_ANTLR_LICENSE = Path(r"D:\Document\7.AWorks\BlockSim\2.Development\third_party\antlr4-dev\LICENSE.txt")


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


def copy_verified(
    source: Path, target: Path, expected_hash: str, expected_content_hash: str
) -> dict[str, object]:
    actual_hash = sha256(source)
    if actual_hash != expected_hash:
        raise SystemExit(
            f"Local source no longer matches the P0 baseline: {source}\n"
            f"expected {expected_hash}\nactual   {actual_hash}"
        )
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    return {
        "source": source.as_posix(),
        "target": target.as_posix(),
        "sizeBytes": target.stat().st_size,
        "sourceSha256": actual_hash,
        "contentSha256": expected_content_hash,
    }


def main() -> int:
    runtime_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference-root", type=Path, default=DEFAULT_REFERENCE_ROOT)
    parser.add_argument("--antlr-license", type=Path, default=DEFAULT_ANTLR_LICENSE)
    parser.add_argument(
        "--baseline",
        type=Path,
        default=runtime_root / "migration" / "reference-baseline" / "baseline-manifest.json",
    )
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    reference_root = args.reference_root.resolve()
    manifest = json.loads(args.baseline.resolve().read_text(encoding="utf-8"))
    hashes = {record["path"]: record["sha256"] for record in manifest["files"]}
    destinations = (
        runtime_root / "engine" / "core",
        runtime_root / "third_party" / "antlr4-runtime-4.13.2",
    )
    existing = [path for path in destinations if path.exists()]
    if existing and not args.force:
        raise SystemExit("P1 sources already exist; refusing to overwrite without --force: " + ", ".join(map(str, existing)))
    for path in existing:
        shutil.rmtree(path)

    records: list[dict[str, object]] = []
    selected = [
        record
        for record in manifest["files"]
        if (
            record["path"].startswith("core/")
            and Path(record["path"]).name != "CMakeLists.txt"
        )
        or record["path"].startswith("third_party/antlr4/runtime/src/")
        or record["path"] == "third_party/antlr4/VERSION"
    ]
    for record in selected:
        relative = record["path"]
        if relative.startswith("core/"):
            target_relative = Path("engine") / relative
        elif relative == "third_party/antlr4/VERSION":
            target_relative = Path("third_party/antlr4-runtime-4.13.2/VERSION")
        else:
            target_relative = Path("third_party/antlr4-runtime-4.13.2/runtime/src") / Path(relative).relative_to(
                "third_party/antlr4/runtime/src"
            )
        copied = copy_verified(
            reference_root / relative,
            runtime_root / target_relative,
            hashes[relative],
            record["contentSha256"],
        )
        copied["source"] = relative
        copied["target"] = target_relative.as_posix()
        records.append(copied)

    license_source = args.antlr_license.resolve()
    if not license_source.is_file():
        raise SystemExit(f"Local ANTLR license was not found: {license_source}")
    license_target = runtime_root / "third_party" / "antlr4-runtime-4.13.2" / "LICENSE.txt"
    license_target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(license_source, license_target)

    import_manifest = {
        "schemaVersion": "aidc.sysml-p1-import/0.1",
        "importedAt": datetime.now(timezone.utc).isoformat(),
        "baselineCommit": manifest["source"]["commit"],
        "baselineCapturedAt": manifest["capturedAt"],
        "files": records,
        "antlrLicense": {
            "sourceAtImport": str(license_source),
            "target": "third_party/antlr4-runtime-4.13.2/LICENSE.txt",
            "sha256": sha256(license_target),
            "contentSha256": content_sha256(license_target),
        },
    }
    output = runtime_root / "engine" / "P1-IMPORT.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(import_manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"imported": len(records), "manifest": str(output)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
