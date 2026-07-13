from __future__ import annotations

import copy
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "generate_api_docs", ROOT / "tools" / "generate_api_docs.py")
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class ApiDocsCodegenTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.api = MODULE.load_document(ROOT / "schema" / "api.yml")
        cls.events = MODULE.load_document(ROOT / "schema" / "events.yml")
        cls.capabilities = MODULE.load_document(ROOT / "schema" / "capabilities.yml")

    def generate(self, api=None, events=None, capabilities=None):
        documents = (
            copy.deepcopy(api or self.api),
            copy.deepcopy(events or self.events),
            copy.deepcopy(capabilities or self.capabilities),
        )
        return (MODULE.generate_luadoc(*documents), MODULE.generate_markdown(*documents))

    def test_versioned_artifacts_match_canonical_schemas(self):
        luadoc, markdown = self.generate()
        self.assertEqual(luadoc, (ROOT / "generated/lua/shiplua.lua").read_text(encoding="utf-8"))
        self.assertEqual(markdown, (ROOT / "generated/docs/api-reference.md").read_text(
            encoding="utf-8"))

    def test_luadoc_contains_event_completion_and_types(self):
        luadoc, _ = self.generate()
        self.assertIn("---@alias ShipLuaEventName", luadoc)
        self.assertIn("---@param event ShipLuaEventName", luadoc)
        self.assertIn("---@class ShipLuaActorHandle", luadoc)

    def test_markdown_is_pt_br_and_covers_all_sections(self):
        _, markdown = self.generate()
        for heading in ("## Tipos", "## Funções", "## Eventos", "## Capabilities",
                        "## Códigos de erro"):
            self.assertIn(heading, markdown)
        self.assertIn("Cancelável", markdown)
        self.assertIn("não", markdown)

    def test_invalid_schema_is_rejected(self):
        events = copy.deepcopy(self.events)
        events["events"][0]["payload"][0]["type"] = "NativePointer*"
        with self.assertRaisesRegex(ValueError, "tipo proibido"):
            self.generate(events=events)

    def test_check_mode_reports_both_drifted_outputs(self):
        with tempfile.TemporaryDirectory() as directory:
            luadoc = Path(directory) / "shiplua.lua"
            markdown = Path(directory) / "api-reference.md"
            luadoc.write_text("antigo\n", encoding="utf-8")
            markdown.write_text("antigo\n", encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(ROOT / "tools/generate_api_docs.py"), "--check",
                 "--luadoc-output", str(luadoc), "--markdown-output", str(markdown)],
                cwd=ROOT,
                capture_output=True,
                check=False,
            )
        self.assertEqual(1, result.returncode)
        self.assertIn("LuaDoc diverge".encode("utf-8"), result.stderr)
        self.assertIn("Markdown diverge".encode("utf-8"), result.stderr)


if __name__ == "__main__":
    unittest.main()
