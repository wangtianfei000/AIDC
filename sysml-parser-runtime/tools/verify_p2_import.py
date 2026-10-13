#!/usr/bin/env python3
"""Verify the in-repository P2 parser source set and its aggregate digest."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


def content_sha256(path: Path) -> str:
    data = path.read_bytes()
    if b"\0" not in data:
        try:
            data.decode("utf-8")
            data = data.replace(b"\r\n", b"\n")
        except UnicodeDecodeError:
            pass
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    runtime_root = Path(__file__).resolve().parents[1]
    manifest_path = runtime_root / "engine" / "P2-IMPORT.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    aggregate = hashlib.sha256()
    for record in sorted(manifest["files"], key=lambda item: str(item["target"])):
        target = runtime_root / record["target"]
        if not target.is_file():
            raise SystemExit(f"Imported P2 file is missing: {target}")
        actual = content_sha256(target)
        if actual != record["contentSha256"]:
            raise SystemExit(
                f"Imported P2 file changed: {target}\n"
                f"expected {record['contentSha256']}\nactual   {actual}"
            )
        aggregate.update(str(record["target"]).encode("utf-8"))
        aggregate.update(b"\0")
        aggregate.update(actual.encode("ascii"))
        aggregate.update(b"\n")
    actual_digest = aggregate.hexdigest()
    if actual_digest != manifest["sourceDigest"]:
        raise SystemExit(
            f"P2 aggregate digest changed: expected {manifest['sourceDigest']}, actual {actual_digest}"
        )
    grammar_count = sum(1 for item in manifest["files"] if item["target"].endswith(".g4"))
    generated_count = sum(1 for item in manifest["files"] if item["kind"] == "antlr-generated")
    maintained_count = sum(1 for item in manifest["files"] if item["kind"] == "maintained-source")
    print(
        json.dumps(
            {
                "verified": True,
                "engineVersion": manifest["engineVersion"],
                "sourceDigest": actual_digest,
                "grammarFiles": grammar_count,
                "generatedFiles": generated_count,
                "maintainedFiles": maintained_count,
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
