#!/usr/bin/env python3
"""Verify the integrated Jingang C3 architecture, analysis and Table View sample."""

from __future__ import annotations

import argparse
import json
import math
import pathlib

import jsonschema


def close(actual: float, expected: float) -> bool:
    return math.isclose(actual, expected, rel_tol=0, abs_tol=1e-9)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", required=True, type=pathlib.Path)
    parser.add_argument("--model-snapshot", required=True, type=pathlib.Path)
    parser.add_argument("--analysis-snapshot", required=True, type=pathlib.Path)
    parser.add_argument("--model-schema", required=True, type=pathlib.Path)
    parser.add_argument("--analysis-schema", required=True, type=pathlib.Path)
    parser.add_argument("--expected", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    source = args.model.read_text(encoding="utf-8")
    model = json.loads(args.model_snapshot.read_text(encoding="utf-8"))
    analysis_snapshot = json.loads(args.analysis_snapshot.read_text(encoding="utf-8"))
    expected = json.loads(args.expected.read_text(encoding="utf-8"))
    jsonschema.Draft202012Validator(json.loads(args.model_schema.read_text(encoding="utf-8"))).validate(model)
    jsonschema.Draft202012Validator(json.loads(args.analysis_schema.read_text(encoding="utf-8"))).validate(analysis_snapshot)

    forbidden_manual_results = [
        "currentPlannedMass", "currentEstimatedEndurance", "batteryEnergy",
        "maximumAveragePowerForEndurance", "peakElectricalPowerLimit",
    ]
    assert not any(name in source for name in forbidden_manual_results)
    assert sum(item.get("severity") == "error" for item in model["diagnostics"]) == 0
    calculation = model["calculationModel"]
    assert len(calculation["definitions"]) == 4
    assert len(calculation["analyses"]) == 1
    assert len(calculation["usages"]) == 4

    view_model = model["viewModel"]
    view = next(item for item in view_model["views"] if item["name"] == expected["view"]["name"])
    rendering = next(item for item in view_model["renderings"] if item["id"] == view["renderingId"])
    assert rendering["renderingKind"] == expected["view"]["renderingKind"]
    assert [item["name"] for item in rendering["columns"]] == expected["view"]["columns"]
    assert len(view["exposedElementIds"]) == 4
    assert [item.rsplit("::", 1)[-1] for item in view["exposedReferences"]] == expected["view"]["exposedRows"]

    assert len(analysis_snapshot["analyses"]) == 1
    assert not analysis_snapshot["diagnostics"]
    analysis = analysis_snapshot["analyses"][0]
    results = {item["name"]: item for item in analysis["results"]}
    constraints = {item.get("resultParameterId"): item for item in analysis["constraints"]}
    checks = []
    for oracle in expected["rows"]:
        result = results[oracle["id"]]
        constraint = constraints[result["parameterId"]]
        passed = (
            result["status"] == "computed"
            and result["unit"] == oracle["unit"]
            and close(result["value"], oracle["value"])
            and close(constraint["target"], oracle["target"])
            and close(constraint["margin"], oracle["margin"])
            and constraint["status"] == oracle["status"]
            and bool(result["inputParameterIds"])
        )
        if oracle["requirement"]:
            passed = passed and constraint["name"].startswith(oracle["requirement"].replace("-", "_"))
        checks.append({
            "id": oracle["id"], "value": result["value"], "unit": result["unit"],
            "target": constraint["target"], "margin": constraint["margin"],
            "status": constraint["status"], "inputCount": len(result["inputParameterIds"]),
            "passed": passed,
        })
    assert all(item["passed"] for item in checks)

    summary = {
        "schemaValidation": "pass",
        "parseErrors": 0,
        "parseWarnings": sum(item.get("severity") == "warning" for item in model["diagnostics"]),
        "calculationDefinitions": len(calculation["definitions"]),
        "calculationUsages": len(calculation["usages"]),
        "analysisCases": len(calculation["analyses"]),
        "calculationDiagnostics": len(analysis_snapshot["diagnostics"]),
        "manualResultFieldsRemoved": forbidden_manual_results,
        "tableView": {"rows": len(view["exposedElementIds"]), "columns": len(rendering["columns"]), "status": "pass"},
        "checks": checks,
    }
    args.output.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
