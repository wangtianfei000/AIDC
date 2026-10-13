from __future__ import annotations

import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
import sys
import threading
import time
import uuid
from typing import Any

from . import PARSER_BASELINE, RUNTIME_VERSION, SCHEMA_VERSION
from .analysis import analyze
from .model import CAPABILITIES, make_snapshot


ACTIVE: dict[str, threading.Event] = {}
ACTIVE_LOCK = threading.Lock()


def health() -> dict[str, Any]:
    return {"status": "ok", "runtime": "aidc-sysml-parser-runtime", "runtimeVersion": RUNTIME_VERSION, "parserMode": "stdlib", "parserBaseline": PARSER_BASELINE, "qtDependency": False}


def capabilities() -> dict[str, Any]:
    return {"runtimeVersion": RUNTIME_VERSION, "schemaVersion": SCHEMA_VERSION, "parserMode": "stdlib", "parserBaseline": PARSER_BASELINE, "capabilities": CAPABILITIES}


class RuntimeServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True
    def __init__(self, address, maximum_request_bytes: int, parse_timeout_ms: int):
        super().__init__(address, Handler)
        self.maximum_request_bytes = maximum_request_bytes
        self.parse_timeout_ms = parse_timeout_ms

    def handle_error(self, _request, _client_address):
        # Browser disconnects during shutdown and cancellation are expected.
        pass


class Handler(BaseHTTPRequestHandler):
    server_version = "AIDC-SysML/1.0"
    protocol_version = "HTTP/1.1"

    def log_message(self, *_args): pass

    def allowed_origin(self) -> str | None:
        origin = self.headers.get("Origin")
        if not origin or origin == "null": return origin
        for prefix in ("http://localhost", "http://127.0.0.1", "https://localhost", "https://127.0.0.1"):
            if origin == prefix or origin.startswith(prefix + ":"): return origin
        return None

    def send_json(self, status: int, value: dict[str, Any], origin: str | None = None, request_id: str | None = None):
        body = json.dumps(value, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        self.send_response(status); self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body))); self.send_header("Cache-Control", "no-store")
        if request_id: self.send_header("X-Request-Id", request_id)
        if origin:
            self.send_header("Access-Control-Allow-Origin", origin); self.send_header("Vary", "Origin")
            self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); self.send_header("Access-Control-Allow-Headers", "Content-Type")
            self.send_header("Access-Control-Expose-Headers", "X-Request-Id")
        self.end_headers(); self.wfile.write(body)

    def error(self, status: int, code: str, message: str, origin: str | None = None):
        self.send_json(status, {"error": {"code": code, "message": message}}, origin)

    def authorize(self) -> tuple[bool, str | None]:
        raw = self.headers.get("Origin")
        origin = self.allowed_origin()
        if raw and raw != "null" and origin is None:
            self.error(403, "AIDC_ORIGIN_REJECTED", "Only localhost web origins are allowed")
            return False, None
        return True, origin

    def do_OPTIONS(self):
        ok, origin = self.authorize()
        if ok: self.send_json(200, {"status": "ok"}, origin)

    def do_GET(self):
        ok, origin = self.authorize()
        if not ok: return
        if self.path.split("?", 1)[0] == "/health": self.send_json(200, health(), origin)
        elif self.path.split("?", 1)[0] == "/capabilities": self.send_json(200, capabilities(), origin)
        else: self.error(404, "AIDC_NOT_FOUND", "Unknown runtime endpoint", origin)

    def do_POST(self):
        ok, origin = self.authorize()
        if not ok: return
        path = self.path.split("?", 1)[0]
        if path not in {"/parse", "/analyze", "/cancel"}:
            self.error(404, "AIDC_NOT_FOUND", "Unknown runtime endpoint", origin); return
        try: length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self.error(400, "AIDC_BAD_HTTP_REQUEST", "Invalid Content-Length", origin); return
        if length > self.server.maximum_request_bytes:
            self.error(413, "AIDC_REQUEST_TOO_LARGE", "Request exceeds configured size limit", origin); return
        try: body = json.loads(self.rfile.read(length) or b"{}")
        except (json.JSONDecodeError, UnicodeDecodeError) as error:
            self.error(400, "AIDC_INVALID_JSON", str(error), origin); return
        if path == "/cancel":
            request_id = body.get("requestId", "")
            with ACTIVE_LOCK: event = ACTIVE.get(request_id)
            if event: event.set()
            self.send_json(202 if event else 200, {"requestId": request_id, "accepted": bool(event)}, origin); return
        uri, text = body.get("uri"), body.get("text")
        if not isinstance(uri, str) or not isinstance(text, str):
            self.error(400, "AIDC_INVALID_PARSE_REQUEST", "uri and text strings are required", origin); return
        request_id = body.get("requestId") or str(uuid.uuid4()); canceled = threading.Event()
        with ACTIVE_LOCK:
            if request_id in ACTIVE:
                self.error(409, "AIDC_DUPLICATE_REQUEST_ID", "requestId is already active", origin); return
            ACTIVE[request_id] = canceled
        timer = threading.Timer(self.server.parse_timeout_ms / 1000, canceled.set); timer.start()
        try:
            # Long documents yield briefly so a concurrent /cancel request can be observed.
            if len(text) > 25000:
                for _ in range(10):
                    if canceled.wait(0.003): break
            if canceled.is_set():
                self.error(499, "AIDC_PARSE_CANCELED", "Request was canceled or exceeded the soft timeout", origin); return
            result = analyze(uri, text) if path == "/analyze" else make_snapshot(uri, text)[0]
            if canceled.is_set(): self.error(499, "AIDC_PARSE_CANCELED", "Request was canceled or exceeded the soft timeout", origin)
            else: self.send_json(200, result, origin, request_id)
        except Exception as error:
            self.error(500, "AIDC_RUNTIME_ERROR", str(error), origin)
        finally:
            timer.cancel()
            with ACTIVE_LOCK: ACTIVE.pop(request_id, None)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="AIDC Qt-free read-only SysML v2 parser runtime")
    parser.add_argument("--version", action="version", version=RUNTIME_VERSION)
    parser.add_argument("--port", type=int, default=47321)
    parser.add_argument("--max-request-bytes", type=int, default=8 * 1024 * 1024)
    parser.add_argument("--parse-timeout-ms", type=int, default=30000)
    parser.add_argument("--standard-library")
    args = parser.parse_args(argv)
    if not 0 <= args.port <= 65535 or args.max_request_bytes < 1024 or args.parse_timeout_ms < 1:
        print("Invalid command-line option value", file=sys.stderr); return 2
    if args.standard_library: os.environ["AIDC_SYSML_LIBRARY_ROOT"] = args.standard_library
    server = RuntimeServer(("127.0.0.1", args.port), args.max_request_bytes, args.parse_timeout_ms)
    print(json.dumps({"status": "listening", "address": "127.0.0.1", "port": server.server_address[1]}), flush=True)
    try: server.serve_forever()
    except KeyboardInterrupt: pass
    finally: server.server_close()
    return 0


if __name__ == "__main__": raise SystemExit(main())

