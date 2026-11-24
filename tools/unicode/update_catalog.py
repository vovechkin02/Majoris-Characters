"""Regenerate GNOME-derived catalogs from locally acquired Unicode source files."""

import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Kept in dependency order: names consume both UCD and the emoji test input.
GENERATORS = (
    ("blocks", ("Blocks.txt",)),
    ("emoji", ("emoji-test.txt",)),
    ("names", ("UnicodeData.txt", "emoji-test.txt")),
    ("confusables", ("confusables.txt",)),
)


def regenerate(root: Path, inputs: Path, version: str) -> None:
    """Generate all outputs before replacing any catalog, without network access."""
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Expected a three-part Unicode version")
    base = root / "native/unicode"
    vendor = base / "vendor/gnome"
    with tempfile.TemporaryDirectory(dir=base) as directory:
        staging = Path(directory)
        for name, sources in GENERATORS:
            paths = [inputs / source for source in sources]
            if any(not path.is_file() for path in paths):
                raise FileNotFoundError(f"Missing data inputs: {paths}")
            result = subprocess.run(
                [sys.executable, str(vendor / f"gen-{name}.py"), *map(str, paths)],
                check=True, capture_output=True, timeout=180,
            )
            if len(result.stdout) < 100:
                raise ValueError(f"Unexpectedly small generated {name} table")
            (staging / f"{name}.h").write_bytes(result.stdout)
        (staging / "catalog-version.h").write_text(
            "/* Generated catalog identity; update with the upstream tables. */\n"
            f'#define CATALOG_VERSION "{version}"\n', encoding="utf-8",
        )
        for name, _ in GENERATORS:
            (staging / f"{name}.h").replace(vendor / f"{name}.h")
        (staging / "catalog-version.h").replace(base / "generated/catalog-version.h")


def main() -> int:
    """Parse local input paths and update the repository's checked-in tables."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", type=Path)
    parser.add_argument("version")
    args = parser.parse_args()
    regenerate(Path(__file__).resolve().parents[2], args.inputs.resolve(), args.version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
