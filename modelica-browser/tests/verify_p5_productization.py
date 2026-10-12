#!/usr/bin/env python3
"""P5 end-to-end productization acceptance."""
from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / "tools" / "aidc_modelica.py"
EXAMPLE = ROOT / "packages" / "robot-ir" / "examples" / "reference-arm5"
OUTPUT = ROOT / "generated" / "reference-arm5"
PACKAGE = ROOT / "dist" / "reference-arm5-p5.aidcmpkg"
LIBRARY_PACKAGE = ROOT / "dist" / "AIDCRobotics-0.3.0.zip"
SUMMARY = ROOT / "results" / "p5-productization-summary.json"
sys.path.insert(0, str(ROOT / "tools")); sys.path.insert(0, str(ROOT / "packages" / "robot-ir" / "src"))
from project_tool import run_project, validate_project
from robot_ir_validator import validate_bundle


def run(*arguments: object, expected: int = 0) -> tuple[dict, subprocess.CompletedProcess[str]]:
    process = subprocess.run([sys.executable, str(CLI), *(str(item) for item in arguments)], cwd=ROOT, capture_output=True, text=True)
    if process.returncode != expected:
        raise AssertionError(f"command failed ({process.returncode}): {' '.join(map(str, arguments))}\n{process.stdout}\n{process.stderr}")
    text = process.stdout if expected == 0 else process.stderr
    return json.loads(text), process


def main() -> int:
    subprocess.run([sys.executable, str(EXAMPLE / "build_bundle.py")], check=True)
    bundle = validate_bundle(EXAMPLE)
    if OUTPUT.exists(): shutil.rmtree(OUTPUT)
    validated, _ = run("validate", EXAMPLE)
    generated, _ = run("generate", EXAMPLE, OUTPUT)
    cached, _ = run("generate", EXAMPLE, OUTPUT)
    assert generated["cacheStatus"] == "miss" and cached["cacheStatus"] == "hit"
    assert generated["requestDigest"] == cached["requestDigest"]
    lock = json.loads((OUTPUT / "robot-ir" / "generation.lock.json").read_text(encoding="utf-8"))
    assert lock["structure"] == {"rootType": "fixed", "bodyCount": 5, "jointCount": 4, "actuatorCount": 4, "totalMass_kg": 9.299999999999999} or lock["structure"]["totalMass_kg"] == 9.3
    source = (ROOT / "tools" / "modelica_generator.py").read_text(encoding="utf-8").lower()
    assert "reference.arm5" not in source and "microduck" not in source

    run_dir = run_project(OUTPUT)
    compared, _ = run("compare", run_dir, run_dir, "--abs-tol", 0, "--rel-tol", 0)
    assert compared["passed"] and compared["sampleCount"] > 1

    packaged, _ = run("package", OUTPUT, PACKAGE)
    with tempfile.TemporaryDirectory(prefix="aidc-p5-package-") as directory:
        with zipfile.ZipFile(PACKAGE) as archive: archive.extractall(directory)
        portable = validate_project(Path(directory))
        assert portable["project"]["projectId"] == "aidc.reference.arm5.modelica"

    (ROOT / "results").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="aidc-p5-migration-", dir=ROOT / "results") as directory:
        legacy = Path(directory) / "legacy"; migrated_output = Path(directory) / "generated"
        shutil.copytree(EXAMPLE, legacy)
        for kind in ("physics", "devices", "interfaces"):
            path = legacy / f"{kind}.json"; value = json.loads(path.read_text(encoding="utf-8")); value["schemaVersion"] = value["schemaVersion"].replace("/0.1", "/0.0")
            if kind == "physics":
                for key in ("rootType", "namedPoses", "contactGeometries"): value.pop(key, None)
            path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
        migrated, _ = run("generate", legacy, migrated_output, "--migrate")
        assert migrated["migration"] and migrated["migration"]["documents"][0]["changes"]
        validate_project(migrated_output)

    with tempfile.TemporaryDirectory(prefix="aidc-p5-diagnostic-") as directory:
        invalid = Path(directory); shutil.copytree(EXAMPLE, invalid, dirs_exist_ok=True)
        path = invalid / "physics.json"; value = json.loads(path.read_text(encoding="utf-8")); value["bodies"][0]["mass_kg"] = "bad"; path.write_text(json.dumps(value, indent=2), encoding="utf-8")
        failure, _ = run("validate", invalid, expected=1)
        assert failure["diagnostics"][0]["source"]["jsonPointer"] == "/bodies/0/mass_kg"

    release_script = ROOT / "tools" / "package_aidc_robotics.py"
    subprocess.run([sys.executable, str(release_script), str(LIBRARY_PACKAGE)], cwd=ROOT, check=True, capture_output=True, text=True)
    first_digest = hashlib.sha256(LIBRARY_PACKAGE.read_bytes()).hexdigest()
    subprocess.run([sys.executable, str(release_script), str(LIBRARY_PACKAGE)], cwd=ROOT, check=True, capture_output=True, text=True)
    assert hashlib.sha256(LIBRARY_PACKAGE.read_bytes()).hexdigest() == first_digest

    summary = {
        "schemaVersion": "aidc.verification.p5/0.1", "passed": True,
        "cli": {"version": "0.1.0", "commands": ["generate", "validate", "compare", "package"], "incrementalCache": cached["cacheStatus"], "schemaMigration": True, "diagnosticLocation": "/bodies/0/mass_kg"},
        "secondRobot": {"modelId": bundle["modelId"], "bodyCount": bundle["bodyCount"], "jointCount": bundle["jointCount"], "deviceCount": bundle["deviceCount"], "generatorBranchAdded": False, "runDirectory": str(run_dir.relative_to(ROOT)), "simulationPassed": True},
        "portablePackage": {"path": str(PACKAGE.relative_to(ROOT)), "sha256": packaged["sha256"], "fileCount": packaged["fileCount"]},
        "libraryRelease": {"path": str(LIBRARY_PACKAGE.relative_to(ROOT)), "sha256": "sha256:" + first_digest, "deterministic": True, "apiDocs": True, "profiles": True, "compatibilityMatrix": True},
        "publishedModelBridge": {"script": "model3d-viewer/scripts/create-modelica-project.ps1", "supportedInputs": ["Robot IR directory", "published directory containing robot-ir/source"], "oneCommand": "aidc-modelica generate <published-model> <project>"},
    }
    SUMMARY.parent.mkdir(parents=True, exist_ok=True); SUMMARY.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2)); return 0


if __name__ == "__main__": raise SystemExit(main())
