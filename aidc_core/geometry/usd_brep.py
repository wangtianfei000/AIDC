from __future__ import annotations

import importlib
import os
import sys
import tempfile
import time
from contextlib import contextmanager
from pathlib import Path
from typing import Any

from .base import BackendDescriptor, CapabilityResult, GeometryBackend, VALIDATION_SCHEMA


@contextmanager
def _capture_native_stderr():
    """Capture warnings written by bundled native OpenUSD libraries to fd 2."""
    saved = os.dup(2)
    with tempfile.TemporaryFile(mode="w+b") as stream:
        try:
            sys.stderr.flush()
            os.dup2(stream.fileno(), 2)
            yield stream
        finally:
            sys.stderr.flush()
            os.dup2(saved, 2)
            os.close(saved)


class UsdBrepBackend(GeometryBackend):
    descriptor = BackendDescriptor(
        id="usd-brep",
        name="NVIDIA USD BRep",
        status="experimental",
        process_model="python",
        exact_format="OpenUSD BRepArray",
        capabilities=("primitive", "boolean", "distance", "measure", "tessellate", "usd-roundtrip"),
    )

    def _candidate_paths(self) -> list[Path]:
        configured = os.environ.get("AIDC_USD_BREP_PATH")
        paths = [
            self.product_root / "runtimes" / "python" / "Lib" / "site-packages",
            self.product_root / ".codex-runtime" / "usd-brep",
        ]
        if configured:
            paths.insert(0, Path(configured).expanduser())
        return [path.resolve() for path in paths if path.exists()]

    def _load(self):
        try:
            return importlib.import_module("usd_brep")
        except ImportError as first_error:
            for path in self._candidate_paths():
                value = str(path)
                if value not in sys.path:
                    sys.path.insert(0, value)
                try:
                    return importlib.import_module("usd_brep")
                except ImportError:
                    continue
            raise first_error

    def probe(self) -> dict[str, Any]:
        try:
            module = self._load()
            package = Path(module.__file__).resolve().parent
            return {
                "available": True,
                "version": getattr(module, "__version__", "1.0.0"),
                "path": str(package),
                "python": sys.version.split()[0],
                "error": None,
            }
        except (ImportError, OSError) as error:
            return {"available": False, "version": None, "path": None, "python": sys.version.split()[0], "error": str(error)}

    def validate(self) -> dict[str, Any]:
        started = time.perf_counter()
        probe = self.probe()
        if not probe["available"]:
            return self._report(probe, [], started, "unavailable")

        api = self._load()
        checks: list[CapabilityResult] = []

        def run(identifier, operation):
            try:
                message, metrics = operation()
                checks.append(CapabilityResult(identifier, True, message, metrics))
            except Exception as error:  # backend errors are validation evidence
                checks.append(CapabilityResult(identifier, False, f"{type(error).__name__}: {error}"))

        with _capture_native_stderr() as native_stderr:
            run("primitive", lambda: self._primitive(api))
            run("boolean", lambda: self._boolean(api))
            run("distance", lambda: self._distance(api))
            run("tessellate", lambda: self._tessellate(api))
            run("usd-roundtrip", lambda: self._roundtrip(api))
            native_stderr.seek(0)
            warnings = native_stderr.read().decode("utf-8", errors="replace").strip()
        state = "passed" if checks and all(item.passed for item in checks) else "failed"
        report = self._report(probe, checks, started, state)
        report["runtimeWarnings"] = warnings.splitlines()[:50]
        report["formatCoverage"] = {
            "exactImport": [".usd", ".usda", ".usdc", ".smb", ".smp"],
            "exactExport": [".usd", ".usda", ".usdc"],
            "unsupportedAIDCSource": [".FCStd", ".step", ".stp", ".iges", ".igs", ".jt"],
        }
        report["productionQualification"] = {
            "ready": False,
            "blockers": [
                "The public Python API has no direct FCStd, STEP, IGES or JT importer",
                "Real AIDC robot fixtures have not completed cross-kernel topology and mass-property comparison",
                "Native OpenUSD loader warnings must be qualified or eliminated before embedding",
            ],
        }
        return report

    def _primitive(self, api):
        box = api.create_box((0.0, 0.0, 0.0), 10.0, 10.0, 10.0)
        volume = float(api.compute_volume(box, 1e-8))
        bounds = api.brep_bounding_box(box, True)
        if abs(volume - 1000.0) > 1e-6:
            raise ValueError(f"unexpected box volume {volume}")
        return "基础实体、包围盒和体积计算通过", {"volume": volume, "bounds": bounds}

    def _boolean(self, api):
        first = api.create_box((0.0, 0.0, 0.0), 10.0, 10.0, 10.0)
        second = api.create_box((5.0, 0.0, 0.0), 10.0, 10.0, 10.0)
        common = api.boolean_intersection(first, second)
        volume = float(api.compute_volume(common, 1e-8))
        if abs(volume - 500.0) > 1e-5:
            raise ValueError(f"unexpected common volume {volume}")
        return "BRep 布尔求交通过", {"commonVolume": volume}

    def _distance(self, api):
        first = api.create_box((0.0, 0.0, 0.0), 10.0, 10.0, 10.0)
        second = api.create_box((11.0, 0.0, 0.0), 10.0, 10.0, 10.0)
        distance = float(api.brep_distance(first, second))
        if abs(distance - 1.0) > 1e-6:
            raise ValueError(f"unexpected distance {distance}")
        return "BRep 最小距离计算通过", {"distance": distance}

    def _tessellate(self, api):
        box = api.create_box((0.0, 0.0, 0.0), 10.0, 10.0, 10.0)
        mesh, report = api.tessellate(box, chord_height_tolerance=0.01, surface_angle_tolerance_deg=15.0, return_report=True)
        mesh_volume = float(api.poly_brep_volume_via_api(mesh))
        if abs(mesh_volume - 1000.0) > 1e-4:
            raise ValueError(f"unexpected mesh volume {mesh_volume}")
        return "精确 BRep 网格化通过", {"meshVolume": mesh_volume, "report": str(report)}

    def _roundtrip(self, api):
        with tempfile.TemporaryDirectory(prefix="aidc-usd-brep-") as directory:
            path = Path(directory) / "box.usda"
            api.usd.export_brep(api.create_box((0.0, 0.0, 0.0), 10.0, 10.0, 10.0), str(path))
            imported = api.usd.import_breps(str(path))
            if len(imported) != 1:
                raise ValueError(f"expected one BRep, got {len(imported)}")
            volume = float(api.compute_volume(imported[0], 1e-8))
            if abs(volume - 1000.0) > 1e-5:
                raise ValueError(f"unexpected round-trip volume {volume}")
            return "OpenUSD BRep 导出和导入通过", {"brepCount": 1, "volume": volume, "fileBytes": path.stat().st_size}

    def _report(self, probe, checks, started, state):
        return {
            "schemaVersion": VALIDATION_SCHEMA,
            "backend": self.descriptor.to_dict(),
            "probe": probe,
            "state": state,
            "durationMs": round((time.perf_counter() - started) * 1000, 2),
            "checks": [item.to_dict() for item in checks],
        }

