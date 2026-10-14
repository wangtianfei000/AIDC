from __future__ import annotations

import argparse
import http.client
import json
import mimetypes
import os
import re
import subprocess
import threading
import time
from http import HTTPStatus
from http.cookies import SimpleCookie
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlsplit


STATIC_CONTENT_TYPES = {
    ".css": "text/css; charset=utf-8",
    ".html": "text/html; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".mjs": "text/javascript; charset=utf-8",
    ".svg": "image/svg+xml",
    ".wasm": "application/wasm",
}

SESSION_CLIENT_SCRIPT = """<script data-aidc-session-client>
(() => {
  const id = globalThis.crypto?.randomUUID?.() || `${Date.now()}-${Math.random()}`;
  const send = (action, beacon = false) => {
    const body = JSON.stringify({clientId: id});
    const url = `/session/client/${action}`;
    if (beacon && navigator.sendBeacon) {
      navigator.sendBeacon(url, new Blob([body], {type: "application/json"}));
      return;
    }
    fetch(url, {
      method: "POST",
      credentials: "same-origin",
      headers: {"Content-Type": "application/json"},
      body,
      keepalive: true,
    }).catch(() => {});
  };
  send("register");
  const timer = setInterval(() => send("heartbeat"), 5000);
  addEventListener("pagehide", () => {
    clearInterval(timer);
    send("unregister", true);
  }, {once: true});
})();
</script>"""


