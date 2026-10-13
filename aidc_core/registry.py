from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

from .paths import product_root


class ComponentRegistry:
    def __init__(self, root: Path | None = None):
        self.root = root or product_root()

    def components(self) -> list[dict[str, Any]]:
        manifests = self.root / "distribution" / "manifests"
        return [json.loads(path.read_text(encoding="utf-8")) for path in sorted(manifests.glob("*.component.json"))]

    def capabilities(self) -> list[str]:
        return sorted({item for component in self.components() for item in component.get("capabilities", [])})

    def dependencies(self) -> list[dict[str, Any]]:
        path = self.root / "distribution" / "dependencies.lock.json"
        return json.loads(path.read_text(encoding="utf-8"))["dependencies"]

    def dependency_status(self, dependency_id: str | None = None) -> list[dict[str, Any]]:
        statuses = [self._dependency_status(item) for item in self.dependencies()]
        return [item for item in statuses if dependency_id is None or item["id"] == dependency_id]

    def _dependency_status(self, dependency: dict[str, Any]) -> dict[str, Any]:
        dep_id = dependency["id"]
        candidates: list[Path] = []
        if dep_id == "freecad":
            candidates = [
                self.root / "runtimes" / "freecad" / "bin" / "FreeCADCmd.exe",
                self.root.parent / "tools" / "freecad" / dependency["version"] / "bin" / "FreeCADCmd.exe",
            ]
        elif dep_id == "openmodelica":
            candidates = [self.root / "runtimes" / "openmodelica" / "bin" / "omc.exe", self.root.parent / "tools" / "openmodelica" / dependency["version"] / "bin" / "omc.exe"]
        elif dep_id == "python-runtime":
            candidates = [self.root / "runtimes" / "python" / ("python.exe" if sys.platform == "win32" else "python")]
            if not getattr(sys, "frozen", False): candidates.append(Path(sys.executable))
        elif dep_id == "sysml-parser-runtime":
            candidates = [
                self.root / "components" / "sysml" / "bin" / "aidc-sysml-parser-runtime.exe",
                self.root / "sysml-parser-runtime" / "dist" / "bin" / "aidc-sysml-parser-runtime-0.8.1.exe",
                self.root / "sysml-parser-runtime" / "dist" / "bin" / "aidc-sysml-parser-runtime.exe",
                self.root / "sysml-parser-runtime" / "build" / "bin" / "aidc-sysml-parser-runtime.exe",
            ]
        elif dep_id == "sysml-standard-library":
            candidates = [self.root / "components" / "sysml" / "library" / "sysml.library", self.root / "sysml-parser-runtime" / "dist" / "library" / "sysml.library"]
        resolved = next((path.resolve() for path in candidates if path.exists()), None)
        return {
            "id": dep_id,
            "name": dependency["name"],
            "version": dependency["version"],
            "available": resolved is not None,
            "path": str(resolved) if resolved else None,
            "distributionStatus": dependency["distributionStatus"],
            "blockers": dependency.get("blockers", []),
        }

    @staticmethod
    def executable(name: str) -> str | None:
        return shutil.which(name)

    @staticmethod
    def command_version(command: list[str], timeout: float = 5) -> str | None:
        try:
            completed = subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)
            return (completed.stdout or completed.stderr).strip().splitlines()[0]
        except (OSError, subprocess.SubprocessError, IndexError):
            return None
