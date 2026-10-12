from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import urllib.request
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from uuid import uuid4
from urllib.parse import urlsplit

from .paths import state_root
from .result import AidcError


RUNTIME_DIRECTORIES = {
    "freecad": "freecad",
    "openmodelica": "openmodelica",
    "python-runtime": "python",
}


def _now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


class DependencyManager:
    def __init__(self, root: Path, state_directory: Path | None = None):
        self.root = root.resolve()
        self.runtime_root = self.root / "runtimes"
        self.metadata_root = self.runtime_root / ".aidc"
        self.rollback_root = self.runtime_root / ".rollback"
        self.staging_root = self.runtime_root / ".staging"
        self.cache_root = (state_directory or state_root()) / "runtime-packs"
        for path in (self.metadata_root, self.rollback_root, self.staging_root, self.cache_root):
            path.mkdir(parents=True, exist_ok=True)
        self.dependencies = {item["id"]: item for item in json.loads((self.root / "distribution" / "dependencies.lock.json").read_text(encoding="utf-8"))["dependencies"]}
        catalog_path = self.root / "distribution" / "runtime-packs" / "catalog.json"
        self.catalog = json.loads(catalog_path.read_text(encoding="utf-8")) if catalog_path.is_file() else {"packs": []}

    def _dependency(self, dependency_id: str) -> dict[str, Any]:
        dependency = self.dependencies.get(dependency_id)
        if dependency is None or dependency_id not in RUNTIME_DIRECTORIES:
            raise AidcError(f"未知或非可安装依赖：{dependency_id}", 3, "DEPENDENCY_UNKNOWN")
        return dependency

    def _target(self, dependency_id: str) -> Path:
        return self.runtime_root / RUNTIME_DIRECTORIES[dependency_id]

    def _receipt_path(self, dependency_id: str) -> Path:
        return self.metadata_root / f"{dependency_id}.json"

    def _read_receipt(self, dependency_id: str) -> dict[str, Any] | None:
        path = self._receipt_path(dependency_id)
        try: return json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError): return None

    def status(self, dependency_id: str) -> dict[str, Any]:
        dependency = self._dependency(dependency_id)
        target = self._target(dependency_id)
        receipt = self._read_receipt(dependency_id)
        if not receipt or not target.is_dir():
            return {"id": dependency_id, "name": dependency["name"], "version": dependency["version"], "installed": False, "healthy": False, "path": None, "issues": ["not-installed"], "receipt": None}
        issues = []
        for item in receipt.get("files", []):
            path = (target / item["path"]).resolve()
            if target != path and target not in path.parents:
                issues.append(f"unsafe-path:{item['path']}")
            elif not path.is_file():
                issues.append(f"missing:{item['path']}")
            elif _sha256(path) != item["sha256"]:
                issues.append(f"digest:{item['path']}")
        health = self._health(target, receipt["healthCheck"])
        if not health["passed"]: issues.append("health-check")
        return {"id": dependency_id, "name": dependency["name"], "version": receipt["version"], "installed": True, "healthy": not issues, "path": str(target), "issues": issues, "healthCheck": health, "receipt": str(self._receipt_path(dependency_id))}

    def _health(self, root: Path, check: dict[str, Any]) -> dict[str, Any]:
        target = (root / check["target"]).resolve()
        if root != target and root not in target.parents:
            return {"passed": False, "message": "health target escapes runtime"}
        if check["type"] == "file":
            return {"passed": target.is_file(), "target": str(target), "output": "file exists" if target.is_file() else "file missing"}
        if not target.is_file(): return {"passed": False, "target": str(target), "output": "command missing"}
        command = [str(target), *check.get("arguments", [])]
        if target.suffix.lower() in {".cmd", ".bat"}: command = [os.environ.get("COMSPEC", "cmd.exe"), "/d", "/c", *command]
        try:
            completed = subprocess.run(command, cwd=target.parent, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=check.get("timeoutSeconds", 30), check=False)
            output = (completed.stdout + completed.stderr).strip()
            expected = check.get("expected", "")
            return {"passed": completed.returncode == 0 and expected in output, "target": str(target), "exitCode": completed.returncode, "output": output[-2000:]}
        except (OSError, subprocess.SubprocessError) as exc:
            return {"passed": False, "target": str(target), "output": str(exc)}

    def _catalog_source(self, dependency_id: str) -> tuple[str, str | None]:
        item = next((value for value in self.catalog.get("packs", []) if value.get("dependencyId") == dependency_id and value.get("version") == self.dependencies[dependency_id]["version"]), None)
        if not item or not item.get("source"):
            blockers = "; ".join(self.dependencies[dependency_id].get("blockers", []))
            raise AidcError(f"依赖包尚未发布，请使用 --source 指定已审核的离线包。{blockers}", 4, "PACK_SOURCE_REQUIRED")
        return item["source"], item.get("sha256")

    def _obtain(self, dependency_id: str, source: str | None) -> Path:
        expected = None
        if not source: source, expected = self._catalog_source(dependency_id)
        candidate = Path(source).expanduser()
        if not candidate.is_absolute(): candidate = self.root / "distribution" / "runtime-packs" / candidate
        if candidate.is_file(): archive = candidate.resolve()
        elif source.startswith(("https://", "http://")):
            name = Path(urlsplit(source).path).name or f"{dependency_id}.aidcpack"
            partial = self.cache_root / (name + ".partial"); archive = self.cache_root / name
            headers = {"Range": f"bytes={partial.stat().st_size}-"} if partial.is_file() else {}
            request = urllib.request.Request(source, headers=headers)
            try:
                with urllib.request.urlopen(request, timeout=60) as response, partial.open("ab" if response.status == 206 else "wb") as stream:
                    shutil.copyfileobj(response, stream)
                os.replace(partial, archive)
            except OSError as exc: raise AidcError(f"下载依赖包失败：{exc}", 6, "PACK_DOWNLOAD_FAILED") from exc
        else: raise AidcError(f"依赖包不存在：{source}", 3, "PACK_NOT_FOUND", str(candidate))
        digest = _sha256(archive)
        if expected and digest != expected: raise AidcError("依赖包 SHA-256 不匹配", 3, "PACK_DIGEST_MISMATCH", str(archive))
        cached = self.cache_root / f"{digest}.aidcpack"
        if archive != cached and not cached.is_file(): shutil.copy2(archive, cached)
        return cached

    @staticmethod
    def _safe_member(name: str) -> bool:
        path = Path(name.replace("\\", "/"))
        return not path.is_absolute() and ".." not in path.parts and bool(path.parts)

    def _extract(self, archive: Path, dependency_id: str) -> tuple[Path, dict[str, Any]]:
        staging = self.staging_root / f"{dependency_id}-{uuid4().hex}"
        staging.mkdir(parents=True)
        try:
            with zipfile.ZipFile(archive) as package:
                if any(not self._safe_member(item.filename) for item in package.infolist()): raise AidcError("依赖包包含非法路径", 3, "PACK_PATH_UNSAFE")
                package.extractall(staging)
            manifest_path = staging / "pack-manifest.json"
            if not manifest_path.is_file(): raise AidcError("依赖包缺少 pack-manifest.json", 3, "PACK_MANIFEST_MISSING")
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            required = {"schemaVersion", "packId", "dependencyId", "version", "platform", "architecture", "entryPoints", "healthCheck", "files"}
            if manifest.get("schemaVersion") != "aidc.runtime-pack/1.0" or not required.issubset(manifest): raise AidcError("依赖包清单无效", 3, "PACK_MANIFEST_INVALID")
            if manifest["dependencyId"] != dependency_id: raise AidcError("依赖包 ID 与请求不匹配", 3, "PACK_ID_MISMATCH")
            if manifest["platform"] != "windows" or manifest["architecture"] != "x86_64": raise AidcError("依赖包平台不兼容", 5, "PACK_PLATFORM_UNSUPPORTED")
            payload = staging / "payload"
            if any(not self._safe_member(path) for path in manifest["entryPoints"].values()): raise AidcError("依赖包入口路径非法", 3, "PACK_PATH_UNSAFE")
            for item in manifest["files"]:
                path = (payload / item["path"]).resolve()
                if payload != path and payload not in path.parents: raise AidcError("依赖包文件路径非法", 3, "PACK_PATH_UNSAFE")
                if not path.is_file() or path.stat().st_size != item["size"] or _sha256(path) != item["sha256"]: raise AidcError(f"依赖包文件校验失败：{item['path']}", 3, "PACK_CONTENT_INVALID")
            health = self._health(payload, manifest["healthCheck"])
            if not health["passed"]: raise AidcError(f"依赖包健康检查失败：{health['output']}", 6, "PACK_HEALTH_FAILED")
            return staging, manifest
        except Exception:
            shutil.rmtree(staging, ignore_errors=True)
            raise

    def install(self, dependency_id: str, source: str | None = None, *, repair: bool = False) -> dict[str, Any]:
        dependency = self._dependency(dependency_id)
        archive = self._obtain(dependency_id, source)
        staging, manifest = self._extract(archive, dependency_id)
        if manifest["version"] != dependency["version"]:
            shutil.rmtree(staging, ignore_errors=True)
            raise AidcError(f"依赖包版本 {manifest['version']} 与锁定版本 {dependency['version']} 不匹配", 5, "PACK_VERSION_MISMATCH")
        target = self._target(dependency_id); receipt_path = self._receipt_path(dependency_id)
        backup = None
        try:
            if target.exists():
                backup = self.rollback_root / dependency_id / datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-%f")
                backup.mkdir(parents=True); os.replace(target, backup / "payload")
                if receipt_path.is_file(): shutil.copy2(receipt_path, backup / "receipt.json")
            os.replace(staging / "payload", target)
            receipt = {"schemaVersion": "aidc.runtime-install/1.0", "dependencyId": dependency_id, "packId": manifest["packId"], "version": manifest["version"], "installedAt": _now(), "archiveSha256": _sha256(archive), "cachedArchive": str(archive), "entryPoints": manifest["entryPoints"], "healthCheck": manifest["healthCheck"], "files": manifest["files"]}
            receipt_path.write_text(json.dumps(receipt, ensure_ascii=False, indent=2), encoding="utf-8")
            result = self.status(dependency_id)
            if not result["healthy"]: raise AidcError("安装后健康检查失败", 6, "PACK_INSTALL_UNHEALTHY")
            return {**result, "operation": "repair" if repair else "install", "packId": manifest["packId"], "archiveSha256": receipt["archiveSha256"], "rollbackAvailable": backup is not None}
        except Exception:
            if target.exists(): shutil.rmtree(target, ignore_errors=True)
            if backup and (backup / "payload").exists():
                os.replace(backup / "payload", target)
                if (backup / "receipt.json").is_file(): shutil.copy2(backup / "receipt.json", receipt_path)
                else: receipt_path.unlink(missing_ok=True)
            else: receipt_path.unlink(missing_ok=True)
            raise
        finally: shutil.rmtree(staging, ignore_errors=True)

    def repair(self, dependency_id: str, source: str | None = None) -> dict[str, Any]:
        receipt = self._read_receipt(dependency_id)
        if source is None and receipt: source = receipt.get("cachedArchive")
        return self.install(dependency_id, source, repair=True)

    def rollback(self, dependency_id: str) -> dict[str, Any]:
        self._dependency(dependency_id)
        base = self.rollback_root / dependency_id
        candidates = sorted((item for item in base.iterdir() if (item / "payload").is_dir()), reverse=True) if base.is_dir() else []
        if not candidates: raise AidcError("没有可回滚的运行时", 7, "ROLLBACK_UNAVAILABLE")
        chosen = candidates[0]; target = self._target(dependency_id); failed = self.staging_root / f"rollback-{dependency_id}-{uuid4().hex}"
        receipt_path = self._receipt_path(dependency_id)
        previous_receipt = receipt_path.read_bytes() if receipt_path.is_file() else None
        if target.exists(): os.replace(target, failed)
        try:
            os.replace(chosen / "payload", target)
            receipt = chosen / "receipt.json"
            if receipt.is_file(): shutil.copy2(receipt, receipt_path)
            else: receipt_path.unlink(missing_ok=True)
            result = self.status(dependency_id)
            if not result["healthy"]: raise AidcError("回滚后健康检查失败", 6, "ROLLBACK_UNHEALTHY")
            shutil.rmtree(chosen, ignore_errors=True); shutil.rmtree(failed, ignore_errors=True)
            return {**result, "operation": "rollback"}
        except Exception:
            if target.exists(): shutil.rmtree(target, ignore_errors=True)
            if failed.exists(): os.replace(failed, target)
            if previous_receipt is None: receipt_path.unlink(missing_ok=True)
            else: receipt_path.write_bytes(previous_receipt)
            raise

    def uninstall(self, dependency_id: str) -> dict[str, Any]:
        self._dependency(dependency_id); target = self._target(dependency_id)
        removed = target.exists()
        if removed: shutil.rmtree(target)
        self._receipt_path(dependency_id).unlink(missing_ok=True)
        return {"id": dependency_id, "operation": "uninstall", "removed": removed, "path": str(target)}
