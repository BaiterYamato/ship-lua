from __future__ import annotations

import copy
import importlib.util
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "validate_api_schemas", ROOT / "tools" / "validate_api_schemas.py")
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class SchemaValidationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.api = MODULE.load_document(ROOT / "schema" / "api.yml")
        cls.events = MODULE.load_document(ROOT / "schema" / "events.yml")
        cls.capabilities = MODULE.load_document(ROOT / "schema" / "capabilities.yml")

    def validate(self, api=None, events=None, capabilities=None):
        return MODULE.validate_documents(
            copy.deepcopy(api or self.api),
            copy.deepcopy(events or self.events),
            copy.deepcopy(capabilities or self.capabilities),
        )

    def test_canonical_documents_are_valid(self):
        self.assertEqual([], self.validate())

    def test_orphan_capability_is_rejected(self):
        events = copy.deepcopy(self.events)
        events["events"][0]["capability"] = "missing.capability"
        self.assertTrue(any("órfã" in error for error in self.validate(events=events)))

    def test_pointer_type_is_rejected(self):
        api = copy.deepcopy(self.api)
        api["types"][3]["fields"][0]["type"] = "Actor*"
        self.assertTrue(any("proibido" in error for error in self.validate(api=api)))

    def test_specific_event_requires_capability(self):
        events = copy.deepcopy(self.events)
        events["events"][0]["support"] = ["mm"]
        self.assertTrue(any("suporte específico" in error for error in self.validate(events=events)))

    def test_planned_capability_cannot_be_referenced(self):
        events = copy.deepcopy(self.events)
        events["events"][0]["capability"] = "mm.cycle"
        events["events"][0]["support"] = ["mm"]
        self.assertTrue(any("ainda não contratada" in error for error in self.validate(events=events)))

    def test_unknown_payload_type_is_rejected(self):
        events = copy.deepcopy(self.events)
        events["events"][0]["payload"][0]["type"] = "internal_layout"
        self.assertTrue(any("tipo desconhecido" in error for error in self.validate(events=events)))

    def test_cli_emits_utf8_on_windows(self):
        result = subprocess.run(
            [sys.executable, str(ROOT / "tools" / "validate_api_schemas.py")],
            cwd=ROOT,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, result.returncode)
        self.assertIn("Schemas ShipLua válidos".encode("utf-8"), result.stdout)


if __name__ == "__main__":
    unittest.main()
