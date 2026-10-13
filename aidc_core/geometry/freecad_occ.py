from __future__ import annotations

import json
import subprocess
import tempfile
import time
from pathlib import Path
from typing import Any

from aidc_core.registry import ComponentRegistry

from .base import BackendDescriptor, GeometryBackend, VALIDATION_SCHEMA


VALIDATION_SCRIPT = r'''
import json, tempfile
import FreeCAD as App
import MeshPart, Part
a=Part.makeBox(10,10,10)
b=Part.makeBox(10,10,10,App.Vector(5,0,0))
gap=Part.makeBox(10,10,10,App.Vector(11,0,0))
common=a.common(b)
mesh=MeshPart.meshFromShape(Shape=a,LinearDeflection=0.01,AngularDeflection=0.261799,Relative=False)
print("AIDC_GEOMETRY_RESULT="+json.dumps({
 "volume":float(a.Volume),"commonVolume":float(common.Volume),
 "distance":float(a.distToShape(gap)[0]),"facets":int(mesh.CountFacets)
}))
'''


class FreeCadOccBackend(GeometryBackend):
    descriptor = BackendDescriptor(
        id="freecad-occ",
        name="FreeCAD / OpenCascade",
        status="production",
        process_model="isolated-command",
        exact_format="FCStd / OpenCascade TopoDS",
        capabilities=("primitive", "boolean", "distance", "measure", "tessellate", "cad-document"),
    )

    def _executable(self) -> Path | None:
        status = ComponentRegistry(self.product_root).dependency_status("freecad")
        return Path(status[0]["path"]) if status and status[0]["path"] else None

    def probe(self) -> dict[str, Any]:
        executable = self._executable()
        if executable is None:
            return {"available": False, "version": None, "path": None, "error": "managed FreeCAD runtime not found"}
        version = ComponentRegistry.command_version([str(executable), "--version"], 30)
        return {"available": True, "version": version, "path": str(executable), "error": None}

    def validate(self) -> dict[str, Any]:
        started = time.perf_counter()
        probe = self.probe()
        if not probe["available"]:
            return self._report(probe, "unavailable", [], started)
        with tempfile.TemporaryDirectory(prefix="aidc-freecad-occ-") as directory:
            script = Path(directory) / "validate.py"
            script.write_text(VALIDATION_SCRIPT, encoding="utf-8")
            completed = subprocess.run(
                [probe["path"], str(script)], capture_output=True, text=True,
                encoding="utf-8", errors="replace", timeout=120, check=False,
            )
        marker = next((line.split("=", 1)[1] for line in completed.stdout.splitlines() if line.startswith("AIDC_GEOMETRY_RESULT=")), None)
        if completed.returncode or marker is None:
            detail = (completed.stderr or completed.stdout).strip()
            return self._report(probe, "failed", [{"id": "kernel-smoke", "passed": False, "message": detail, "metrics": {}}], started)
        metrics = json.loads(marker)
        passed = abs(metrics["volume"] - 1000.0) < 1e-6 and abs(metrics["commonVolume"] - 500.0) < 1e-6 and abs(metrics["distance"] - 1.0) < 1e-6 and metrics["facets"] > 0
        check = {"id": "kernel-smoke", "passed": passed, "message": "OpenCascade 基础实体、布尔、距离、体积和网格化通过" if passed else "OpenCascade 结果超出能力基线", "metrics": metrics}
        return self._report(probe, "passed" if passed else "failed", [check], started)

    def _report(self, probe, state, checks, started):
        return {"schemaVersion": VALIDATION_SCHEMA, "backend": self.descriptor.to_dict(), "probe": probe, "state": state, "durationMs": round((time.perf_counter()-started)*1000, 2), "checks": checks}