class GatewayHandler(BaseHTTPRequestHandler):
    server: "GatewayServer"

    def _authorized(self) -> bool:
        parsed = urlsplit(self.path)
        supplied = parse_qs(parsed.query).get("token", [None])[0]
        cookie = SimpleCookie(self.headers.get("Cookie", ""))
        stored = cookie.get("AIDC_SESSION")
        if supplied == self.server.token:
            self._set_cookie = True
            return True
        return bool(stored and stored.value == self.server.token)

    def _reject(self) -> None:
        payload = json.dumps({"error": {"code": "SESSION_TOKEN_REQUIRED", "message": "需要有效的 AIDC 会话令牌"}}).encode()
        self.send_response(HTTPStatus.UNAUTHORIZED)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers(); self.wfile.write(payload)

    def _headers(self, status: int, headers: list[tuple[str, str]], length: int | None = None) -> None:
        self.send_response(status)
        for key, value in headers:
            if key.lower() not in {"connection", "transfer-encoding", "set-cookie", "content-length"}:
                self.send_header(key, value)
        if getattr(self, "_set_cookie", False):
            self.send_header("Set-Cookie", "AIDC_SESSION=" + self.server.token + "; Path=/; HttpOnly; SameSite=Strict")
        if length is not None: self.send_header("Content-Length", str(length))
        self.end_headers()

    def _proxy(self, target: str, strip_prefix: str = "") -> None:
        target_url = urlsplit(target)
        request_url = urlsplit(self.path)
        path = request_url.path[len(strip_prefix):] if strip_prefix else request_url.path
        if not path.startswith("/"): path = "/" + path
        if request_url.query: path += "?" + request_url.query
        body = self.rfile.read(int(self.headers.get("Content-Length", "0")))
        connection = http.client.HTTPConnection(target_url.hostname, target_url.port, timeout=60)
        headers = {key: value for key, value in self.headers.items() if key.lower() not in {"host", "cookie", "content-length", "connection", "accept-encoding"}}
        connection.request(self.command, path, body=body or None, headers=headers)
        response = connection.getresponse(); payload = response.read()
        payload = self.server.instrument_html(payload, response.getheader("Content-Type"))
        self._headers(response.status, response.getheaders(), len(payload)); self.wfile.write(payload)
        connection.close()

    def _static(self) -> None:
        parsed = urlsplit(self.path)
        relative = parsed.path.lstrip("/") or "index.html"
        roots = [root for root in (self.server.overlay_root, self.server.static_root) if root]
        candidate = next(((root / relative).resolve() for root in roots if (root / relative).resolve().is_file()), (self.server.static_root / relative).resolve())
        if not any(root == candidate or root in candidate.parents for root in roots):
            self.send_error(HTTPStatus.FORBIDDEN); return
        if not candidate.is_file():
            # Only extensionless client-side routes may fall back to the SPA shell.
            # Missing data/assets must remain 404; otherwise clients try to parse
            # index.html as JSON, JavaScript, or a model file.
            if Path(relative).suffix or parsed.path.startswith(("/api/", "/session/")):
                self.send_error(HTTPStatus.NOT_FOUND); return
            candidate = self.server.static_root / "index.html"
        if not candidate.is_file(): self.send_error(HTTPStatus.NOT_FOUND); return
        payload = candidate.read_bytes()
        # Windows' MIME registry commonly maps .js to text/plain. ES modules are
        # rejected by Chromium when served with that type, leaving the shell page
        # completely blank. Keep browser-critical types deterministic on every host.
        content_type = STATIC_CONTENT_TYPES.get(candidate.suffix.lower()) or mimetypes.guess_type(candidate.name)[0] or "application/octet-stream"
        payload = self.server.instrument_html(payload, content_type)
        self._headers(HTTPStatus.OK, [("Content-Type", content_type), ("Cache-Control", "no-store")], len(payload)); self.wfile.write(payload)

    def _session_client(self, path: str) -> bool:
        if not path.startswith("/session/client/"): return False
        if self.command != "POST": self.send_error(HTTPStatus.METHOD_NOT_ALLOWED); return True
        try:
            size = int(self.headers.get("Content-Length", "0"))
            document = json.loads(self.rfile.read(size) or b"{}")
            client_id = str(document.get("clientId", "")).strip()
        except (ValueError, json.JSONDecodeError):
            client_id = ""
        if not client_id or len(client_id) > 128:
            self.send_error(HTTPStatus.BAD_REQUEST); return True
        action = path.rsplit("/", 1)[-1]
        if action == "register": self.server.register_client(client_id)
        elif action == "heartbeat": self.server.heartbeat_client(client_id)
        elif action == "unregister": self.server.unregister_client(client_id)
        else: self.send_error(HTTPStatus.NOT_FOUND); return True
        payload = json.dumps({"status": "ok", "clients": self.server.client_count()}).encode()
        self._headers(HTTPStatus.OK, [("Content-Type", "application/json")], len(payload)); self.wfile.write(payload); return True

    def _model3d_api(self, path: str) -> bool:
        match = re.fullmatch(r"/api/model-projects/([^/]+)/pose-presets", path)
        if not match or not self.server.project_root: return False
        manifest_path = self.server.project_root / "generated" / "model3d.manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if match.group(1) != manifest.get("modelId"):
            self.send_error(HTTPStatus.NOT_FOUND); return True
        presets = self.server.project_root / "pose-presets.json"
        generated = self.server.project_root / "generated" / "pose-presets.json"
        if self.command == "GET":
            source = presets if presets.is_file() else generated
            if not source.is_file(): self.send_error(HTTPStatus.NOT_FOUND); return True
            payload = source.read_bytes()
        elif self.command == "PUT":
            payload = self.rfile.read(int(self.headers.get("Content-Length", "0")))
            value = json.loads(payload.decode("utf-8")); presets.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")
            generated.parent.mkdir(parents=True, exist_ok=True); generated.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")
            payload = json.dumps(value, ensure_ascii=False).encode("utf-8")
        else: self.send_error(HTTPStatus.METHOD_NOT_ALLOWED); return True
        self._headers(HTTPStatus.OK, [("Content-Type", "application/json; charset=utf-8")], len(payload)); self.wfile.write(payload); return True

    def _dispatch(self) -> None:
        if not self._authorized(): self._reject(); return
        path = urlsplit(self.path).path
        if path == "/health":
            payload = json.dumps({"status": "ok", "clients": self.server.client_count()}).encode(); self._headers(200, [("Content-Type", "application/json")], len(payload)); self.wfile.write(payload); return
        if self._session_client(path): return
        if path == "/session/input" and self.server.input_file:
            payload = json.dumps({"name": self.server.input_file.name, "uri": self.server.input_file.as_uri(), "text": self.server.input_file.read_text(encoding="utf-8")}, ensure_ascii=False).encode("utf-8")
            self._headers(200, [("Content-Type", "application/json; charset=utf-8")], len(payload)); self.wfile.write(payload); return
        if self._model3d_api(path): return
        for prefix, target in self.server.proxies:
            if path.startswith(prefix): self._proxy(target, prefix); return
        if self.server.default_proxy: self._proxy(self.server.default_proxy); return
        self._static()

    do_GET = _dispatch
    do_POST = _dispatch
    do_PUT = _dispatch
    do_DELETE = _dispatch
    def log_message(self, format: str, *args: object) -> None:  # noqa: A002
        return


