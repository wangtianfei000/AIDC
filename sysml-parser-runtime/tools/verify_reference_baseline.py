#!/usr/bin/env python3
"""Verify the frozen baseline against its local source and a reconstructed tree."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tarfile
import tempfile
from pathlib import Path


DEFAULT_REFERENCE_ROOT = Path(r"D:\Document\7.AWorks\BlockSim\2.Development\SysML")
DEFAULT_REFERENCE_BUILD = DEFAULT_REFERENCE_ROOT / "build" / "Desktop_Qt_6_8_1_MinGW_64_bit-Debug"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def assert_hash(path: Path, expected: str) -> None:
    if not path.is_file():
        raise SystemExit(f"Baseline file is missing: {path}")
    actual = sha256(path)
    if actual != expected:
        raise SystemExit(f"Baseline hash mismatch: {path}\nexpected {expected}\nactual   {actual}")


def content_sha256(path: Path) -> str:
    data = path.read_bytes()
    if b"\0" not in data:
        try:
            data.decode("utf-8")
            data = data.replace(b"\r\n", b"\n")
        except UnicodeDecodeError:
            pass
    return hashlib.sha256(data).hexdigest()


def assert_content_hash(path: Path, expected: str) -> None:
    if not path.is_file():
        raise SystemExit(f"Reconstructed baseline file is missing: {path}")
    actual = content_sha256(path)
    if actual != expected:
        raise SystemExit(
            f"Reconstructed content mismatch: {path}\nexpected {expected}\nactual   {actual}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference-root", type=Path, default=DEFAULT_REFERENCE_ROOT)
    parser.add_argument("--reference-build", type=Path, default=DEFAULT_REFERENCE_BUILD)
    parser.add_argument(
        "--baseline",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "migration" / "reference-baseline",
    )
    args = parser.parse_args()
    reference_root = args.reference_root.resolve()
    reference_build = args.reference_build.resolve()
    baseline = args.baseline.resolve()
    manifest = json.loads((baseline / "baseline-manifest.json").read_text(encoding="utf-8"))

    for record in manifest["files"]:
        assert_hash(reference_root / record["path"], record["sha256"])
    for record in manifest["generatedParser"]:
        assert_hash(baseline / "generated" / record["path"], record["sha256"])
    for record in manifest["validatedStaticLibraries"]:
        assert_hash(reference_build / record["path"], record["sha256"])

    with tempfile.TemporaryDirectory(prefix="aidc-sysml-baseline-") as temporary:
        temporary_root = Path(temporary)
        archive = temporary_root / "source.tar"
        with archive.open("wb") as stream:
            result = subprocess.run(
                ["git", "-C", str(reference_root), "archive", "--format=tar", manifest["source"]["commit"]],
                stdout=stream,
            )
        if result.returncode:
            raise SystemExit("Unable to archive the recorded reference commit")
        reconstructed = temporary_root / "reconstructed"
        reconstructed.mkdir()
        with tarfile.open(archive) as source_archive:
            source_archive.extractall(reconstructed, filter="data")
        patch = baseline / "tracked-changes.patch"
        if patch.stat().st_size:
            subprocess.run(["git", "apply", "--whitespace=nowarn", str(patch)], cwd=reconstructed, check=True)
        untracked = baseline / "untracked"
        if untracked.exists():
            shutil.copytree(untracked, reconstructed, dirs_exist_ok=True)
        for record in manifest["files"]:
            assert_content_hash(reconstructed / record["path"], record["contentSha256"])

    print(
        json.dumps(
            {
                "verified": True,
                "sourceFiles": len(manifest["files"]),
                "generatedFiles": len(manifest["generatedParser"]),
                "staticLibraries": len(manifest["validatedStaticLibraries"]),
                "reconstruction": "pass",
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
