"""Testes do CLI de desenvolvimento `tools/shipmod.py` (MODSDK-006).

Segue a convenção do repositório: unittest executado via ctest com o
interpretador Python encontrado pelo CMake.
"""

from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

import shipmod  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]


def run_cli(argv: list[str]) -> tuple[int, str, str]:
    """Executa o CLI capturando stdout/stderr, sem encerrar o processo."""
    stdout, stderr = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
        try:
            code = shipmod.main(argv)
        except SystemExit as exit_:  # erros de argparse saem com código 2
            code = exit_.code if isinstance(exit_.code, int) else 2
    return code, stdout.getvalue(), stderr.getvalue()


class NewCommandTests(unittest.TestCase):
    def test_scaffold_creates_expected_structure(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            code, out, _ = run_cli(["new", "kafei-puppet", "--dir", temporary])
            self.assertEqual(code, shipmod.EXIT_OK)

            mod_dir = Path(temporary) / "kafei-puppet"
            manifest = (mod_dir / "manifest.toml").read_text(encoding="utf-8")
            self.assertIn('id = "community.kafei_puppet"', manifest)
            self.assertIn('api = ">=0.1 <0.5"', manifest)
            self.assertIn('entrypoint = "main.lua"', manifest)
            self.assertTrue((mod_dir / "main.lua").is_file())
            self.assertTrue((mod_dir / "README.md").is_file())
            test_file = mod_dir / "tests" / "kafei_puppet_test.lua"
            self.assertTrue(test_file.is_file())
            self.assertIn("describe(", test_file.read_text(encoding="utf-8"))
            self.assertIn("community.kafei_puppet", out)

    @unittest.skipIf(shipmod.tomllib is None, "requer Python 3.11+ (tomllib)")
    def test_scaffold_manifest_passes_basic_validation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            code, _, _ = run_cli(["new", "puppet-show", "--dir", temporary])
            self.assertEqual(code, shipmod.EXIT_OK)
            ok, message = shipmod._validate_basic(Path(temporary) / "puppet-show")
            self.assertTrue(ok, message)

    def test_scaffold_honors_custom_id_author_and_description(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            code, _, _ = run_cli([
                "new", "meu-mod", "--dir", temporary,
                "--id", "baiteryamato.meu_mod",
                "--autor", "Kimi",
                "--desc", "Descrição personalizada.",
            ])
            self.assertEqual(code, shipmod.EXIT_OK)
            manifest = (Path(temporary) / "meu-mod" / "manifest.toml").read_text(encoding="utf-8")
            self.assertIn('id = "baiteryamato.meu_mod"', manifest)
            self.assertIn('authors = ["Kimi"]', manifest)
            self.assertIn('description = "Descrição personalizada."', manifest)

    def test_scaffold_escapes_toml_and_lua_strings(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            code, _, _ = run_cli([
                "new", "meu-mod", "--dir", temporary,
                "--id", 'community.meu_mod"quoted',
                "--autor", 'Autor "Teste"',
                "--desc", "Linha 1\nLinha 2",
            ])
            self.assertEqual(code, shipmod.EXIT_OK)
            mod_dir = Path(temporary) / "meu-mod"
            ok, message = shipmod._validate_basic(mod_dir)
            self.assertTrue(ok, message)
            main_lua = (mod_dir / "main.lua").read_text(encoding="utf-8")
            self.assertIn(r'community.meu_mod\"quoted', main_lua)

    def test_scaffold_refuses_invalid_name(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            for invalid in ("MeuMod", "mod_name", "1mod"):
                code, _, err = run_cli(["new", invalid, "--dir", temporary])
                self.assertEqual(code, shipmod.EXIT_USAGE, invalid)
                self.assertIn("inválido", err)
            # "-mod" é interceptado pelo argparse como opção desconhecida (uso inválido)
            code, _, _ = run_cli(["new", "--", "-mod", "--dir", temporary])
            self.assertEqual(code, shipmod.EXIT_USAGE)

    def test_scaffold_refuses_non_empty_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary) / "kafei-puppet"
            target.mkdir()
            (target / "ocupado.txt").write_text("x", encoding="utf-8")
            code, _, err = run_cli(["new", "kafei-puppet", "--dir", temporary])
            self.assertEqual(code, shipmod.EXIT_FAILURE)
            self.assertIn("não está vazio", err)


class ValidateFallbackTests(unittest.TestCase):
    """Fallback estrutural em Python, sem o validador canônico compilado."""

    def setUp(self) -> None:
        if shipmod.tomllib is None:
            self.skipTest("requer Python 3.11+ (tomllib)")
        patcher = mock.patch.object(shipmod, "find_repo_tool", return_value=None)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_valid_example_directory(self) -> None:
        code, out, _ = run_cli(["validate", str(REPO_ROOT / "examples" / "hello-world")])
        self.assertEqual(code, shipmod.EXIT_OK)
        self.assertIn("example.hello_world", out)

    def test_valid_manifest_file(self) -> None:
        fixture = REPO_ROOT / "tests" / "fixtures" / "manifest" / "valid-minimal.toml"
        code, _, _ = run_cli(["validate", str(fixture)])
        self.assertEqual(code, shipmod.EXIT_OK)

    def test_invalid_version_fixture(self) -> None:
        fixture = REPO_ROOT / "tests" / "fixtures" / "manifest" / "invalid-version.toml"
        code, _, err = run_cli(["validate", str(fixture)])
        self.assertEqual(code, shipmod.EXIT_FAILURE)
        self.assertIn("Inválido", err)

    def test_missing_path(self) -> None:
        code, _, err = run_cli(["validate", "caminho/inexistente"])
        self.assertEqual(code, shipmod.EXIT_FAILURE)
        self.assertIn("Inválido", err)

    def test_shipmod_package(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            package = Path(temporary) / "pacote.shipmod"
            with zipfile.ZipFile(package, "w") as archive:
                archive.writestr(
                    "manifest.toml",
                    'id = "community.teste"\nname = "Teste"\nversion = "1.0.0"\n'
                    'api = ">=0.1"\nentrypoint = "main.lua"\n',
                )
                archive.writestr("main.lua", 'local ship = require("ship")\n')
            code, out, _ = run_cli(["validate", str(package)])
            self.assertEqual(code, shipmod.EXIT_OK)
            self.assertIn("community.teste", out)


class ValidateDelegationTests(unittest.TestCase):
    """Com o validador canônico presente, o CLI apenas delega e propaga o código."""

    def test_delegates_to_canonical_validator(self) -> None:
        fake = Path("C:/fake/shiplua_manifest_validator.exe")
        completed = mock.Mock(returncode=0)
        with mock.patch.object(shipmod, "find_repo_tool", return_value=fake), \
             mock.patch.object(shipmod.subprocess, "run", return_value=completed) as run:
            code, _, _ = run_cli(["validate", "a", "b"])
        self.assertEqual(code, 0)
        run.assert_called_once_with([str(fake), "a", "b"])

    def test_propagates_validator_failure(self) -> None:
        fake = Path("C:/fake/shiplua_manifest_validator.exe")
        completed = mock.Mock(returncode=1)
        with mock.patch.object(shipmod, "find_repo_tool", return_value=fake), \
             mock.patch.object(shipmod.subprocess, "run", return_value=completed):
            code, _, _ = run_cli(["validate", "mod-invalido"])
        self.assertEqual(code, 1)

    def test_unexpected_validator_exit_is_diagnosed(self) -> None:
        fake = Path("C:/fake/shiplua_manifest_validator.exe")
        completed = mock.Mock(returncode=127)
        with mock.patch.object(shipmod, "find_repo_tool", return_value=fake), \
             mock.patch.object(shipmod.subprocess, "run", return_value=completed):
            code, _, err = run_cli(["validate", "mod"])
        self.assertEqual(code, 127)
        self.assertIn("127", err)
        self.assertIn("PATH", err)


class TestCommandTests(unittest.TestCase):
    def test_missing_runner_fails_with_clear_message(self) -> None:
        with mock.patch.object(shipmod, "find_repo_tool", return_value=None):
            code, _, err = run_cli(["test", "examples/hello-runtime"])
        self.assertEqual(code, shipmod.EXIT_MISSING_TOOL)
        self.assertIn("shiplua_mod_test_runner", err)
        self.assertIn("MODSDK-004", err)

    def test_delegates_with_game_and_capabilities(self) -> None:
        fake = Path("C:/fake/shiplua_mod_test_runner.exe")
        completed = mock.Mock(returncode=0)
        with mock.patch.object(shipmod, "find_repo_tool", return_value=fake), \
             mock.patch.object(shipmod.subprocess, "run", return_value=completed) as run:
            code, _, _ = run_cli([
                "test", "meu-mod", "--game", "mm",
                "--capability", "core.input", "--capability", "mm.cycle",
            ])
        self.assertEqual(code, 0)
        run.assert_called_once_with([
            str(fake), "meu-mod", "--game", "mm",
            "--capability", "core.input", "--capability", "mm.cycle",
        ])


class DoctorTests(unittest.TestCase):
    def test_python_check_ok_on_supported_version(self) -> None:
        status, name, _ = shipmod._check_python()
        self.assertEqual(name, "Python")
        if sys.version_info >= (3, 11):
            self.assertEqual(status, "ok")
        else:
            self.assertIn(status, {"aviso", "falha"})

    def test_parse_version(self) -> None:
        self.assertEqual(shipmod._parse_version("cmake version 3.28.1"), (3, 28, 1))
        self.assertIsNone(shipmod._parse_version("sem versão"))

    def test_doctor_runs_all_checks_and_summarizes(self) -> None:
        code, out, _ = run_cli(["doctor"])
        self.assertIn(code, (shipmod.EXIT_OK, shipmod.EXIT_FAILURE))
        for expected in ("Python", "git", "CMake", "Schemas", "Resumo"):
            self.assertIn(expected, out)


if __name__ == "__main__":
    unittest.main()
