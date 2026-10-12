from __future__ import annotations

import os
import sys
from pathlib import Path


def product_root() -> Path:
    if getattr(sys, "frozen", False):
        candidate = Path(sys.executable).resolve().parent.parent
        if (candidate / "distribution").is_dir():
            return candidate
    return Path(__file__).resolve().parents[1]


def state_root() -> Path:
    base = Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData" / "Local"))
    root = base / "AIDC" / "state"
    root.mkdir(parents=True, exist_ok=True)
    return root


def absolute_existing(value: str, kind: str = "path") -> Path:
    path = Path(value).expanduser().resolve()
    if not path.exists():
        from .result import AidcError
        raise AidcError(f"{kind}不存在：{path}", 3, "INPUT_NOT_FOUND", str(path))
    return path
