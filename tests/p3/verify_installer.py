from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="aidc-installer-") as directory:
        root = Path(directory); source = root / "portable"; install = root / "installed"
        (source / "bin").mkdir(parents=True); (source / "bin" / "aidc.exe").write_bytes(b"test-core")
        (source / "distribution").mkdir(); (source / "distribution" / "README.md").write_text("test", encoding="utf-8")
        script = ROOT / "distribution" / "installer" / "install.ps1"
        completed = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script), "-Source", str(source), "-InstallRoot", str(install)], capture_output=True, text=True, encoding="utf-8", timeout=60, check=False)
        assert completed.returncode == 0, completed.stderr
        assert (install / "bin" / "aidc.exe").read_bytes() == b"test-core" and (install / "install-manifest.json").is_file()
        (install / "runtimes" / "mock").mkdir(parents=True); (install / "runtimes" / "mock" / "keep.txt").write_text("keep")
        (source / "bin" / "aidc.exe").write_bytes(b"test-core-v2")
        completed = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script), "-Source", str(source), "-InstallRoot", str(install)], capture_output=True, text=True, encoding="utf-8", timeout=60, check=False)
        assert completed.returncode == 0, completed.stderr
        assert (install / "bin" / "aidc.exe").read_bytes() == b"test-core-v2" and (install / "runtimes" / "mock" / "keep.txt").is_file()
        (source / "bin" / "aidc.exe").write_bytes(b"test-core-broken-upgrade")
        completed = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script), "-Source", str(source), "-InstallRoot", str(install), "-RuntimePack", f"openmodelica={root / 'missing.aidcpack'}"], capture_output=True, text=True, encoding="utf-8", timeout=60, check=False)
        assert completed.returncode != 0
        assert (install / "bin" / "aidc.exe").read_bytes() == b"test-core-v2"
        assert (install / "runtimes" / "mock" / "keep.txt").is_file()
        uninstall = ROOT / "distribution" / "installer" / "uninstall.ps1"
        completed = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(uninstall), "-InstallRoot", str(install)], capture_output=True, text=True, encoding="utf-8", timeout=60, check=False)
        assert completed.returncode == 0, completed.stderr
        assert not install.exists() and list(root.glob("AIDC-runtimes-*/mock/keep.txt"))

        runtime_source = root / "runtime-source" / "bin"
        runtime_source.mkdir(parents=True)
        (runtime_source / "omc.exe").write_bytes(b"omc")
        (runtime_source / "OMSimulator.exe").write_bytes(b"oms")
        pack = root / "openmodelica.aidcpack"
        completed = subprocess.run([sys.executable, str(ROOT / "distribution" / "scripts" / "build_runtime_pack.py"), "openmodelica", str(runtime_source.parent), str(pack)], capture_output=True, text=True, encoding="utf-8", timeout=60, check=False)
        assert completed.returncode == 0, completed.stderr
        with zipfile.ZipFile(pack) as archive:
            pack_manifest = json.loads(archive.read("pack-manifest.json"))
            assert pack_manifest["dependencyId"] == "openmodelica" and len(pack_manifest["files"]) == 2

        bundle = root / "offline.zip"
        completed = subprocess.run([sys.executable, str(ROOT / "distribution" / "scripts" / "build_offline_bundle.py"), str(source), str(bundle), "--pack", str(pack)], capture_output=True, text=True, encoding="utf-8", timeout=60, check=False)
        assert completed.returncode == 0, completed.stderr
        with zipfile.ZipFile(bundle) as archive: assert {"install.ps1", "uninstall.ps1", "offline-manifest.json", "product/bin/aidc.exe", "packs/openmodelica.aidcpack"}.issubset(archive.namelist())
        print(json.dumps({"passed": True, "installer": "transactional", "failedUpgradeRolledBack": True, "uninstallPreservedRuntime": True, "runtimePackBuilder": True, "offlineBundle": str(bundle)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
