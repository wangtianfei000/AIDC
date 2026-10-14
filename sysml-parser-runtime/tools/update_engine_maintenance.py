#!/usr/bin/env python3
"""Record an intentional SysML engine revision after source and tests are updated."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from datetime import date
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "engine"
MANIFEST = ENGINE / "MAINTENANCE.json"
GENERATED = ENGINE / "parser" / "generated"
GRAMMAR = ENGINE / "parser" / "grammar" / "SysML.g4"
ANTLR_VERSION = (ROOT / "third_party" / "antlr4-runtime-4.13.2" / "VERSION").read_text(encoding="utf-8").strip()
GENERATED_FILES = (
    "SysML.interp",
    "SysML.tokens",
    "SysMLBaseVisitor.cpp",
    "SysMLBaseVisitor.h",
    "SysMLLexer.cpp",
    "SysMLLexer.h",
    "SysMLLexer.interp",
    "SysMLLexer.tokens",
    "SysMLParser.cpp",
    "SysMLParser.h",
    "SysMLVisitor.cpp",
    "SysMLVisitor.h",
)
SOURCE_SUFFIXES = {".cpp", ".h", ".hpp", ".g4", ".interp", ".tokens"}
SEMVER = re.compile(r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$")


def normalized_bytes(path: Path) -> bytes:
    return path.read_bytes().replace(b"\r\n", b"\n")


def digest_file(path: Path) -> str:
    return hashlib.sha256(normalized_bytes(path)).hexdigest()


def digest_records(records: list[tuple[str, str]]) -> str:
    digest = hashlib.sha256()
    for relative, file_digest in sorted(records):
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_digest.encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def engine_files() -> list[Path]:
    result: list[Path] = []
    for path in ENGINE.rglob("*"):
        if not path.is_file() or path.name in {"MAINTENANCE.json", "P1-IMPORT.json", "P2-IMPORT.json", "ORIGIN.md"}:
            continue
        if path.name == "CMakeLists.txt" or path.suffix.lower() in SOURCE_SUFFIXES:
            result.append(path)
    return sorted(result)


def semver_tuple(value: str) -> tuple[int, int, int]:
    match = SEMVER.fullmatch(value)
    if not match:
        raise SystemExit(f"Engine version must be SemVer x.y.z: {value}")
    return tuple(int(part) for part in match.groups())


def snapshot(version: str) -> dict[str, object]:
    generated_records = [(name, digest_file(GENERATED / name)) for name in GENERATED_FILES]
    source_records = [
        (path.relative_to(ENGINE).as_posix(), digest_file(path))
        for path in engine_files()
    ]
    return {
        "schemaVersion": "aidc.sysml-engine-maintenance/1.0",
        "engineVersion": version,
        "antlrToolVersion": ANTLR_VERSION,
        "grammar": {
            "path": GRAMMAR.relative_to(ROOT).as_posix(),
            "sha256": digest_file(GRAMMAR),
        },
        "generated": {
            "files": list(GENERATED_FILES),
            "sha256": digest_records(generated_records),
        },
        "sourceDigest": digest_records(source_records),
        "sourceFileCount": len(source_records),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine-version", required=True)
    parser.add_argument("--note", default="Intentional engine maintenance update")
    args = parser.parse_args()
    requested = semver_tuple(args.engine_version)
    current = snapshot(args.engine_version)
    history: list[dict[str, object]] = []

    if MANIFEST.is_file():
        previous = json.loads(MANIFEST.read_text(encoding="utf-8"))
        history = list(previous.get("history", []))
        old_version = str(previous["engineVersion"])
        old_digest = str(previous["sourceDigest"])
        if current["sourceDigest"] != old_digest and requested <= semver_tuple(old_version):
            raise SystemExit(
                f"Engine source changed; choose a version greater than {old_version}, got {args.engine_version}"
            )
        if current["sourceDigest"] == old_digest and args.engine_version != old_version:
            raise SystemExit("Engine version cannot change when the maintained source digest is unchanged")

    release = {
        "engineVersion": args.engine_version,
        "recordedAt": date.today().isoformat(),
        "sourceDigest": current["sourceDigest"],
        "grammarSha256": current["grammar"]["sha256"],
        "generatedSha256": current["generated"]["sha256"],
        "note": args.note,
    }
    if not history or history[-1].get("sourceDigest") != release["sourceDigest"]:
        history.append(release)
    current["history"] = history
    MANIFEST.write_text(json.dumps(current, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(release, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
