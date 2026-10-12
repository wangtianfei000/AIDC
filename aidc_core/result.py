from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Any
from uuid import uuid4

from . import __version__


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


@dataclass
class CommandResult:
    command: str
    message: str
    data: Any = None
    status: str = "succeeded"
    exit_code: int = 0
    diagnostics: list[dict[str, Any]] = field(default_factory=list)
    artifacts: list[dict[str, Any]] = field(default_factory=list)
    task: dict[str, Any] | None = None
    request_id: str = field(default_factory=lambda: str(uuid4()))
    started_at: str = field(default_factory=utc_now)

    def finish(self) -> dict[str, Any]:
        return {
            "schemaVersion": "1.0",
            "requestId": self.request_id,
            "command": self.command,
            "status": self.status,
            "exitCode": self.exit_code,
            "message": self.message,
            "data": self.data,
            "task": self.task,
            "diagnostics": self.diagnostics,
            "artifacts": self.artifacts,
            "meta": {"toolVersion": __version__, "startedAt": self.started_at, "finishedAt": utc_now()},
        }


class AidcError(Exception):
    def __init__(self, message: str, exit_code: int = 6, code: str = "TASK_FAILED", path: str | None = None):
        super().__init__(message)
        self.exit_code = exit_code
        self.code = code
        self.path = path
