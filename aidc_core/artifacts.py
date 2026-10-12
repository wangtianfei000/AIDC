from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
from typing import Any

from .paths import state_root
from .result import AidcError, utc_now


ARTIFACT_TYPES = {
    "system.doctor": ("aidc.diagnostic.report", "application/vnd.aidc.diagnostics+json", "1.0"),
    "sysml.open": ("aidc.browser.session", "application/vnd.aidc.browser-session+json", "1.0"),
    "model3d.open": ("aidc.browser.session", "application/vnd.aidc.browser-session+json", "1.0"),
    "model3d.publish": ("aidc.model3d.package", "application/vnd.aidc.model3d-project", "0.1"),
    "model3d.export-modelica": ("aidc.assembly.package", "application/vnd.aidc.assembly-package", "1.0"),
    "modelica.generate": ("aidc.modelica.project", "application/vnd.aidc.modelica-project", "2.0"),
    "modelica.open": ("aidc.browser.session", "application/vnd.aidc.browser-session+json", "1.0"),
    "modelica.simulate": ("aidc.modelica.run", "application/vnd.aidc.modelica-run", "1.0"),
    "modelica.package": ("aidc.modelica.package", "application/vnd.aidc.modelica-package", "1.0"),
}


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


class ArtifactRegistry:
    def __init__(self, state_directory: Path | None = None):
        self.root = (state_directory or state_root()) / "artifacts"
        self.root.mkdir(parents=True, exist_ok=True)
        self.index = self.root / "index.json"

    def _load(self) -> dict[str, Any]:
        try:
            return json.loads(self.index.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return {"schemaVersion": "aidc.artifact-index/1.0", "artifacts": []}

    def _save(self, data: dict[str, Any]) -> None:
        temporary = self.index.with_suffix(".tmp")
        temporary.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
        os.replace(temporary, self.index)

    def record(self, document: dict[str, Any], task_id: str | None = None) -> list[dict[str, Any]]:
        if document.get("status") != "succeeded":
            return []
        command = str(document.get("command", ""))
        artifact_type = ARTIFACT_TYPES.get(command)
        candidates = list(document.get("artifacts") or [])
        data = document.get("data") if isinstance(document.get("data"), dict) else {}
        if artifact_type and not candidates and data.get("launchUrl"):
            candidates.append({"path": data.get("target") or data["launchUrl"], "metadata": {"url": data["launchUrl"], "instanceId": data.get("instanceId")}})
        if artifact_type and not candidates and data.get("output"):
            candidates.append({"path": data["output"]})
        if not artifact_type:
            return []
        type_id, media_type, schema_version = artifact_type
        index = self._load(); recorded = []
        for candidate in candidates:
            raw_path = candidate.get("path")
            if not raw_path:
                continue
            path = Path(raw_path).expanduser()
            resolved = str(path.resolve()) if path.exists() else str(raw_path)
            fingerprint = hashlib.sha256(f"{command}\0{resolved}\0{document.get('requestId')}".encode("utf-8")).hexdigest()[:24]
            descriptor = {
                "artifactId": f"artifact:{fingerprint}", "type": type_id, "path": resolved, "mediaType": media_type,
                "schemaVersion": schema_version, "sha256": _sha256(path) if path.is_file() else None,
                "sizeBytes": path.stat().st_size if path.is_file() else None, "createdAt": utc_now(),
                "metadata": {**candidate.get("metadata", {}), "command": command, "requestId": document.get("requestId"), "taskId": task_id},
            }
            index["artifacts"] = [item for item in index["artifacts"] if item["artifactId"] != descriptor["artifactId"]]
            index["artifacts"].append(descriptor); recorded.append(descriptor)
        if recorded:
            self._save(index)
        return recorded

    def list(self, task_id: str | None = None) -> list[dict[str, Any]]:
        items = self._load()["artifacts"]
        if task_id:
            items = [item for item in items if item.get("metadata", {}).get("taskId") == task_id]
        return list(reversed(items))

    def get(self, artifact_id: str) -> dict[str, Any]:
        item = next((item for item in self._load()["artifacts"] if item["artifactId"] == artifact_id), None)
        if not item:
            raise AidcError(f"产物不存在：{artifact_id}", 3, "ARTIFACT_NOT_FOUND")
        return item
