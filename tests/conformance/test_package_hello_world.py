from __future__ import annotations

import pathlib
import tempfile
import unittest
import zipfile

from PackageHelloWorld import FIXED_TIMESTAMP, PACKAGE_FILES, package


class HelloWorldPackagingTests(unittest.TestCase):
    def test_package_is_reproducible_and_canonical(self) -> None:
        source = pathlib.Path(__file__).parents[2] / "examples" / "hello-world"
        with tempfile.TemporaryDirectory() as temporary:
            first = pathlib.Path(temporary) / "first.shipmod"
            second = pathlib.Path(temporary) / "second.shipmod"
            package(source, first)
            package(source, second)

            self.assertEqual(first.read_bytes(), second.read_bytes())
            with zipfile.ZipFile(first) as archive:
                self.assertEqual(archive.namelist(), list(PACKAGE_FILES))
                for name in PACKAGE_FILES:
                    info = archive.getinfo(name)
                    self.assertEqual(info.date_time, FIXED_TIMESTAMP)
                    self.assertEqual(info.external_attr >> 16, 0o100644)
                    self.assertEqual(archive.read(name), (source / name).read_bytes())


if __name__ == "__main__":
    unittest.main()
