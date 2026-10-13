from __future__ import annotations

import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from aidc_core.geometry import GeometryBackendRegistry


class GeometryBackendContractTests(unittest.TestCase):
    def test_freecad_backend_is_the_only_registered_backend(self):
        registry = GeometryBackendRegistry(ROOT)
        self.assertEqual(registry.default_backend, "freecad-occ")
        self.assertEqual(registry.get().descriptor.status, "production")
        self.assertEqual([item["id"] for item in registry.list()], ["freecad-occ"])

    def test_backend_listing_uses_stable_setting_name(self):
        completed = subprocess.run(
            [sys.executable, str(ROOT / "aidc.py"), "geometry", "backends", "--json"],
            cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=False,
        )
        self.assertEqual(completed.returncode, 0, completed.stderr)
        document = json.loads(completed.stdout)
        self.assertEqual(document["data"]["setting"], "geometry.backend")
        self.assertEqual(document["data"]["default"], "freecad-occ")
        self.assertEqual({item["id"] for item in document["data"]["backends"]}, {"freecad-occ"})

    def test_removed_backend_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "unknown geometry backend"):
            GeometryBackendRegistry(ROOT).get("removed-backend")


if __name__ == "__main__":
    unittest.main()
