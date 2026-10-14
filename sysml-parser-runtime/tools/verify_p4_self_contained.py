#!/usr/bin/env python3
"""Fail when the production SysML runtime can still reach the retired parser tree."""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

# Keep the tokens split so this verifier does not flag its own policy strings.
FORBIDDEN = {
    "engine switch declaration": "set(AIDC_SYSML_" + "ENGINE ",
    "engine switch argument": "-DAIDC_SYSML_" + "ENGINE=",
    "reference source variable": "REFERENCE_" + "ROOT",
    "reference build variable": "REFERENCE_" + "BUILD",
    "external parser archive": "libsysml_" + "parser.a",
    "retired checkout": "7.AWorks" + "/BlockSim",
}

ACTIVE_FILES = [
    ROOT / "CMakeLists.txt",
    ROOT / "build-runtime.ps1",
    ROOT / "run-runtime.ps1",
    ROOT / "src" / "RuntimeServer.cpp",
    ROOT / "engine" / "CMakeLists.txt",
    ROOT / "engine" / "services" / "ModelSnapshotAdapter.h",
    ROOT / "engine" / "services" / "ModelSnapshotAdapter.cpp",
    ROOT / "engine" / "services" / "SysMLKernelService.cpp",
    ROOT / "engine" / "core" / "analysis" / "AnalysisService.cpp",
    ROOT / "engine" / "core" / "semantic" / "UnitSystem.cpp",
    ROOT / "engine" / "parser" / "CMakeLists.txt",
    ROOT / "tools" / "regenerate-parser.ps1",
]

REQUIRED_LOCAL_INPUTS = [
    ROOT / "engine" / "core" / "CMakeLists.txt",
    ROOT / "engine" / "parser" / "grammar" / "SysML.g4",
    ROOT / "engine" / "parser" / "generated" / "SysMLParser.cpp",
    ROOT / "engine" / "parser" / "src" / "parser" / "SysMLParser.cpp",
    ROOT / "third_party" / "antlr4-runtime-4.13.2" / "CMakeLists.txt",
]


def main() -> int:
    failures: list[str] = []
    for path in REQUIRED_LOCAL_INPUTS:
        if not path.is_file():
            failures.append(f"missing repository input: {path.relative_to(ROOT)}")

    for path in ACTIVE_FILES:
        if not path.is_file():
            failures.append(f"missing production file: {path.relative_to(ROOT)}")
            continue
        text = path.read_text(encoding="utf-8").replace("\\", "/")
        for label, token in FORBIDDEN.items():
            if token in text:
                failures.append(f"{path.relative_to(ROOT)} still contains {label}: {token}")

    if failures:
        print("P4 self-contained gate failed:")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print("P4 self-contained gate passed: production build uses only repository-owned parser inputs.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
