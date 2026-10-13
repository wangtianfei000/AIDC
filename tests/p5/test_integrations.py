from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from aidc_core.integrations import CODEX_BEGIN, IntegrationManager
from aidc_core.result import AidcError


ROOT = Path(__file__).resolve().parents[2]


class HostIntegrationTests(unittest.TestCase):
    def test_codex_install_check_idempotency_and_uninstall(self):
        with tempfile.TemporaryDirectory(prefix="aidc-p5-codex-") as directory:
            base = Path(directory); home = base / "home"; state = base / "state"
            config = home / ".codex" / "config.toml"; config.parent.mkdir(parents=True)
            config.write_text('model = "test"\n', encoding="utf-8")
            previous = home / ".codex" / "skills" / "robot-requirements-authoring"
            previous.mkdir(parents=True); (previous / "SKILL.md").write_text("previous skill", encoding="utf-8")
            manager = IntegrationManager(ROOT, home=home, state=state)

            installed = manager.install("codex", config)
            self.assertEqual(installed["action"], "installed")
            self.assertEqual(config.read_text(encoding="utf-8").count(CODEX_BEGIN), 1)
            self.assertEqual(len(installed["skills"]), 4)
            checked = manager.check("codex", config)
            self.assertTrue(checked["compatible"]); self.assertEqual(checked["probe"]["toolCount"], 17)
            self.assertEqual(checked["probe"]["resourceCount"], 4)

            manager.install("codex", config)
            self.assertEqual(config.read_text(encoding="utf-8").count(CODEX_BEGIN), 1)
            manager.uninstall("codex", config)
            self.assertEqual(config.read_text(encoding="utf-8"), 'model = "test"\n')
            self.assertEqual((previous / "SKILL.md").read_text(encoding="utf-8"), "previous skill")
            self.assertFalse((home / ".codex" / "skills" / "robot-system-architecture").exists())

    def test_cursor_preserves_other_servers_and_restores_baseline(self):
        with tempfile.TemporaryDirectory(prefix="aidc-p5-cursor-") as directory:
            base = Path(directory); home = base / "home"; state = base / "state"
            config = home / ".cursor" / "mcp.json"; config.parent.mkdir(parents=True)
            original = {"mcpServers": {"existing": {"command": "existing.exe"}}, "setting": True}
            config.write_text(json.dumps(original), encoding="utf-8")
            manager = IntegrationManager(ROOT, home=home, state=state)

            installed = manager.install("cursor", config)
            document = json.loads(config.read_text(encoding="utf-8"))
            self.assertIn("existing", document["mcpServers"]); self.assertIn("aidc", document["mcpServers"])
            self.assertEqual(len(installed["skills"]), 4)
            checked = manager.check("cursor", config)
            self.assertTrue(checked["probe"]["coreCallPassed"])

            restored = manager.restore("cursor")
            self.assertEqual(restored["action"], "restored")
            self.assertEqual(json.loads(config.read_text(encoding="utf-8")), original)
            self.assertFalse((home / ".cursor" / "skills" / "robot-modelica-model-development").exists())

    def test_generic_mcp_template_and_targeted_uninstall(self):
        with tempfile.TemporaryDirectory(prefix="aidc-p5-mcp-") as directory:
            base = Path(directory); target = base / "client" / "mcp.json"
            manager = IntegrationManager(ROOT, home=base / "home", state=base / "state")
            manager.install("mcp", target)
            configured = json.loads(target.read_text(encoding="utf-8"))["mcpServers"]["aidc"]
            self.assertTrue(Path(configured["command"]).is_file())
            self.assertTrue(manager.check("mcp", target)["compatible"])
            manager.uninstall("mcp", target)
            self.assertFalse(target.exists())

    def test_invalid_json_is_not_overwritten(self):
        with tempfile.TemporaryDirectory(prefix="aidc-p5-invalid-") as directory:
            target = Path(directory) / "mcp.json"; target.write_text("{invalid", encoding="utf-8")
            manager = IntegrationManager(ROOT, home=Path(directory), state=Path(directory) / "state")
            with self.assertRaises(AidcError) as caught: manager.install("cursor", target)
            self.assertEqual(caught.exception.code, "INTEGRATION_CONFIG_INVALID")
            self.assertEqual(target.read_text(encoding="utf-8"), "{invalid")

    def test_check_rejects_foreign_command_without_starting_it(self):
        with tempfile.TemporaryDirectory(prefix="aidc-p5-foreign-") as directory:
            target = Path(directory) / "mcp.json"
            target.write_text(json.dumps({"mcpServers": {"aidc": {"command": "foreign-command.exe", "args": []}}}), encoding="utf-8")
            manager = IntegrationManager(ROOT, home=Path(directory), state=Path(directory) / "state")
            with self.assertRaises(AidcError) as caught: manager.check("mcp", target)
            self.assertEqual(caught.exception.code, "INTEGRATION_SERVER_MISMATCH")


if __name__ == "__main__": unittest.main()
