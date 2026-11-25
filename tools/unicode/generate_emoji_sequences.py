"""Generate presentation and selector-omission identities from pinned Unicode data."""

import argparse
import itertools
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data/unicode/emoji"
CATALOG = ROOT / "native/unicode/vendor/gnome/emoji.h"
OUTPUT = ROOT / "native/unicode/generated/emoji-sequences.h"


def read_bases(path: Path) -> list[int]:
    """Read the checked-in set of standardized presentation bases."""
    values = [
        int(line.split("#")[0].strip(), 16)
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.split("#")[0].strip()
    ]
    if values != sorted(set(values)) or any(not 0 < x <= 0x10FFFF or
                                           0xD800 <= x <= 0xDFFF for x in values):
        raise ValueError("Invalid presentation bases")
    return values


def read_official(path: Path) -> list[int]:
    """Require both standardized selectors for every base in the source file."""
    pairs = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        value = line.split("#")[0].strip()
        if not value:
            continue
        sequence, style, *_ = value.split(";")
        base, selector = (int(x, 16) for x in sequence.split())
        if (selector, style.strip()) not in ((0xFE0E, "text style"),
                                             (0xFE0F, "emoji style")):
            raise ValueError("Invalid presentation pair")
        if selector in pairs.setdefault(base, set()):
            raise ValueError("Duplicate presentation pair")
        pairs[base].add(selector)
    if not pairs or any(values != {0xFE0E, 0xFE0F} for values in pairs.values()):
        raise ValueError("Incomplete presentation pairs")
    return sorted(pairs)


def read_catalog(path: Path) -> list[tuple[int, ...]]:
    """Read exact sequences in the existing private catalog index order."""
    source = path.read_text(encoding="utf-8")
    rows = re.findall(r'\{\s*\{\s*([^{}]+)\s*\},\s*(\d+),\s*"', source)
    result = []
    for points, length in rows:
        sequence = tuple(int(x.strip(), 16) for x in points.split(","))
        if len(sequence) != int(length):
            raise ValueError("Catalog length mismatch")
        result.append(sequence)
    if not result or len(set(result)) != len(result):
        raise ValueError("Empty or duplicate catalog")
    return result


def selector_omissions(catalog: list[tuple[int, ...]]) -> dict:
    """Enumerate omissions of FE0F only, rejecting ambiguous canonical identities."""
    indices = {sequence: index for index, sequence in enumerate(catalog)}
    aliases = {}
    for index, sequence in enumerate(catalog):
        selectors = [i for i, point in enumerate(sequence) if point == 0xFE0F]
        for count in range(1, len(selectors) + 1):
            for removed in itertools.combinations(selectors, count):
                variant = tuple(p for i, p in enumerate(sequence) if i not in removed)
                if variant not in indices:
                    previous = aliases.setdefault(variant, index)
                    if previous != index:
                        raise ValueError("Ambiguous qualification alias")
    return aliases


def verify_emoji_test(path: Path, catalog: list[tuple[int, ...]]) -> None:
    """Compare fully, minimally and unqualified sets to official emoji-test.txt."""
    groups = {status: set() for status in (
        "fully-qualified", "minimally-qualified", "unqualified", "component"
    )}
    for line in path.read_text(encoding="utf-8").splitlines():
        value = line.split("#")[0].strip()
        if not value:
            continue
        sequence, status = value.split(";")
        points = tuple(int(x, 16) for x in sequence.split())
        group = groups[status.strip()]
        if points in group:
            raise ValueError("Duplicate emoji-test sequence")
        group.add(points)
    if groups["fully-qualified"] != set(catalog):
        raise ValueError("Fully qualified catalog differs from emoji-test data")
    omissions = groups["minimally-qualified"] | groups["unqualified"]
    if omissions != set(selector_omissions(catalog)):
        raise ValueError("Selector omissions differ from official emoji-test data")


def build_aliases(catalog: list[tuple[int, ...]], bases: list[int]) -> dict:
    """Build only exact standard pairs or omissions of existing emoji selectors."""
    indices = {sequence: index for index, sequence in enumerate(catalog)}
    aliases = selector_omissions(catalog)
    for base in bases:
        canonical = indices.get((base, 0xFE0F), indices.get((base,), None))
        for variant in ((base,), (base, 0xFE0E), (base, 0xFE0F)):
            if variant not in indices:
                aliases[variant] = canonical
    return aliases


def generate(bases: list[int], catalog: list[tuple[int, ...]], output: Path) -> None:
    """Emit immutable sorted tables; no runtime parsing or network dependency."""
    aliases = build_aliases(catalog, bases)
    lines = [
        "/* Generated emoji presentation and qualification tables; do not edit. */",
        "static const uint32_t presentation_bases[] = {",
    ]
    for offset in range(0, len(bases), 8):
        values = ", ".join(f"0x{x:X}" for x in bases[offset:offset + 8])
        lines.append("    " + values + ",")
    lines.extend(["};", "static const struct SequenceAlias sequence_aliases[] = {"])
    for points, target in sorted(aliases.items()):
        text = json.dumps("".join(map(chr, points)), ensure_ascii=False)
        index = "SIZE_MAX" if target is None else str(target)
        lines.append(f"    {{ {text}, {index} }},")
    lines.append("};")
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    """Regenerate from bundled inputs or compare against an acquired official file."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--official", type=Path)
    parser.add_argument("--emoji-test", type=Path)
    parser.add_argument("--bases", type=Path, default=DATA / "presentation-bases.txt")
    parser.add_argument("--catalog", type=Path, default=CATALOG)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()
    bases = read_bases(args.bases)
    if args.official and read_official(args.official) != bases:
        raise ValueError("Official presentation bases differ from pinned snapshot")
    catalog = read_catalog(args.catalog)
    if args.emoji_test:
        verify_emoji_test(args.emoji_test, catalog)
    generate(bases, catalog, args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
