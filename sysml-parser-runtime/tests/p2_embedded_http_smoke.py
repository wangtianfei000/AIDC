#!/usr/bin/env python3
"""Start the embedded runtime and verify its stable P2 HTTP surface."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import urllib.request
from pathlib import Path


def request_json(base_url: str, path: str, body: dict | None = None) -> tuple[int, dict]:
    data = None if body is None else json.dumps(body).encode("utf-8")
    request = urllib.request.Request(
        base_url + path,
        data=data,
        headers={"Content-Type": "application/json"},
        method="GET" if body is None else "POST",
    )
    with urllib.request.urlopen(request, timeout=20) as response:
        return response.status, json.load(response)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime", type=Path, required=True)
    args = parser.parse_args()

    runtime = args.runtime.resolve()
    environment = os.environ.copy()
    environment["PATH"] = str(runtime.parent) + os.pathsep + environment.get("PATH", "")
    process = subprocess.Popen(
        [str(runtime), "--port", "0"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        encoding="utf-8",
        env=environment,
    )
    try:
        assert process.stdout
        startup = json.loads(process.stdout.readline())
        base_url = f"http://127.0.0.1:{startup['port']}"

        status, health = request_json(base_url, "/health")
        assert status == 200 and health["status"] == "ok"
        assert health["parserMode"] == "embedded-antlr"
        assert health["engineVersion"]
        assert health["parserBaseline"].startswith(f"aidc-engine/{health['engineVersion']}+sha256:")

        status, capabilities = request_json(base_url, "/capabilities")
        assert status == 200 and capabilities["parserMode"] == "embedded-antlr"
        assert capabilities["engineVersion"] == health["engineVersion"]

        status, snapshot = request_json(
            base_url,
            "/parse",
            {"uri": "memory://p2-http.sysml", "text": "package P2 { part def Robot; }"},
        )
        assert status == 200 and snapshot["schemaVersion"] == "0.1"
        assert snapshot["document"]["parser"]["baseline"] == health["parserBaseline"]
        assert len(snapshot["elements"]) >= 2
        assert not any(item["severity"] == "error" for item in snapshot["diagnostics"])

        status, invalid = request_json(
            base_url,
            "/parse",
            {"uri": "memory://p2-invalid.sysml", "text": "package Broken { part def ;"},
        )
        assert status == 200
        assert any(item["severity"] == "error" for item in invalid["diagnostics"])
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()

    print(json.dumps({"embeddedHttp": "pass", "parserMode": "embedded-antlr"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
