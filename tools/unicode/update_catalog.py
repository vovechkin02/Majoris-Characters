"""Regenerate GNOME-derived catalogs from locally acquired Unicode source files."""

import argparse
import gzip
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from generate_emoji_annotations import coverage
from generate_emoji_sequences import generate as generate_emoji_sequences
from generate_emoji_sequences import read_catalog, read_official, verify_emoji_test
from generate_supplement import generate


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
    property_header = (base / "generated/properties.h").read_text(encoding="utf-8")
    if f'#define PROPERTY_VERSION "{version}"' not in property_header:
        raise ValueError("Generate the matching scalar property snapshot first")
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
        aliases = inputs / "NameAliases.txt"
        sequences = inputs / "NamedSequences.txt"
        (staging / "supplement.h").write_text(
            generate(aliases, sequences, version), encoding="utf-8",
        )
        for source in (aliases, sequences):
            (staging / source.name).write_bytes(source.read_bytes())
        # Alias indexes depend on the exact catalog order. Stage every dependent table
        # and verify official qualification identities before replacing any catalog.
        presentation = inputs / "emoji-variation-sequences.txt"
        bases = read_official(presentation)
        catalog = read_catalog(staging / "emoji.h")
        verify_emoji_test(inputs / "emoji-test.txt", catalog)
        generate_emoji_sequences(bases, catalog, staging / "emoji-sequences.h")
        (staging / "presentation-bases.txt").write_text(
            f"# Unicode {version} emoji-variation-sequences.txt: base scalars.\n"
            "# Copyright Unicode, Inc. See ../supplement/LICENSE.unicode.\n"
            f"# https://www.unicode.org/Public/{version}/ucd/emoji/"
            "emoji-variation-sequences.txt\n" + "".join(f"{point:04X}\n" for point in bases),
            encoding="utf-8",
        )
        emoji_data = root / "data/unicode/emoji"
        with gzip.open(emoji_data / "annotations.json.gz", "rt", encoding="utf-8") as stream:
            annotations = json.load(stream)
        report = {"source": annotations["source"],
                  "locales": coverage(annotations["locales"], catalog)}
        (staging / "coverage.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8",
        )
        for name, _ in GENERATORS:
            (staging / f"{name}.h").replace(vendor / f"{name}.h")
        (staging / "catalog-version.h").replace(base / "generated/catalog-version.h")
        (staging / "supplement.h").replace(base / "generated/supplement.h")
        (staging / "emoji-sequences.h").replace(base / "generated/emoji-sequences.h")
        for name in ("presentation-bases.txt", "coverage.json"):
            (staging / name).replace(emoji_data / name)
        for source in (aliases, sequences):
            (staging / source.name).replace(root / "data/unicode/supplement" / source.name)


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
