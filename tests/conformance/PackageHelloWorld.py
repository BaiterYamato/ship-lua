from __future__ import annotations

import pathlib
import sys
import zipfile


FIXED_TIMESTAMP = (1980, 1, 1, 0, 0, 0)
PACKAGE_FILES = ("main.lua", "manifest.toml")


def package(source_dir: pathlib.Path, output: pathlib.Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + ".tmp")
    temporary.unlink(missing_ok=True)

    try:
        with zipfile.ZipFile(
            temporary, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
        ) as archive:
            for name in PACKAGE_FILES:
                info = zipfile.ZipInfo(name, FIXED_TIMESTAMP)
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                archive.writestr(info, (source_dir / name).read_bytes())
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)


def main() -> int:
    if len(sys.argv) != 3:
        print("Uso: PackageHelloWorld.py <diretório-fonte> <saída.shipmod>", file=sys.stderr)
        return 2
    package(pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
