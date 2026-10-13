#!/usr/bin/env python3
"""Freeze the locally validated reference parser state without modifying it."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path


DEFAULT_REFERENCE_ROOT = Path(r"D:\Document\7.AWorks\BlockSim\2.Development\SysML")
DEFAULT_REFERENCE_BUILD = DEFAULT_REFERENCE_ROOT / "build" / "Desktop_Qt_6_8_1_MinGW_64_bit-Debug"
SCOPES = (
    "CMakeLists.txt",
    "core",
    "parser",
    "third_party/antlr4",
    "ui/mid_windows/CMakeLists.txt",
    "ui/mid_windows/architecture",
)
STATIC_LIBRARIES = (
    "libsysml_parser.a",
    "libsysml_architecture_metadata.a",
    "libsysml_core.a",
    "libsysml_infrastructure.a",
    "libsysml_model.a",
    "libsysml_common.a",
    "libantlr4-runtime-static.a",
)


def run_git(root: Path, *arguments: str, binary: bool = False) -> bytes | str:
    result = subprocess.run(
        ["git", "-c", "core.quotepath=false", "-C", str(root), *arguments],
        check=True,
        capture_output=True,
    )
    return result.stdout if binary else result.stdout.decode("utf-8", errors="strict").strip()


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


def scoped_files(root: Path) -> list[Path]:
    files: set[Path] = set()
    for scope in SCOPES:
        candidate = root / scope
        if candidate.is_file():
            files.add(candidate)
        elif candidate.is_dir():
            files.update(path for path in candidate.rglob("*") if path.is_file())
    return sorted(files, key=lambda path: path.relative_to(root).as_posix())


def status_map(root: Path) -> dict[str, str]:
    raw = run_git(root, "status", "--porcelain=v1", "-z", "--untracked-files=all", "--", *SCOPES, binary=True)
    entries: dict[str, str] = {}
    for record in raw.decode("utf-8", errors="strict").split("\0"):
        if not record:
            continue
        state, name = record[:2], record[3:]
        if " -> " in name:
            name = name.split(" -> ", 1)[1]
        entries[Path(name).as_posix()] = state
    return entries


def copy_file(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference-root", type=Path, default=DEFAULT_REFERENCE_ROOT)
    parser.add_argument("--reference-build", type=Path, default=DEFAULT_REFERENCE_BUILD)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "migration" / "reference-baseline",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="replace an existing frozen baseline (including its validation evidence)",
    )
    args = parser.parse_args()

    reference_root = args.reference_root.resolve()
    reference_build = args.reference_build.resolve()
    output = args.output.resolve()
    if not (reference_root / ".git").exists():
        raise SystemExit(f"Reference Git checkout was not found: {reference_root}")
    if output.exists():
        if not args.force:
            raise SystemExit(f"Baseline already exists; refusing to overwrite without --force: {output}")
        shutil.rmtree(output)
    output.mkdir(parents=True)

    commit = str(run_git(reference_root, "rev-parse", "HEAD"))
    remote = str(run_git(reference_root, "remote", "get-url", "origin"))
    states = status_map(reference_root)
    inventory = []
    untracked = []
    for path in scoped_files(reference_root):
        relative = path.relative_to(reference_root).as_posix()
        state = states.get(relative, "  ")
        record = {
            "path": relative,
            "sizeBytes": path.stat().st_size,
            "sha256": sha256(path),
            "contentSha256": content_sha256(path),
            "gitState": state,
        }
        inventory.append(record)
        if state == "??":
            copy_file(path, output / "untracked" / relative)
            untracked.append(record)

    patch = run_git(reference_root, "diff", "--binary", "HEAD", "--", *SCOPES, binary=True)
    (output / "tracked-changes.patch").write_bytes(patch)

    generated_root = reference_build / "parser" / "generated"
    generated = []
    for path in sorted(item for item in generated_root.glob("SysML*") if item.is_file()):
        copy_file(path, output / "generated" / path.name)
        generated.append({"path": path.name, "sizeBytes": path.stat().st_size, "sha256": sha256(path)})

    libraries = []
    for name in STATIC_LIBRARIES:
        path = reference_build / "lib" / name
        if not path.is_file():
            raise SystemExit(f"Validated static library was not found: {path}")
        libraries.append({"path": f"lib/{name}", "sizeBytes": path.stat().st_size, "sha256": sha256(path)})

    manifest = {
        "schemaVersion": "aidc.sysml-reference-baseline/0.1",
        "capturedAt": datetime.now(timezone.utc).isoformat(),
        "source": {
            "kind": "local-git-working-tree",
            "remote": remote,
            "commit": commit,
            "localPathAtCapture": str(reference_root),
            "licenseStatus": "pending",
        },
        "scope": list(SCOPES),
        "summary": {
            "files": len(inventory),
            "bytes": sum(item["sizeBytes"] for item in inventory),
            "modifiedFiles": sum(item["gitState"] not in ("  ", "??") for item in inventory),
            "untrackedFiles": len(untracked),
            "patchBytes": len(patch),
            "generatedFiles": len(generated),
            "staticLibraries": len(libraries),
        },
        "files": inventory,
        "generatedParser": generated,
        "validatedStaticLibraries": libraries,
    }
    (output / "baseline-manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (output / "README.md").write_text(
        "# Reference Parser Baseline\n\n"
        "This directory freezes the locally validated source state used by AIDC before embedding the parser.\n\n"
        f"- Upstream commit: `{commit}`\n"
        f"- Source inventory: `{len(inventory)}` files\n"
        f"- Modified tracked files in migration scope: `{manifest['summary']['modifiedFiles']}`\n"
        f"- Preserved untracked files: `{len(untracked)}`\n"
        f"- Generated parser files: `{len(generated)}`\n\n"
        "`tracked-changes.patch` applies the local tracked changes to the upstream commit. "
        "`untracked/` preserves source files that Git did not track. `generated/` preserves the exact "
        "ANTLR output used by the validated build. `baseline-manifest.json` records hashes for every "
        "source file and validated static library. The reference checkout was read only during capture.\n",
        encoding="utf-8",
    )
    print(json.dumps(manifest["summary"], ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
