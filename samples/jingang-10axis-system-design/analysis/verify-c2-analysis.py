#!/usr/bin/env python3
"""Validate the C2 AnalysisSnapshot against the frozen C0 hand calculation oracle."""

from __future__ import annotations

import argparse
import json
import math
import pathlib

import jsonschema


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--actual", required=True, type=pathlib.Path)
    parser.add_argument("--expected", required=True, type=pathlib.Path)
    parser.add_argument("--schema", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    actual = json.loads(args.actual.read_text(encoding="utf-8"))
    expected = json.loads(args.expected.read_text(encoding="utf-8"))
    schema = json.loads(args.schema.read_text(encoding="utf-8"))
    jsonschema.Draft202012Validator(schema).validate(actual)

    analysis = actual["analyses"][0]
    results = {item["name"]: item for item in analysis["results"]}
    constraints = {
        item.get("resultParameterId"): item for item in analysis["constraints"]
    }
    checks = []
    for oracle in expected["rows"]:
        result = results[oracle["id"]]
        constraint = constraints[result["parameterId"]]
        passed = (
            result["status"] == "computed"
            and result["unit"] == oracle["unit"]
            and math.isclose(result["value"], oracle["value"], rel_tol=0, abs_tol=1e-9)
            and constraint["status"] == oracle["status"]
            and constraint["comparison"] == oracle["comparison"]
            and constraint["unit"] == oracle["marginUnit"]
            and math.isclose(constraint["target"], oracle["target"], rel_tol=0, abs_tol=1e-9)
            and math.isclose(constraint["margin"], oracle["margin"], rel_tol=0, abs_tol=1e-9)
        )
        checks.append({
            "id": oracle["id"],
            "value": result["value"],
            "unit": result["unit"],
            "target": constraint["target"],
            "margin": constraint["margin"],
            "status": constraint["status"],
            "passed": passed,
        })
    assert all(item["passed"] for item in checks)
    summary = {
        "schemaValidation": "pass",
        "oracle": expected["fixture"],
        "engine": actual["engine"],
        "checks": checks,
        "calculationDiagnostics": len(actual["diagnostics"]),
        "parseErrors": sum(item.get("severity") == "error" for item in actual.get("parseDiagnostics", [])),
    }
    args.output.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
