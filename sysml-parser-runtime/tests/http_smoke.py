#!/usr/bin/env python3
"""Black-box checks for a running AIDC parser runtime."""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import pathlib
import socket
import time
import urllib.parse
import urllib.error
import urllib.request

import jsonschema


def request_json(base_url: str, path: str, method: str = "GET", body=None, origin=None):
    data = None if body is None else json.dumps(body).encode("utf-8")
    headers = {"Content-Type": "application/json"}
    if origin:
        headers["Origin"] = origin
    request = urllib.request.Request(base_url + path, data=data, headers=headers, method=method)
    with urllib.request.urlopen(request, timeout=20) as response:
        return response.status, dict(response.headers), json.load(response)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-url", required=True)
    parser.add_argument("--schema", required=True, type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--fixture", action="append", default=[], type=pathlib.Path)
    parser.add_argument("--analysis-schema", type=pathlib.Path)
    parser.add_argument("--analysis-fixture", type=pathlib.Path)
    args = parser.parse_args()

    schema = json.loads(args.schema.read_text(encoding="utf-8"))
    status, _, health = request_json(args.base_url, "/health")
    assert status == 200 and health["status"] == "ok"
    status, _, capabilities = request_json(args.base_url, "/capabilities")
    assert status == 200 and capabilities["schemaVersion"] == "0.1"

    model = """package Robot {
    requirement def SafeStop;
    requirement stop : SafeStop;
}"""
    snapshots = []
    for index in range(100):
        status, headers, snapshot = request_json(
            args.base_url,
            "/parse",
            "POST",
            {"uri": "memory://robot.sysml", "text": model, "requestId": f"repeat-{index}"},
            "http://127.0.0.1:5173",
        )
        assert status == 200
        assert headers.get("X-Request-Id") == f"repeat-{index}"
        jsonschema.Draft202012Validator(schema).validate(snapshot)
        snapshots.append(snapshot)
    canonical = json.dumps(snapshots[0], sort_keys=True, ensure_ascii=False)
    assert all(json.dumps(item, sort_keys=True, ensure_ascii=False) == canonical for item in snapshots)

    def parallel_parse(index: int):
        return request_json(
            args.base_url,
            "/parse",
            "POST",
            {
                "uri": f"memory://parallel-{index}.sysml",
                "text": f"package Parallel{index} {{ part def Robot{index}; }}",
                "requestId": f"parallel-{index}",
            },
        )[2]

    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
        parallel = list(executor.map(parallel_parse, range(2)))
    assert parallel[0]["document"]["uri"] != parallel[1]["document"]["uri"]

    _, _, invalid = request_json(
        args.base_url,
        "/parse",
        "POST",
        {"uri": "memory://invalid.sysml", "text": "package Broken { this is invalid @@@ }"},
    )
    assert any(item["severity"] == "error" and item["source"] for item in invalid["diagnostics"])

    status, _, canceled = request_json(
        args.base_url, "/cancel", "POST", {"requestId": "not-active"}
    )
    assert status == 200 and canceled["accepted"] is False

    large_model = "package CancelModel {\n" + "\n".join(
        f"part def Part{i};" for i in range(1500)
    ) + "\n}"
    cancellation_result = {}

    def slow_parse():
        try:
            request_json(
                args.base_url,
                "/parse",
                "POST",
                {"uri": "memory://cancel.sysml", "text": large_model, "requestId": "cancel-active"},
            )
            cancellation_result["status"] = 200
        except urllib.error.HTTPError as error:
            cancellation_result["status"] = error.code

    with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
        future = executor.submit(slow_parse)
        accepted = False
        for _ in range(20):
            time.sleep(0.01)
            cancel_status, _, cancel_body = request_json(
                args.base_url, "/cancel", "POST", {"requestId": "cancel-active"}
            )
            if cancel_body["accepted"]:
                accepted = cancel_status == 202
                break
        future.result(timeout=20)
    assert accepted and cancellation_result.get("status") == 499

    fixture_results = []
    for fixture in args.fixture:
        fixture_text = fixture.read_text(encoding="utf-8")
        _, _, fixture_snapshot = request_json(
            args.base_url,
            "/parse",
            "POST",
            {"uri": fixture.resolve().as_uri(), "text": fixture_text},
        )
        jsonschema.Draft202012Validator(schema).validate(fixture_snapshot)
        fixture_results.append({
            "file": fixture.name,
            "elements": len(fixture_snapshot["elements"]),
            "relationships": len(fixture_snapshot["relationships"]),
            "relationshipsWithBothEnds": sum(
                bool(item["sourceIds"]) and bool(item["targetIds"])
                for item in fixture_snapshot["relationships"]
            ),
            "errors": sum(item["severity"] == "error" for item in fixture_snapshot["diagnostics"]),
            "warnings": sum(item["severity"] == "warning" for item in fixture_snapshot["diagnostics"]),
        })

    analysis_summary = None
    if args.analysis_schema or args.analysis_fixture:
        assert args.analysis_schema and args.analysis_fixture
        analysis_schema = json.loads(args.analysis_schema.read_text(encoding="utf-8"))
        analysis_text = args.analysis_fixture.read_text(encoding="utf-8")
        analysis_snapshots = []
        for index in range(2):
            analysis_status, analysis_headers, analysis_snapshot = request_json(
                args.base_url,
                "/analyze",
                "POST",
                {
                    "uri": args.analysis_fixture.resolve().as_uri(),
                    "text": analysis_text,
                    "requestId": f"analysis-{index}",
                },
                "http://127.0.0.1:5173",
            )
            assert analysis_status == 200
            assert analysis_headers.get("X-Request-Id") == f"analysis-{index}"
            jsonschema.Draft202012Validator(analysis_schema).validate(analysis_snapshot)
            analysis_snapshots.append(analysis_snapshot)
        assert json.dumps(analysis_snapshots[0], sort_keys=True, ensure_ascii=False) == json.dumps(
            analysis_snapshots[1], sort_keys=True, ensure_ascii=False
        )
        first_analysis = analysis_snapshots[0]["analyses"][0]
        changed_text = analysis_text.replace(
            "attribute usableEnergyFraction : Real = 0.8;",
            "attribute usableEnergyFraction : Real = 0.72;",
        )
        assert changed_text != analysis_text
        _, _, changed_snapshot = request_json(
            args.base_url,
            "/analyze",
            "POST",
            {"uri": "memory://changed-input.sysml", "text": changed_text},
        )
        changed_results = {
            item["name"]: item for item in changed_snapshot["analyses"][0]["results"]
        }
        assert abs(changed_results["enduranceResult"]["value"] - 1.728) < 1e-9
        analysis_summary = {
            "schemaValidation": "pass",
            "deterministicAnalyses": 2,
            "results": len(first_analysis["results"]),
            "constraints": len(first_analysis["constraints"]),
            "calculationDiagnostics": len(analysis_snapshots[0]["diagnostics"]),
            "inputChangePropagation": "pass",
        }

    rejected_origin = False
    try:
        request_json(args.base_url, "/health", origin="https://example.com")
    except urllib.error.HTTPError as error:
        rejected_origin = error.code == 403
    assert rejected_origin

    parsed_url = urllib.parse.urlparse(args.base_url)
    with socket.create_connection((parsed_url.hostname, parsed_url.port), timeout=5) as raw_socket:
        raw_socket.sendall(
            b"POST /parse HTTP/1.1\r\nHost: localhost\r\n"
            b"Content-Type: application/json\r\nContent-Length: 8388609\r\n\r\n"
        )
        oversized_response = raw_socket.recv(4096)
    assert b" 413 " in oversized_response

    summary = {
        "health": "pass",
        "capabilities": "pass",
        "schemaValidation": "pass",
        "deterministicParses": 100,
        "parallelParses": 2,
        "structuredInvalidDiagnostic": "pass",
        "inactiveCancel": "pass",
        "activeCancel": "pass",
        "nonLocalOriginRejected": "pass",
        "oversizedRequestRejected": "pass",
        "elementCount": len(snapshots[0]["elements"]),
        "relationshipCount": len(snapshots[0]["relationships"]),
        "fixtures": fixture_results,
        "analysis": analysis_summary,
    }
    rendered = json.dumps(summary, ensure_ascii=False, indent=2)
    print(rendered)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
