#!/usr/bin/env python3
"""Deterministic Robot IR migrations used at product ingestion boundaries."""
from __future__ import annotations

import copy
import json
from pathlib import Path
from typing import Any

CURRENT = {
    "physics": "aidc.robot.physics/0.1",
    "devices": "aidc.robot.devices/0.1",
    "interfaces": "aidc.robot.interfaces/0.1",
}
LEGACY = {name: version.replace("/0.1", "/0.0") for name, version in CURRENT.items()}


class MigrationError(RuntimeError):
    pass


def _read(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception as exc:
        raise MigrationError(f"无法读取 {path}: {exc}") from exc


def migrate_document(kind: str, document: dict[str, Any]) -> tuple[dict[str, Any], list[dict[str, str]]]:
    if kind not in CURRENT:
        raise MigrationError(f"未知 Robot IR 文档类型: {kind}")
    result = copy.deepcopy(document)
    source = result.get("schemaVersion")
    if source == CURRENT[kind]:
        return result, []
    if source != LEGACY[kind]:
        raise MigrationError(f"不支持的 {kind} schema: {source!r}")
    changes: list[dict[str, str]] = [{"path": "/schemaVersion", "from": source, "to": CURRENT[kind]}]
    result["schemaVersion"] = CURRENT[kind]
    if kind == "physics":
        defaults = {
            "rootType": "fixed",
            "namedPoses": [],
            "contactGeometries": [],
        }
        for key, value in defaults.items():
            if key not in result:
                result[key] = value
                changes.append({"path": f"/{key}", "from": "<missing>", "to": json.dumps(value)})
    return result, changes


def migrate_bundle(source: Path, destination: Path) -> dict[str, Any]:
    source, destination = source.resolve(), destination.resolve()
    destination.mkdir(parents=True, exist_ok=True)
    report: dict[str, Any] = {"schemaVersion": "aidc.robot.migration-report/0.1", "documents": []}
    for kind in ("physics", "devices", "interfaces"):
        migrated, changes = migrate_document(kind, _read(source / f"{kind}.json"))
        (destination / f"{kind}.json").write_text(json.dumps(migrated, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        report["documents"].append({"document": f"{kind}.json", "changes": changes})
    return report
