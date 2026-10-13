from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from aidc_core.geometry import GeometryBackendRegistry


class GeometryBackendContractTests(unittest.TestCase):
    def test_default_and_experimental_backends_are_registered(self):
        registry = GeometryBackendRegistry(ROOT)
        self.assertEqual(registry.default_backend, "freecad-occ")
        self.assertEqual(registry.get().descriptor.status, "production")
        self.assertEqual(registry.get("usd-brep").descriptor.status, "experimental")

    def test_backend_listing_uses_stable_setting_name(self):
        completed = subprocess.run(
            [sys.executable, str(ROOT / "aidc.py"), "geometry", "backends", "--json"],
            cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=False,
        )
        self.assertEqual(completed.returncode, 0, completed.stderr)
        document = json.loads(completed.stdout)
        self.assertEqual(document["data"]["setting"], "geometry.backend")
        self.assertEqual(document["data"]["default"], "freecad-occ")
        self.assertEqual({item["id"] for item in document["data"]["backends"]}, {"freecad-occ", "usd-brep"})

    def test_usd_brep_capability_validation_when_runtime_is_available(self):
        backend = GeometryBackendRegistry(ROOT).get("usd-brep")
        if not backend.probe()["available"]:
            self.skipTest("usd-brep experimental runtime is not installed")
        report = backend.validate()
        self.assertEqual(report["state"], "passed", report)
        self.assertEqual({item["id"] for item in report["checks"]}, {"primitive", "boolean", "distance", "tessellate", "usd-roundtrip"})
        self.assertTrue(all(item["passed"] for item in report["checks"]))
        self.assertFalse(report["productionQualification"]["ready"])
        self.assertIn(".step", report["formatCoverage"]["unsupportedAIDCSource"])

    def test_cli_writes_a_machine_readable_validation_report(self):
        backend = GeometryBackendRegistry(ROOT).get("usd-brep")
        if not backend.probe()["available"]:
            self.skipTest("usd-brep experimental runtime is not installed")
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "geometry-validation.json"
            completed = subprocess.run(
                [sys.executable, str(ROOT / "aidc.py"), "geometry", "validate", "--backend", "usd-brep", "--report", str(report), "--json"],
                cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertEqual(completed.stderr, "")
            document = json.loads(completed.stdout)
            self.assertEqual(document["data"]["schemaVersion"], "aidc.geometry.validation/0.1")
            self.assertEqual(json.loads(report.read_text(encoding="utf-8"))["state"], "passed")


if __name__ == "__main__":
    unittest.main()
