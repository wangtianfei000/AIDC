#!/usr/bin/env python3
"""Verify that every imported P1 source still matches its frozen local input."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


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


def main() -> int:
    runtime_root = Path(__file__).resolve().parents[1]
    manifest_path = runtime_root / "engine" / "P1-IMPORT.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    for record in manifest["files"]:
        target = runtime_root / record["target"]
        if not target.is_file():
            raise SystemExit(f"Imported P1 file is missing: {target}")
        actual = content_sha256(target)
        if actual != record["contentSha256"]:
            raise SystemExit(
                f"Imported P1 file changed: {target}\n"
                f"expected {record['contentSha256']}\nactual   {actual}"
            )
    license_record = manifest["antlrLicense"]
    license_path = runtime_root / license_record["target"]
    if content_sha256(license_path) != license_record["contentSha256"]:
        raise SystemExit(f"ANTLR license changed: {license_path}")
    print(
        json.dumps(
            {
                "verified": True,
                "baselineCommit": manifest["baselineCommit"],
                "files": len(manifest["files"]),
                "license": "pass",
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
