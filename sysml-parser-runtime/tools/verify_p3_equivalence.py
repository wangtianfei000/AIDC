#!/usr/bin/env python3
"""Run the P3 black-box and semantic-equivalence gates for both parser engines."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

from compare_runtime_behavior import first_differences, normalize


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


class Runtime:
    def __init__(self, executable: Path, extra_arguments: list[str] | None = None):
        environment = os.environ.copy()
        environment["PATH"] = str(executable.parent) + os.pathsep + environment.get("PATH", "")
        self.process = subprocess.Popen(
            [str(executable.resolve()), "--port", "0", *(extra_arguments or [])],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=environment,
        )
        assert self.process.stdout
        startup_line = self.process.stdout.readline()
        if not startup_line:
            stderr = self.process.stderr.read() if self.process.stderr else ""
            raise RuntimeError(f"Runtime failed to start: {stderr}")
        startup = json.loads(startup_line)
        self.base_url = f"http://127.0.0.1:{startup['port']}"

    def close(self) -> None:
        self.process.terminate()
        try:
            self.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.kill()


def capture(base_url: str, aidc_root: Path) -> dict[str, dict]:
    fixtures = {
        "architecture": aidc_root / "samples/jingang-10axis-system-design/architecture.sysml",
        "vehicle-mode": aidc_root / "samples/VehicleMode.sysml",
        "analysis-exception": aidc_root / "samples/analysis-exception-acceptance.sysml",
    }
    captured = {
        "health.json": request_json(base_url, "/health"),
        "capabilities.json": request_json(base_url, "/capabilities"),
    }
    for name, fixture in fixtures.items():
        source = fixture.read_text(encoding="utf-8")
        body = {"uri": f"p3:///{fixture.name}", "text": source}
        captured[f"{name}.model.json"] = request_json(base_url, "/parse", body)
        if name == "architecture":
            captured[f"{name}.analysis.json"] = request_json(base_url, "/analyze", body)
    captured["invalid-syntax.model.json"] = request_json(
        base_url,
        "/parse",
        {"uri": "p3:///invalid.sysml", "text": "package Broken { this is invalid @@@ }"},
    )
    return captured


def run_http_smoke(base_url: str, runtime_root: Path, aidc_root: Path, output: Path) -> dict:
    command = [
        sys.executable,
        "-X",
        "utf8",
        str(runtime_root / "tests/http_smoke.py"),
        "--base-url",
        base_url,
        "--schema",
        str(runtime_root / "contracts/model-snapshot.schema.json"),
        "--fixture",
        str(aidc_root / "samples/VehicleMode.sysml"),
        "--fixture",
        str(aidc_root / "samples/jingang-10axis-system-design/architecture.sysml"),
        "--analysis-schema",
        str(runtime_root / "contracts/analysis-snapshot.schema.json"),
        "--analysis-fixture",
        str(aidc_root / "samples/jingang-10axis-system-design/architecture.sysml"),
        "--output",
        str(output),
    ]
    completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=180)
    if completed.returncode:
        raise RuntimeError(completed.stdout + completed.stderr)
    return json.loads(output.read_text(encoding="utf-8"))


def verify_timeout(executable: Path) -> str:
    runtime = Runtime(executable, ["--parse-timeout-ms", "1"])
    try:
        source = "package TimeoutModel {\n" + "\n".join(
            f"part def TimeoutPart{index};" for index in range(1500)
        ) + "\n}"
        try:
            request_json(
                runtime.base_url,
                "/parse",
                {"uri": "p3:///timeout.sysml", "text": source, "requestId": "timeout-check"},
            )
        except urllib.error.HTTPError as error:
            # Parser work uses a cooperative soft timeout and shares the 499
            # cancellation response; HTTP framing timeout uses 408 separately.
            if error.code == 499:
                return "pass"
            raise
        raise AssertionError("Runtime completed the timeout fixture instead of returning its soft-timeout response")
    finally:
        runtime.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--external-runtime", required=True, type=Path)
    parser.add_argument("--embedded-runtime", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    runtime_root = Path(__file__).resolve().parents[1]
    aidc_root = runtime_root.parent

    with tempfile.TemporaryDirectory(prefix="aidc-p3-") as temporary:
        temp = Path(temporary)
        results = {}
        captures = {}
        for engine, executable in (("external", args.external_runtime), ("embedded", args.embedded_runtime)):
            runtime = Runtime(executable)
            try:
                captures[engine] = capture(runtime.base_url, aidc_root)
                results[engine] = run_http_smoke(runtime.base_url, runtime_root, aidc_root, temp / f"{engine}-http.json")
            finally:
                runtime.close()
            results[engine]["timeout"] = verify_timeout(executable)

    cases = []
    for name in sorted(captures["external"]):
        external = normalize(name, captures["external"][name])
        embedded = normalize(name, captures["embedded"][name])
        differences = first_differences(external, embedded)
        cases.append({"name": name, "status": "passed" if not differences else "failed", "differences": differences})
    report = {
        "schemaVersion": "aidc.sysml-parser-p3-validation/1.0",
        "status": "passed" if all(case["status"] == "passed" for case in cases) else "failed",
        "engines": {
            name: {
                "parserMode": captures[name]["health.json"]["parserMode"],
                "parserBaseline": captures[name]["health.json"]["parserBaseline"],
                "standardLibrary": {
                    key: value
                    for key, value in captures[name]["health.json"]["standardLibrary"].items()
                    if key != "rootPath"
                },
                "httpSmoke": summary,
            }
            for name, summary in results.items()
        },
        "equivalence": {
            "normalization": [
                "implementation identity",
                "standard-library deployment path",
                "Transition.isInitial derived from source/target states",
            ],
            "cases": cases,
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
