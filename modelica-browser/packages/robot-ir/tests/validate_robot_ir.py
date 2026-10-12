#!/usr/bin/env python3
from __future__ import annotations

import copy
import json
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve()
PACKAGE = HERE.parents[1]
sys.path.insert(0, str(PACKAGE / "src"))

from robot_ir_validator import RobotIRError, validate_bundle


def require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def expect_failure(documents: dict[str, dict], message: str) -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        for name, value in documents.items():
            (root / f"{name}.json").write_text(json.dumps(value), encoding="utf-8")
        try:
            validate_bundle(root)
        except RobotIRError:
            return
        raise AssertionError(message)


def main() -> int:
    example = PACKAGE / "examples" / "single-joint"
    summary = validate_bundle(example)
    require(summary == {
        "valid": True,
        "modelId": "aidc.standard.single-joint",
        "modelRevision": "p0-r1",
        "bodyCount": 2,
        "jointCount": 1,
        "deviceCount": 3,
        "commandCount": 1,
        "stateCount": 4,
        "diagnosticCount": 3,
    }, "unexpected single-joint summary")

    base = {name: json.loads((example / f"{name}.json").read_text(encoding="utf-8")) for name in ("physics", "devices", "interfaces")}
    bad_identity = copy.deepcopy(base)
    bad_identity["devices"]["modelRevision"] = "other"
    expect_failure(bad_identity, "mismatched revisions must fail")

    bad_inertia = copy.deepcopy(base)
    bad_inertia["physics"]["bodies"][1]["inertia_kg_m2"]["ixy"] = 0.1
    expect_failure(bad_inertia, "non-positive-definite inertia must fail")

    bad_order = copy.deepcopy(base)
    bad_order["interfaces"]["states"][0]["index"] = 2
    expect_failure(bad_order, "non-contiguous channel order must fail")

    bad_reference = copy.deepcopy(base)
    bad_reference["devices"]["devices"][0]["jointId"] = "missing-joint"
    expect_failure(bad_reference, "unknown device joint must fail")

    print("robot-ir tests passed: schema, identity, inertia, references and channel order")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
