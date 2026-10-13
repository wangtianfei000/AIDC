#!/usr/bin/env python3
"""Compare frozen external-parser behavior with an embedded-parser capture."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


IMPLEMENTATION_FIELDS = {
    ("health.json", "parserMode"),
    ("health.json", "parserBaseline"),
    ("capabilities.json", "parserMode"),
    ("capabilities.json", "parserBaseline"),
}


def normalize(file_name: str, value: Any, path: tuple[str, ...] = ()) -> Any:
    if isinstance(value, dict):
        result = {}
        for key, item in value.items():
            if (file_name, key) in IMPLEMENTATION_FIELDS and not path:
                continue
            if path and path[-1] == "parser" and key in {"name", "baseline"}:
                continue
            if key == "rootPath" and path and path[-1] == "standardLibrary":
                continue
            result[key] = normalize(file_name, item, (*path, key))
        if path and path[-1] == "properties" and "sourceState" in result and "targetState" in result:
            # The frozen binary exposed an uninitialized backing bool. Normalize
            # it from the parsed transition ends, which define the actual syntax.
            result["isInitial"] = not result["sourceState"] and bool(result["targetState"])
        return result
    if isinstance(value, list):
        return [normalize(file_name, item, (*path, str(index))) for index, item in enumerate(value)]
    return value


def first_differences(left: Any, right: Any, path: str = "$", limit: int = 50) -> list[dict[str, Any]]:
    differences: list[dict[str, Any]] = []

    def walk(a: Any, b: Any, current: str) -> None:
        if len(differences) >= limit:
            return
        if type(a) is not type(b):
            differences.append({"path": current, "external": a, "embedded": b})
        elif isinstance(a, dict):
            for key in sorted(set(a) | set(b)):
                child = f"{current}.{key}"
                if key not in a:
                    differences.append({"path": child, "external": "<missing>", "embedded": b[key]})
                elif key not in b:
                    differences.append({"path": child, "external": a[key], "embedded": "<missing>"})
                else:
                    walk(a[key], b[key], child)
                if len(differences) >= limit:
                    return
        elif isinstance(a, list):
            if len(a) != len(b):
                differences.append({"path": f"{current}.length", "external": len(a), "embedded": len(b)})
            for index, (left_item, right_item) in enumerate(zip(a, b)):
                walk(left_item, right_item, f"{current}[{index}]")
                if len(differences) >= limit:
                    return
        elif a != b:
            differences.append({"path": current, "external": a, "embedded": b})

    walk(left, right, path)
    return differences


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--external", required=True, type=Path)
    parser.add_argument("--embedded", required=True, type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    files = sorted(path.name for path in args.external.glob("*.json"))
    report: dict[str, Any] = {
        "schemaVersion": "aidc.sysml-parser-equivalence/1.0",
        "status": "passed",
        "normalization": [
            "HTTP health/capabilities parserMode and parserBaseline",
            "ModelSnapshot/AnalysisSnapshot document.parser name and baseline",
            "standardLibrary.rootPath deployment location",
            "Transition.properties.isInitial derived from source/target states",
        ],
        "cases": [],
    }
    for file_name in files:
        external = normalize(file_name, json.loads((args.external / file_name).read_text(encoding="utf-8")))
        embedded_path = args.embedded / file_name
        if not embedded_path.is_file():
            differences = [{"path": "$", "external": "present", "embedded": "<missing>"}]
        else:
            embedded = normalize(file_name, json.loads(embedded_path.read_text(encoding="utf-8")))
            differences = first_differences(external, embedded)
        case = {"name": file_name, "status": "passed" if not differences else "failed", "differences": differences}
        report["cases"].append(case)
        if differences:
            report["status"] = "failed"

    rendered = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    print(rendered, end="")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
