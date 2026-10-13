from __future__ import annotations

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from .paths import state_root
from .result import AidcError


CODEX_BEGIN = "# BEGIN AIDC MANAGED MCP"
CODEX_END = "# END AIDC MANAGED MCP"
SERVER_NAME = "aidc"


def _utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def _json_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def _tree_digest(path: Path) -> str:
    digest = hashlib.sha256()
    if not path.exists():
        return "missing"
    for item in sorted(candidate for candidate in path.rglob("*") if candidate.is_file()):
        if "__pycache__" in item.parts or item.suffix == ".pyc":
            continue
        digest.update(item.relative_to(path).as_posix().encode("utf-8"))
        digest.update(item.read_bytes())
    return digest.hexdigest()


class IntegrationManager:
    """Install the host-specific discovery layer without branching core behavior."""

    def __init__(self, product: Path, home: Path | None = None, state: Path | None = None):
        self.product = product.resolve()
        override_home = os.environ.get("AIDC_INTEGRATION_HOME")
        self.home = (home or (Path(override_home) if override_home else Path.home())).expanduser().resolve()
        self.state = (state or state_root() / "integrations").resolve()
        self.state.mkdir(parents=True, exist_ok=True)

    def server_spec(self) -> dict[str, Any]:
        executable = self.product / "bin" / "aidc-mcp.exe"
        if executable.is_file():
            return {"command": str(executable.resolve()), "args": [], "env": {"PYTHONUTF8": "1"}}
        source = self.product / "aidc_mcp.py"
        if source.is_file():
            return {"command": str(Path(sys.executable).resolve()), "args": [str(source.resolve())], "env": {"PYTHONUTF8": "1"}}
        raise AidcError("AIDC MCP 服务入口不存在", 4, "INTEGRATION_MCP_MISSING", str(executable))

    def _paths(self, host: str, config: Path | None) -> tuple[Path, Path | None]:
        if host == "codex":
            codex_home = Path(os.environ.get("CODEX_HOME", self.home / ".codex"))
            return (config or codex_home / "config.toml").expanduser().resolve(), codex_home / "skills"
        if host == "cursor":
            return (config or self.home / ".cursor" / "mcp.json").expanduser().resolve(), self.home / ".cursor" / "skills"
        return (config or self.state / "mcp" / "aidc-mcp.json").expanduser().resolve(), None

    def _state_path(self, host: str) -> Path:
        return self.state / host / "state.json"

    def _load_state(self, host: str) -> dict[str, Any] | None:
        path = self._state_path(host)
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except FileNotFoundError:
            return None
        except json.JSONDecodeError as exc:
            raise AidcError(f"接入状态损坏：{exc}", 6, "INTEGRATION_STATE_INVALID", str(path)) from exc

    def _save_state(self, host: str, document: dict[str, Any]) -> None:
        path = self._state_path(host); path.parent.mkdir(parents=True, exist_ok=True)
        temporary = path.with_suffix(".tmp")
        temporary.write_text(json.dumps(document, ensure_ascii=False, indent=2), encoding="utf-8")
        os.replace(temporary, path)

    @staticmethod
    def _copy(source: Path, destination: Path) -> None:
        if source.is_dir():
            shutil.copytree(source, destination, dirs_exist_ok=True, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
        else:
            destination.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(source, destination)

    def _create_baseline(self, host: str, target: Path, skills_target: Path | None) -> dict[str, Any]:
        backup = self.state / host / "backups" / _utc_stamp()
        suffix = 1
        while backup.exists():
            backup = backup.with_name(f"{backup.name}-{suffix}"); suffix += 1
        backup.mkdir(parents=True)
        baseline: dict[str, Any] = {"directory": str(backup), "configExisted": target.is_file(), "skills": {}}
        if target.is_file(): self._copy(target, backup / "config.original")
        if skills_target:
            catalog = self._skill_catalog()
            for item in catalog:
                current = skills_target / item["id"]
                existed = current.is_dir(); baseline["skills"][item["id"]] = {"existed": existed}
                if existed: self._copy(current, backup / "skills" / item["id"])
        return baseline

    def _skill_catalog(self) -> list[dict[str, Any]]:
        path = self.product / "skills" / "catalog.json"
        try:
            return json.loads(path.read_text(encoding="utf-8"))["skills"]
        except (OSError, KeyError, json.JSONDecodeError) as exc:
            raise AidcError(f"Skills 目录不可用：{exc}", 4, "INTEGRATION_SKILLS_MISSING", str(path)) from exc

    def _install_skills(self, host: str, target: Path | None) -> list[dict[str, Any]]:
        if not target: return []
        target.mkdir(parents=True, exist_ok=True); installed = []
        for item in self._skill_catalog():
            if host not in item.get("hosts", []): continue
            source = (self.product / "skills" / item["path"]).resolve().parent
            destination = target / item["id"]
            if destination.exists(): shutil.rmtree(destination)
            self._copy(source, destination)
            installed.append({"id": item["id"], "version": item["version"], "path": str(destination), "sha256": _tree_digest(destination)})
        return installed

    @staticmethod
    def _remove_codex_block(content: str) -> str:
        pattern = re.compile(rf"(?ms)^\s*{re.escape(CODEX_BEGIN)}.*?{re.escape(CODEX_END)}\s*(?:\r?\n)?")
        return pattern.sub("", content).rstrip() + ("\n" if content.strip() else "")

    @staticmethod
    def _codex_has_unmanaged_aidc(content: str) -> bool:
        outside = IntegrationManager._remove_codex_block(content)
        return re.search(r"(?m)^\s*\[mcp_servers\.aidc\]\s*$", outside) is not None

    def _write_codex(self, target: Path, spec: dict[str, Any]) -> None:
        current = target.read_text(encoding="utf-8") if target.is_file() else ""
        if self._codex_has_unmanaged_aidc(current):
            raise AidcError("Codex 已存在非 AIDC 管理的 aidc MCP 配置", 7, "INTEGRATION_CONFLICT", str(target))
        current = self._remove_codex_block(current)
        args = ", ".join(_json_string(value) for value in spec["args"])
        block = (
            f"{CODEX_BEGIN}\n[mcp_servers.aidc]\n"
            f"command = {_json_string(spec['command'])}\nargs = [{args}]\nstartup_timeout_sec = 120\n\n"
            f"[mcp_servers.aidc.env]\nPYTHONUTF8 = \"1\"\n{CODEX_END}\n"
        )
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(current + ("\n" if current else "") + block, encoding="utf-8")

    @staticmethod
    def _read_json_config(target: Path) -> dict[str, Any]:
        if not target.is_file(): return {}
        try:
            document = json.loads(target.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            raise AidcError(f"MCP 配置不是有效 JSON：{exc}", 6, "INTEGRATION_CONFIG_INVALID", str(target)) from exc
        if not isinstance(document, dict):
            raise AidcError("MCP 配置根节点必须是对象", 6, "INTEGRATION_CONFIG_INVALID", str(target))
        return document

    def _write_json(self, target: Path, spec: dict[str, Any]) -> None:
        document = self._read_json_config(target); servers = document.setdefault("mcpServers", {})
        if not isinstance(servers, dict):
            raise AidcError("mcpServers 必须是对象", 6, "INTEGRATION_CONFIG_INVALID", str(target))
        servers[SERVER_NAME] = spec
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(json.dumps(document, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    def install(self, host: str, config: Path | None = None) -> dict[str, Any]:
        target, skills_target = self._paths(host, config); existing = self._load_state(host)
        if existing and Path(existing["configPath"]).resolve() != target:
            raise AidcError("该宿主已接入到另一个配置文件，请先卸载", 7, "INTEGRATION_TARGET_CHANGED", str(target))
        baseline = existing.get("baseline") if existing else self._create_baseline(host, target, skills_target)
        spec = self.server_spec()
        if host == "codex": self._write_codex(target, spec)
        else: self._write_json(target, spec)
        skills = self._install_skills(host, skills_target)
        document = {"schemaVersion": "aidc.integration-state/1.0", "host": host, "configPath": str(target), "skillsPath": str(skills_target) if skills_target else None, "server": spec, "skills": skills, "baseline": baseline, "installedAt": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")}
        self._save_state(host, document)
        return {"host": host, "action": "installed", "configPath": str(target), "skillsPath": str(skills_target) if skills_target else None, "server": spec, "skills": skills, "backupPath": baseline["directory"]}

    def _configured_spec(self, host: str, target: Path) -> dict[str, Any]:
        if host == "codex":
            content = target.read_text(encoding="utf-8") if target.is_file() else ""
            match = re.search(rf"(?ms){re.escape(CODEX_BEGIN)}.*?command\s*=\s*(\"(?:\\.|[^\"])*\").*?args\s*=\s*(\[[^\]]*\]).*?{re.escape(CODEX_END)}", content)
            if not match: raise AidcError("未找到 AIDC 管理的 Codex 配置", 3, "INTEGRATION_NOT_FOUND", str(target))
            return {"command": json.loads(match.group(1)), "args": json.loads(match.group(2)), "env": {"PYTHONUTF8": "1"}}
        document = self._read_json_config(target); spec = document.get("mcpServers", {}).get(SERVER_NAME)
        if not isinstance(spec, dict): raise AidcError("未找到 AIDC MCP 配置", 3, "INTEGRATION_NOT_FOUND", str(target))
        return spec

    @staticmethod
    def _probe(spec: dict[str, Any]) -> dict[str, Any]:
        command = spec.get("command"); arguments = spec.get("args", [])
        if not isinstance(command, str) or not isinstance(arguments, list):
            raise AidcError("MCP command/args 配置无效", 6, "INTEGRATION_CONFIG_INVALID")
        requests = [
            {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "aidc-integration-check", "version": "1"}}},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
            {"jsonrpc": "2.0", "id": 3, "method": "resources/list"},
            {"jsonrpc": "2.0", "id": 4, "method": "tools/call", "params": {"name": "aidc_system_capabilities", "arguments": {}}},
        ]
        environment = os.environ.copy(); environment.update({str(key): str(value) for key, value in (spec.get("env") or {}).items()})
        try:
            completed = subprocess.run([command, *map(str, arguments)], input="".join(json.dumps(item) + "\n" for item in requests), capture_output=True, text=True, encoding="utf-8", errors="replace", env=environment, timeout=45, check=False)
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise AidcError(f"MCP 启动失败：{exc}", 6, "INTEGRATION_MCP_START_FAILED", command) from exc
        lines = [line for line in completed.stdout.splitlines() if line.strip()]
        if completed.returncode or len(lines) != len(requests):
            raise AidcError(f"MCP 兼容性检查失败：{completed.stderr or completed.stdout}", 6, "INTEGRATION_MCP_PROTOCOL_FAILED", command)
        try: responses = [json.loads(line) for line in lines]
        except json.JSONDecodeError as exc: raise AidcError(f"MCP 返回无效 JSON：{exc}", 6, "INTEGRATION_MCP_PROTOCOL_FAILED", command) from exc
        if any("error" in response for response in responses):
            raise AidcError("MCP 协议返回错误", 6, "INTEGRATION_MCP_PROTOCOL_FAILED", command)
        call = responses[3]["result"]
        return {"protocolVersion": responses[0]["result"]["protocolVersion"], "serverVersion": responses[0]["result"]["serverInfo"]["version"], "toolCount": len(responses[1]["result"]["tools"]), "resourceCount": len(responses[2]["result"]["resources"]), "coreCallPassed": not call.get("isError", True)}

    def check(self, host: str, config: Path | None = None) -> dict[str, Any]:
        target, skills_target = self._paths(host, config); spec = self._configured_spec(host, target)
        expected = self.server_spec()
        try:
            configured_command = str(Path(spec.get("command", "")).expanduser().resolve())
        except (OSError, TypeError, ValueError) as exc:
            raise AidcError(f"MCP command 无效：{exc}", 6, "INTEGRATION_CONFIG_INVALID", str(target)) from exc
        if configured_command != expected["command"] or spec.get("args", []) != expected["args"]:
            raise AidcError("宿主配置未指向当前 AIDC MCP 服务，请重新执行接入命令", 6, "INTEGRATION_SERVER_MISMATCH", str(target))
        probe = self._probe(spec)
        skills = []
        if skills_target:
            state = self._load_state(host) or {}
            for item in state.get("skills", []):
                current = Path(item["path"]); skills.append({"id": item["id"], "path": str(current), "valid": current.is_dir() and _tree_digest(current) == item["sha256"]})
            if len(skills) != len(self._skill_catalog()) or not all(item["valid"] for item in skills):
                raise AidcError("宿主 Skills 缺失或已损坏", 6, "INTEGRATION_SKILLS_INVALID", str(skills_target))
        return {"host": host, "action": "checked", "compatible": True, "configPath": str(target), "skillsPath": str(skills_target) if skills_target else None, "server": spec, "skills": skills, "probe": probe}

    def uninstall(self, host: str, config: Path | None = None) -> dict[str, Any]:
        state = self._load_state(host); target, skills_target = self._paths(host, config)
        if state: target = Path(state["configPath"]); skills_target = Path(state["skillsPath"]) if state.get("skillsPath") else None
        if host == "codex":
            content = target.read_text(encoding="utf-8") if target.is_file() else ""; target.write_text(self._remove_codex_block(content), encoding="utf-8")
        elif target.is_file():
            document = self._read_json_config(target); servers = document.get("mcpServers", {})
            if isinstance(servers, dict): servers.pop(SERVER_NAME, None)
            if host == "mcp" and not document.get("mcpServers"): target.unlink(missing_ok=True)
            else: target.write_text(json.dumps(document, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        restored_skills = []
        if skills_target and state:
            baseline = state["baseline"]; backup = Path(baseline["directory"])
            for item in self._skill_catalog():
                destination = skills_target / item["id"]
                if destination.exists(): shutil.rmtree(destination)
                before = baseline.get("skills", {}).get(item["id"], {})
                if before.get("existed"):
                    self._copy(backup / "skills" / item["id"], destination); restored_skills.append(item["id"])
        self._state_path(host).unlink(missing_ok=True)
        return {"host": host, "action": "uninstalled", "configPath": str(target), "skillsPath": str(skills_target) if skills_target else None, "restoredSkills": restored_skills}

    def restore(self, host: str) -> dict[str, Any]:
        state = self._load_state(host)
        if not state: raise AidcError("没有可恢复的接入备份", 3, "INTEGRATION_BACKUP_NOT_FOUND")
        target = Path(state["configPath"]); baseline = state["baseline"]; backup = Path(baseline["directory"])
        if baseline["configExisted"]: self._copy(backup / "config.original", target)
        else: target.unlink(missing_ok=True)
        skills_target = Path(state["skillsPath"]) if state.get("skillsPath") else None
        if skills_target:
            for item in self._skill_catalog():
                destination = skills_target / item["id"]
                if destination.exists(): shutil.rmtree(destination)
                if baseline.get("skills", {}).get(item["id"], {}).get("existed"):
                    self._copy(backup / "skills" / item["id"], destination)
        self._state_path(host).unlink(missing_ok=True)
        return {"host": host, "action": "restored", "configPath": str(target), "skillsPath": str(skills_target) if skills_target else None, "backupPath": str(backup)}
