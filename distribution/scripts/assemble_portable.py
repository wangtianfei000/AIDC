from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "distribution" / "out" / "aidc-portable"


def copy_tree(source: Path, target: Path) -> None:
    shutil.copytree(source, target, dirs_exist_ok=True, ignore=shutil.ignore_patterns("__pycache__", "*.pyc", "node_modules", "dist", "build", "results", "generated"))


def require(path: Path) -> Path:
    if not path.exists(): raise SystemExit(f"缺少发行输入：{path}")
    return path


def main() -> int:
    shutil.rmtree(OUTPUT, ignore_errors=True)
    (OUTPUT / "bin").mkdir(parents=True, exist_ok=True)
    shutil.copy2(require(ROOT / "dist" / "aidc.exe"), OUTPUT / "bin" / "aidc.exe")
    shutil.copy2(require(ROOT / "dist" / "aidc-mcp.exe"), OUTPUT / "bin" / "aidc-mcp.exe")
    for domain, source in {
        "sysml": ROOT / "sysml-browser" / "dist",
        "model3d": ROOT / "model3d-viewer" / "apps" / "browser" / "dist",
        "modelica": ROOT / "modelica-browser" / "apps" / "web" / "dist",
    }.items(): copy_tree(require(source), OUTPUT / "components" / domain / "browser")

    runtime_bin = require(ROOT / "sysml-parser-runtime" / "dist" / "bin")
    sysml_bin = OUTPUT / "components" / "sysml" / "bin"; sysml_bin.mkdir(parents=True)
    shutil.copy2(require(runtime_bin / "aidc-sysml-parser-runtime-0.8.1.exe"), sysml_bin / "aidc-sysml-parser-runtime.exe")
    for item in runtime_bin.rglob("*"):
        if item.is_file() and (item.suffix.lower() == ".dll"):
            destination = sysml_bin / item.relative_to(runtime_bin); destination.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(item, destination)
    copy_tree(require(ROOT / "sysml-parser-runtime" / "dist" / "library"), OUTPUT / "components" / "sysml" / "library")

    modelica = OUTPUT / "components" / "modelica"
    (modelica / "server").mkdir(parents=True); (modelica / "tools").mkdir(parents=True)
    shutil.copy2(require(ROOT / "dist" / "aidc-modelica-server.exe"), modelica / "server" / "aidc-modelica-server.exe")
    shutil.copy2(require(ROOT / "dist" / "aidc-modelica-tools.exe"), modelica / "tools" / "aidc-modelica-tools.exe")
    copy_tree(ROOT / "modelica-browser" / "packages", modelica / "packages")
    copy_tree(ROOT / "modelica-browser" / "libraries", modelica / "libraries")

    model3d_tools = OUTPUT / "components" / "model3d" / "tools"; model3d_tools.mkdir(parents=True)
    shutil.copy2(require(ROOT / "dist" / "aidc-model3d-export.exe"), model3d_tools / "aidc-model3d-export.exe")
    shutil.copy2(ROOT / "model3d-viewer" / "tools" / "freecad-publisher" / "src" / "publish_model.py", model3d_tools / "publish_model.py")
    exact_service = OUTPUT / "components" / "model3d" / "services" / "exact-check"; exact_service.mkdir(parents=True)
    shutil.copy2(ROOT / "model3d-viewer" / "services" / "exact-check" / "src" / "server.py", exact_service / "server.py")
    for name in ("contracts", "licenses", "manifests", "schemas"):
        copy_tree(ROOT / "distribution" / name, OUTPUT / "distribution" / name)
    copy_tree(ROOT / "distribution" / "runtime-packs", OUTPUT / "distribution" / "runtime-packs")
    copy_tree(ROOT / "distribution" / "installer", OUTPUT / "installer")
    copy_tree(ROOT / "integrations", OUTPUT / "integrations")
    for name in ("dependencies.lock.json", "sbom.cdx.json", "README.md"):
        shutil.copy2(ROOT / "distribution" / name, OUTPUT / "distribution" / name)
    copy_tree(ROOT / "skills", OUTPUT / "skills")

    files = []
    for path in sorted(item for item in OUTPUT.rglob("*") if item.is_file()):
        if path.name == "portable-manifest.json": continue
        files.append({"path": path.relative_to(OUTPUT).as_posix(), "size": path.stat().st_size, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    manifest = {"schemaVersion": "aidc.portable/0.1", "productVersion": "0.6.0", "distributionStatus": "internal-acceptance", "files": files}
    (OUTPUT / "portable-manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"output": str(OUTPUT), "fileCount": len(files), "nodeModules": any(item.name == "node_modules" for item in OUTPUT.rglob("*"))}, ensure_ascii=False))
    return 0


if __name__ == "__main__": raise SystemExit(main())
