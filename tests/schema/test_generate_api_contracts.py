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
    "generate_api_contracts", ROOT / "tools" / "generate_api_contracts.py")
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class ApiContractsCodegenTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.api = MODULE.load_document(ROOT / "schema" / "api.yml")
        cls.events = MODULE.load_document(ROOT / "schema" / "events.yml")
        cls.capabilities = MODULE.load_document(ROOT / "schema" / "capabilities.yml")

    def documents(self, api=None, events=None, capabilities=None):
        return (
            copy.deepcopy(api or self.api),
            copy.deepcopy(events or self.events),
            copy.deepcopy(capabilities or self.capabilities),
        )

    def generate_all(self, api=None, events=None, capabilities=None):
        documents = self.documents(api, events, capabilities)
        return {
            relative: generator(*documents)
            for relative, generator, _ in MODULE.OUTPUTS
        }

    def test_versioned_artifacts_match_canonical_schemas(self):
        for relative, expected in self.generate_all().items():
            actual = (ROOT / relative).read_text(encoding="utf-8")
            self.assertEqual(expected, actual, f"artefato divergente: {relative}")

    def test_generation_is_deterministic(self):
        first = self.generate_all()
        second = self.generate_all()
        self.assertEqual(first, second)

    def test_generated_lua_covers_every_function(self):
        artifacts = self.generate_all()
        for function in self.api["functions"]:
            self.assertIn(function["name"], artifacts["generated/lua/shiplua_validate.lua"])
            self.assertIn(function["name"], artifacts["generated/tests/mock_contract.lua"])

    def test_mock_embeds_contract_capabilities_per_host(self):
        mock = self.generate_all()["generated/lua/shiplua_mock.lua"]
        self.assertIn('"mm.player.jump"', mock)
        self.assertIn('mm = { "core.events"', mock)
        self.assertIn('"mm.spawn_dog"', mock)

    def test_invalid_schema_is_rejected_by_check_mode(self):
        with tempfile.TemporaryDirectory() as directory:
            schema_dir = Path(directory)
            api = copy.deepcopy(self.api)
            api["functions"][0]["stability"] = "internal"
            (schema_dir / "api.yml").write_text(
                __import__("json").dumps(api), encoding="utf-8")
            for name in ("events.yml", "capabilities.yml"):
                (schema_dir / name).write_text(
                    (ROOT / "schema" / name).read_text(encoding="utf-8"), encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(ROOT / "tools/generate_api_contracts.py"),
                 "--schema-dir", str(schema_dir), "--check"],
                cwd=ROOT,
                capture_output=True,
                check=False,
            )
        self.assertEqual(1, result.returncode)
        self.assertIn("stability".encode("utf-8"), result.stderr)

    def test_check_mode_detects_drift(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            drifted = root / "generated/lua/shiplua_mock.lua"
            drifted.parent.mkdir(parents=True)
            drifted.write_text("desatualizado\n", encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(ROOT / "tools/generate_api_contracts.py"),
                 "--root", str(root), "--check"],
                cwd=ROOT,
                capture_output=True,
                check=False,
            )
        self.assertEqual(1, result.returncode)
        self.assertIn("diverge dos schemas".encode("utf-8"), result.stderr)
        self.assertIn("ausente".encode("utf-8"), result.stderr)

    def test_matrix_marks_host_specific_functions(self):
        matrix = self.generate_all()["generated/docs/compatibility-matrix.md"]
        self.assertIn("| `ship.mm.player.jump` | `0.2.0` | `experimental` | — | sim |", matrix)


if __name__ == "__main__":
    unittest.main()
