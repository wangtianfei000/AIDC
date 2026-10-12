from __future__ import annotations

import hashlib
import json
import shutil
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
import sys
sys.path.insert(0, str(ROOT))

from aidc_core.dependencies import DependencyManager
from aidc_core.result import AidcError


def make_pack(path: Path, label: str) -> Path:
    payload = f"@echo off\r\necho OpenModelica v1.27.1 {label}\r\n".encode()
    manifest = {
        "schemaVersion": "aidc.runtime-pack/1.0", "packId": "modelica-pack", "dependencyId": "openmodelica", "version": "1.27.1",
        "platform": "windows", "architecture": "x86_64", "createdAt": "2026-10-12T00:00:00Z", "entryPoints": {"compiler": "bin/omc.cmd"},
        "healthCheck": {"type": "command", "target": "bin/omc.cmd", "arguments": ["--version"], "expected": "OpenModelica v1.27.1", "timeoutSeconds": 10},
        "files": [{"path": "bin/omc.cmd", "size": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}],
    }
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("pack-manifest.json", json.dumps(manifest)); archive.writestr("payload/bin/omc.cmd", payload)
    return path


class DependencyManagerTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="aidc-p3-"); self.root = Path(self.temporary.name) / "product"
        (self.root / "distribution" / "runtime-packs").mkdir(parents=True)
        shutil.copy2(ROOT / "distribution" / "dependencies.lock.json", self.root / "distribution" / "dependencies.lock.json")
        shutil.copy2(ROOT / "distribution" / "runtime-packs" / "catalog.json", self.root / "distribution" / "runtime-packs" / "catalog.json")
        self.manager = DependencyManager(self.root, Path(self.temporary.name) / "state")

    def tearDown(self): self.temporary.cleanup()

    def test_install_verify_repair_upgrade_and_rollback(self):
        first = make_pack(Path(self.temporary.name) / "first.aidcpack", "first")
        second = make_pack(Path(self.temporary.name) / "second.aidcpack", "second")
        installed = self.manager.install("openmodelica", str(first)); self.assertTrue(installed["healthy"])
        compiler = self.root / "runtimes" / "openmodelica" / "bin" / "omc.cmd"
        compiler.write_text("damaged", encoding="utf-8"); self.assertFalse(self.manager.status("openmodelica")["healthy"])
        self.assertTrue(self.manager.repair("openmodelica")["healthy"]); self.assertIn("first", compiler.read_text())
        self.manager.install("openmodelica", str(second)); self.assertIn("second", compiler.read_text())
        rolled = self.manager.rollback("openmodelica"); self.assertTrue(rolled["healthy"]); self.assertIn("first", compiler.read_text())
        removed = self.manager.uninstall("openmodelica"); self.assertTrue(removed["removed"]); self.assertFalse(compiler.exists())

    def test_rejects_archive_path_traversal(self):
        archive = Path(self.temporary.name) / "unsafe.aidcpack"
        with zipfile.ZipFile(archive, "w") as package:
            package.writestr("pack-manifest.json", "{}"); package.writestr("../escape.txt", "bad")
        with self.assertRaises(AidcError) as caught: self.manager.install("openmodelica", str(archive))
        self.assertEqual(caught.exception.code, "PACK_PATH_UNSAFE")


if __name__ == "__main__": unittest.main()