class GatewayServer(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, address: tuple[str, int], token: str, static_root: Path | None, default_proxy: str | None, proxies: list[tuple[str, str]], input_file: Path | None, project_root: Path | None = None, managed_pids: list[int] | None = None, disconnect_grace: float = 4.0, heartbeat_timeout: float = 90.0, registration_timeout: float = 30.0):
        self.token = token; self.static_root = static_root or Path.cwd(); self.default_proxy = default_proxy; self.proxies = proxies; self.input_file = input_file
        self.project_root = project_root
        self.overlay_root = project_root / "generated" if project_root else None
        self.managed_pids = list(managed_pids or [])
        self.disconnect_grace = disconnect_grace; self.heartbeat_timeout = heartbeat_timeout; self.registration_timeout = registration_timeout
        self._clients: dict[str, float] = {}; self._client_lock = threading.Lock(); self._ever_registered = False
        self._shell_served_at: float | None = None; self._disconnect_at: float | None = None; self._shutdown_requested = False
        super().__init__(address, GatewayHandler)
        self._watchdog = threading.Thread(target=self._watch_clients, name="aidc-session-watchdog", daemon=True)
        self._watchdog.start()

    def instrument_html(self, payload: bytes, content_type: str | None) -> bytes:
        if not content_type or "text/html" not in content_type.lower(): return payload
        try: page = payload.decode("utf-8")
        except UnicodeDecodeError: return payload
        if "data-aidc-session-client" in page: return payload
        with self._client_lock:
            if self._shell_served_at is None: self._shell_served_at = time.monotonic()
        marker = page.lower().rfind("</body>")
        page = page[:marker] + SESSION_CLIENT_SCRIPT + page[marker:] if marker >= 0 else page + SESSION_CLIENT_SCRIPT
        return page.encode("utf-8")

    def register_client(self, client_id: str) -> None:
        with self._client_lock:
            self._clients[client_id] = time.monotonic(); self._ever_registered = True; self._disconnect_at = None

    def heartbeat_client(self, client_id: str) -> None:
        with self._client_lock:
            if client_id in self._clients: self._clients[client_id] = time.monotonic()

    def unregister_client(self, client_id: str) -> None:
        with self._client_lock:
            self._clients.pop(client_id, None)
            if self._ever_registered and not self._clients: self._disconnect_at = time.monotonic()

    def client_count(self) -> int:
        with self._client_lock: return len(self._clients)

    def _watch_clients(self) -> None:
        while not self._shutdown_requested:
            time.sleep(0.5); now = time.monotonic(); should_stop = False
            with self._client_lock:
                expired = [client_id for client_id, seen in self._clients.items() if now - seen > self.heartbeat_timeout]
                for client_id in expired: self._clients.pop(client_id, None)
                if expired and self._ever_registered and not self._clients and self._disconnect_at is None: self._disconnect_at = now
                if self._disconnect_at is not None and now - self._disconnect_at >= self.disconnect_grace: should_stop = True
                if not self._ever_registered and self._shell_served_at is not None and now - self._shell_served_at >= self.registration_timeout: should_stop = True
            if should_stop:
                self.request_shutdown(); return

    def request_shutdown(self) -> None:
        if self._shutdown_requested: return
        self._shutdown_requested = True
        self.stop_managed_processes()
        threading.Thread(target=self.shutdown, name="aidc-gateway-shutdown", daemon=True).start()

    def stop_managed_processes(self) -> None:
        pids, self.managed_pids = self.managed_pids, []
        for pid in pids:
            try:
                if os.name == "nt": subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True, timeout=15, check=False)
                else: os.kill(pid, 15)
            except (OSError, subprocess.SubprocessError):
                pass


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True); parser.add_argument("--token", default=os.environ.get("AIDC_SESSION_TOKEN"))
    parser.add_argument("--static", type=Path); parser.add_argument("--proxy"); parser.add_argument("--map", action="append", default=[]); parser.add_argument("--input", type=Path); parser.add_argument("--project", type=Path); parser.add_argument("--managed-pid", type=int, action="append", default=[])
    args = parser.parse_args()
    mappings = []
    for value in args.map:
        prefix, target = value.split("=", 1); mappings.append((prefix.rstrip("/"), target.rstrip("/")))
    if not args.token: parser.error("会话令牌缺失")
    server = GatewayServer(("127.0.0.1", args.port), args.token, args.static.resolve() if args.static else None, args.proxy, mappings, args.input.resolve() if args.input else None, args.project.resolve() if args.project else None, args.managed_pid)
    try: server.serve_forever()
    finally:
        server.stop_managed_processes(); server.server_close()
    return 0


if __name__ == "__main__": raise SystemExit(main())
