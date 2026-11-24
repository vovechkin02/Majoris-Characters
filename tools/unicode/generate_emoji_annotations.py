"""Generate a deterministic native lookup table from local emoji annotation data."""

import argparse
import gzip
import json
from pathlib import Path
import re
import unicodedata
import xml.etree.ElementTree as ET


def key(points: list[int]) -> str:
    """Remove presentation selectors and GTK modifier placeholders from a lookup key."""
    return "".join(chr(point) for point in points if point not in (0, 0xFE0E, 0xFE0F))


def fold(text: str) -> str:
    """Normalize annotation text for case-insensitive multilingual search."""
    return unicodedata.normalize("NFKC", text).casefold()


def quote(text: str) -> str:
    """Quote a UTF-8 C string without relying on compiler-specific Unicode escapes."""
    return json.dumps(text, ensure_ascii=False)


def read_cldr(root: Path) -> dict:
    """Import resolved or explicit CLDR annotation XML, merging derived entries by locale."""
    locales = {}
    for directory in ("annotations", "annotationsDerived"):
        for path in sorted((root / directory).glob("*.xml")):
            if path.stem == "root":
                continue
            rows = locales.setdefault(path.stem, {})
            for node in ET.parse(path).iter("annotation"):
                text = node.get("cp", "")
                value = (node.text or "").strip()
                if not text or not value or value == "↑↑↑" or node.get("alt"):
                    continue
                record = rows.setdefault(text, [list(map(ord, text)), "", []])
                if node.get("type") == "tts":
                    record[1] = value
                else:
                    record[2].extend(word.strip() for word in value.split("|") if word.strip())
    return {lang: list(rows.values()) for lang, rows in locales.items()}


def generate(locales: dict, source: str, output: Path) -> None:
    """Emit sorted, deduplicated lookup records with immutable locale tables."""
    lines = ["/* Generated emoji annotation tables; do not edit. */",
             f"#define EMOJI_ANNOTATION_SOURCE {quote(source)}"]
    for language, rows in sorted(locales.items()):
        if not re.fullmatch(r"[a-zA-Z0-9_]+", language):
            raise ValueError(f"Invalid locale: {language}")
        records = {}
        for points, name, keywords in rows:
            sequence = key(points)
            if not sequence:
                continue
            record = records.setdefault(sequence, set())
            record.update(fold(value) for value in [name, *keywords] if value)
        lines.append(f"static const struct Annotation annotations_{language}[] = {{")
        for sequence, words in sorted(records.items(), key=lambda pair: pair[0].encode()):
            lines.append(f"    {{ {quote(sequence)}, {quote(' | '.join(sorted(words)))} }},")
        lines.append("};")
    lines.append("static const struct AnnotationLocale annotation_locales[] = {")
    for language in sorted(locales):
        lines.append(f'    {{ "{language}", annotations_{language}, COUNT(annotations_{language}) }},')
    lines.append("};")
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    """Generate bundled tables or import a locally acquired CLDR common directory."""
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cldr-common", type=Path)
    parser.add_argument("--cldr-version")
    parser.add_argument("--source", type=Path,
                        default=root / "data/unicode/emoji/annotations.json.gz")
    parser.add_argument("--output", type=Path,
                        default=root / "native/unicode/generated/emoji-annotations.h")
    args = parser.parse_args()
    if args.cldr_common:
        if not args.cldr_version or not re.fullmatch(r"[0-9]+(?:\.[0-9]+)*", args.cldr_version):
            parser.error("--cldr-common requires a numeric --cldr-version")
        data = {"source": f"CLDR {args.cldr_version} annotations",
                "locales": read_cldr(args.cldr_common)}
    else:
        with gzip.open(args.source, "rt", encoding="utf-8") as stream:
            data = json.load(stream)
    locales = data["locales"]
    if not locales or "en" not in locales or not locales["en"]:
        raise ValueError("English annotation fallback is required")
    generate(locales, data["source"], args.output)
    if args.cldr_common:
        encoded = json.dumps(data, ensure_ascii=False, separators=(",", ":")).encode()
        args.source.write_bytes(gzip.compress(encoded, mtime=0))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
