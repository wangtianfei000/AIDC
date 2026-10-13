#!/usr/bin/env python3
"""Reject absolute or reference-project paths in embedded P1 sources and build files."""

from __future__ import annotations

import re
from pathlib import Path


RUNTIME_ROOT = Path(__file__).resolve().parents[1]
SCOPES = (
    RUNTIME_ROOT / "engine",
    RUNTIME_ROOT / "third_party" / "antlr4-runtime-4.13.2",
)
TEXT_SUFFIXES = {".cmake", ".cpp", ".h", ".hpp", ".md", ".txt", ".json"}
FORBIDDEN = (
    re.compile(r"(?<![A-Za-z])[A-Za-z]:[\\/]"),
    re.compile(r"REFERENCE_ROOT|REFERENCE_BUILD"),
    re.compile(r"7\.AWorks[/\\]BlockSim"),
)


def main() -> int:
    violations: list[str] = []
    for scope in SCOPES:
        for path in scope.rglob("*"):
            if not path.is_file() or (path.suffix.lower() not in TEXT_SUFFIXES and path.name != "CMakeLists.txt"):
                continue
            if path.name == "P1-IMPORT.json":
                continue
            text = path.read_text(encoding="utf-8", errors="strict")
            for line_number, line in enumerate(text.splitlines(), 1):
                if any(pattern.search(line) for pattern in FORBIDDEN):
                    violations.append(f"{path.relative_to(RUNTIME_ROOT).as_posix()}:{line_number}: {line.strip()}")
    if violations:
        raise SystemExit("Embedded source boundary violations:\n" + "\n".join(violations))
    print("Embedded source boundary check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
