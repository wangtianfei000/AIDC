#!/usr/bin/env python3
"""Capture canonical HTTP outputs from the frozen external-parser runtime."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import urllib.request
from pathlib import Path


def request_json(base_url: str, path: str, body: dict | None = None) -> dict:
    data = None if body is None else json.dumps(body).encode("utf-8")
    request = urllib.request.Request(
        base_url + path,
        data=data,
        headers={"Content-Type": "application/json"},
        method="GET" if body is None else "POST",
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.load(response)


def write_json(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    runtime_root = Path(__file__).resolve().parents[1]
    aidc_root = runtime_root.parent
    parser.add_argument(
        "--runtime",
        type=Path,
        default=runtime_root / "dist" / "bin" / "aidc-sysml-parser-runtime.exe",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=runtime_root / "migration" / "reference-baseline" / "behavior",
    )
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"Behavior baseline already exists; refusing to overwrite: {output}")
    output.mkdir(parents=True, exist_ok=True)

    runtime = args.runtime.resolve()
    environment = os.environ.copy()
    environment["PATH"] = str(runtime.parent) + os.pathsep + environment.get("PATH", "")
    process = subprocess.Popen(
        [str(runtime), "--port", "0"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        env=environment,
    )
    try:
        assert process.stdout
        startup = json.loads(process.stdout.readline())
        base_url = f"http://127.0.0.1:{startup['port']}"
        write_json(output / "health.json", request_json(base_url, "/health"))
        write_json(output / "capabilities.json", request_json(base_url, "/capabilities"))

        fixtures = {
            "architecture": aidc_root / "samples" / "jingang-10axis-system-design" / "architecture.sysml",
            "vehicle-mode": aidc_root / "samples" / "VehicleMode.sysml",
            "analysis-exception": aidc_root / "samples" / "analysis-exception-acceptance.sysml",
        }
        for name, fixture in fixtures.items():
            text = fixture.read_text(encoding="utf-8")
            uri = f"baseline:///{fixture.name}"
            write_json(output / f"{name}.model.json", request_json(base_url, "/parse", {"uri": uri, "text": text}))
            if name == "architecture":
                write_json(output / f"{name}.analysis.json", request_json(base_url, "/analyze", {"uri": uri, "text": text}))
        write_json(
            output / "invalid-syntax.model.json",
            request_json(
                base_url,
                "/parse",
                {"uri": "baseline:///invalid.sysml", "text": "package Broken { this is invalid @@@ }"},
            ),
        )
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
    print(json.dumps({"captured": True, "output": str(output), "files": len(list(output.glob("*.json")))}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
