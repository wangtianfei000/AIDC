#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path


HERE = Path(__file__).resolve()
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE.parents[1] / "src"))
from build_modelica_export_fixture import build
from export_modelica_assembly import ExportError, export_package


class ModelicaAssemblyExportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.fixture = build()

    def test_exports_and_reuses_a_valid_package(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "single-joint.aidc-assembly"
            first = export_package(self.fixture / "generated" / "model3d.manifest.json", self.fixture / "robot-ir", output)
            second = export_package(self.fixture / "generated" / "model3d.manifest.json", self.fixture / "robot-ir", output)
            self.assertEqual("miss", first["cacheStatus"])
            self.assertEqual("hit", second["cacheStatus"])
            self.assertEqual(2, first["geometryAssetCount"])
            self.assertEqual(
                {"assembly.manifest.json", "physics.json", "devices.json", "interfaces.json", "geometry/base.stl", "geometry/link.stl"},
                {p.relative_to(output).as_posix() for p in output.rglob("*") if p.is_file()},
            )

    def test_rejects_topology_drift(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            generated = root / "generated"
            shutil.copytree(self.fixture / "generated", generated)
            manifest_path = generated / "model3d.manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["joints"][0]["childBodyId"] = "base"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(ExportError, "关节类型或父子关系不一致"):
                export_package(manifest_path, self.fixture / "robot-ir", root / "output.aidc-assembly")


if __name__ == "__main__":
    unittest.main()
