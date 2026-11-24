"""Generate compact alias and named-sequence tables from local UCD text files."""

import argparse
import json
from pathlib import Path
import re


def records(path: Path):
    """Yield non-comment semicolon-delimited UCD fields in input order."""
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        content = line.partition("#")[0].strip()
        if content:
            yield [field.strip() for field in content.split(";")]


def scalar(value: str) -> int:
    """Parse one hexadecimal Unicode scalar, rejecting surrogate values."""
    point = int(value, 16)
    if not 0 <= point <= 0x10FFFF or 0xD800 <= point <= 0xDFFF:
        raise ValueError(f"Invalid scalar: {value}")
    return point


def generate(aliases: Path, sequences: Path, version: str) -> str:
    """Produce deterministic C tables and reject malformed or duplicate records."""
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Expected a three-part Unicode version")
    alias_rows = sorted((scalar(cp), name, kind) for cp, name, kind in records(aliases))
    sequence_rows = sorted(
        (name, tuple(scalar(cp) for cp in points.split()))
        for name, points in records(sequences)
    )
    if not alias_rows or not sequence_rows:
        raise ValueError("Both databases must contain records")
    if len(set(alias_rows)) != len(alias_rows):
        raise ValueError("Duplicate aliases")
    if len({name for name, _ in sequence_rows}) != len(sequence_rows):
        raise ValueError("Duplicate sequence names")
    if any(not 2 <= len(points) <= 32 or 0 in points for _, points in sequence_rows):
        raise ValueError("Unsupported named sequence length or embedded NUL")
    output = [
        "/* Generated Unicode data; do not edit. */",
        f'#define SUPPLEMENT_VERSION "{version}"',
        "static const struct AliasEntry aliases[] = {",
    ]
    for cp, name, kind in alias_rows:
        output.append(f"    {{ 0x{cp:X}, {json.dumps(name)}, {json.dumps(kind)} }},")
    output.extend(["};", "static const struct NamedEntry named_sequences[] = {"])
    for name, points in sequence_rows:
        values = ", ".join(f"0x{cp:X}" for cp in points)
        output.append(f"    {{ {json.dumps(name)}, {{ {values} }}, {len(points)} }},")
    output.append("};")
    return "\n".join(output) + "\n"


def main() -> int:
    """Read local inputs and atomically replace the generated header."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("aliases", type=Path)
    parser.add_argument("sequences", type=Path)
    parser.add_argument("version")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    content = generate(args.aliases, args.sequences, args.version)
    temporary = args.output.with_suffix(".tmp")
    temporary.write_text(content, encoding="utf-8")
    temporary.replace(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
