#!/usr/bin/env python3
"""Enforce the P5 long-term maintenance contract for the embedded SysML engine."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "engine"
sys.path.insert(0, str(Path(__file__).resolve().parent))
from update_engine_maintenance import GENERATED_FILES, digest_file, digest_records, engine_files, snapshot  # noqa: E402


def fail(message: str) -> None:
    raise SystemExit(f"P5 engine maintenance gate failed: {message}")


def verify_manifest() -> dict[str, object]:
    path = ENGINE / "MAINTENANCE.json"
    if not path.is_file():
        fail("engine/MAINTENANCE.json is missing")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    actual = snapshot(str(manifest.get("engineVersion", "")))
    for field in ("schemaVersion", "engineVersion", "antlrToolVersion", "grammar", "generated", "sourceDigest", "sourceFileCount"):
        if manifest.get(field) != actual.get(field):
            fail(f"{field} is stale; run tools/update_engine_maintenance.py with a new engine version")
    history = manifest.get("history")
    if not isinstance(history, list) or not history:
        fail("maintenance history is empty")
    latest = history[-1]
    if latest.get("engineVersion") != manifest["engineVersion"] or latest.get("sourceDigest") != manifest["sourceDigest"]:
        fail("latest maintenance history entry does not describe the current engine")
    return manifest


def verify_model_policy() -> None:
    path = ENGINE / "MODEL-MAINTENANCE.json"
    policy = json.loads(path.read_text(encoding="utf-8"))
    current = {
        item.stem
        for item in (ENGINE / "core" / "model").glob("*.h")
        if item.name != "CMakeLists.txt"
    }
    baseline = set(policy.get("baselineModels", []))
    coverage = policy.get("newModelCoverage", {})
    declared = baseline | set(coverage)
    if current != declared:
        missing = sorted(current - declared)
        removed = sorted(declared - current)
        fail(f"model inventory changed; undeclared={missing}, missing={removed}")
    for model, tests in coverage.items():
        for role in ("parserConstruction", "snapshotProjection"):
            evidence = tests.get(role, {})
            test_path = ROOT / str(evidence.get("file", ""))
            marker = str(evidence.get("marker", ""))
            if not test_path.is_file() or not marker or marker not in test_path.read_text(encoding="utf-8"):
                fail(f"{model} lacks verifiable {role} evidence")


def verify_boundaries() -> None:
    forbidden = {
        "RuntimeServer": re.compile(r"\bRuntimeServer\b"),
        "browser source": re.compile(r"sysml-browser|model3d-viewer|modelica-browser"),
        "AIDC CLI/core": re.compile(r"aidc_core|(?:^|[/\\])cli(?:[/\\])"),
        "desktop UI": re.compile(r"(?:^|[/\\])ui(?:[/\\])|Qt6::(?:Widgets|Gui|Network)"),
    }
    suffixes = {".cpp", ".h", ".hpp", ".cmake"}
    for path in ENGINE.rglob("*"):
        if not path.is_file() or (path.name != "CMakeLists.txt" and path.suffix.lower() not in suffixes):
            continue
        relative = path.relative_to(ROOT).as_posix()
        text = path.read_text(encoding="utf-8", errors="strict")
        for label, pattern in forbidden.items():
            if pattern.search(text):
                fail(f"{relative} crosses the {label} boundary")


def main() -> int:
    manifest = verify_manifest()
    verify_model_policy()
    verify_boundaries()
    print(
        json.dumps(
            {
                "verified": True,
                "engineVersion": manifest["engineVersion"],
                "sourceDigest": manifest["sourceDigest"],
                "generatedFiles": len(GENERATED_FILES),
                "modelPolicy": "pass",
                "dependencyBoundaries": "pass",
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
