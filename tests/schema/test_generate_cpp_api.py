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
    "generate_cpp_api", ROOT / "tools" / "generate_cpp_api.py")
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class CppCodegenTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.api = MODULE.load_document(ROOT / "schema" / "api.yml")
        cls.events = MODULE.load_document(ROOT / "schema" / "events.yml")
        cls.capabilities = MODULE.load_document(ROOT / "schema" / "capabilities.yml")

    def generate(self, api=None, events=None, capabilities=None):
        return MODULE.generate(
            copy.deepcopy(api or self.api),
            copy.deepcopy(events or self.events),
            copy.deepcopy(capabilities or self.capabilities),
        )

    def test_versioned_header_matches_canonical_schemas(self):
        expected = self.generate()
        actual = (ROOT / "generated/include/shiplua/generated/ApiBindings.h").read_text(
            encoding="utf-8")
        self.assertEqual(expected, actual)

    def test_generation_is_deterministic(self):
        self.assertEqual(self.generate(), self.generate())

    def test_invalid_schema_is_rejected_before_generation(self):
        api = copy.deepcopy(self.api)
        api["types"][3]["fields"][0]["type"] = "Actor*"
        with self.assertRaisesRegex(ValueError, "tipo proibido"):
            self.generate(api=api)

    def test_check_mode_detects_drift(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ApiBindings.h"
            output.write_text("desatualizado\n", encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(ROOT / "tools/generate_cpp_api.py"),
                 "--output", str(output), "--check"],
                cwd=ROOT,
                capture_output=True,
                check=False,
            )
        self.assertEqual(1, result.returncode)
        self.assertIn("diverge dos schemas".encode("utf-8"), result.stderr)

    def test_cpp_identifier_collision_is_rejected(self):
        api = copy.deepcopy(self.api)
        first = copy.deepcopy(api["functions"][0])
        second = copy.deepcopy(api["functions"][0])
        first["name"] = "ship.a_b.c"
        second["name"] = "ship.a.b_c"
        api["functions"] = [first, second]
        with self.assertRaisesRegex(ValueError, "mesmo identificador C\\+\\+"):
            self.generate(api=api)


if __name__ == "__main__":
    unittest.main()
