"""Generate a deterministic native lookup table from local emoji annotation data."""

import argparse
import gzip
import json
from pathlib import Path
import re
import unicodedata
import xml.etree.ElementTree as ET

from generate_emoji_sequences import CATALOG, read_catalog


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
    if not (root / "annotations").is_dir():
        raise ValueError("CLDR common/annotations is required")
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
                text = key(list(map(ord, text)))
                if not text:
                    continue
                record = rows.setdefault(text, [list(map(ord, text)), "", []])
                if node.get("type") == "tts":
                    record[1] = value
                else:
                    record[2].extend(word.strip() for word in value.split("|") if word.strip())
    return {lang: list(rows.values()) for lang, rows in locales.items() if rows}


def coverage(locales: dict, catalog: list[tuple[int, ...]]) -> dict:
    """Count direct exact annotation keys and skin-tone-base fallback per locale."""
    report = {}
    for language, rows in sorted(locales.items()):
        keys = {key(points) for points, name, words in rows if name or words}
        direct = sum(key(list(points)) in keys for points in catalog)
        inherited_tone = sum(
            key(list(points)) not in keys and
            key([p for p in points if not 0x1F3FB <= p <= 0x1F3FF]) in keys
            for points in catalog
        )
        report[language] = {"direct": direct, "tone_base": inherited_tone,
                            "missing": len(catalog) - direct - inherited_tone}
    return report


def generate(locales: dict, source: str, output: Path) -> None:
    """Emit sorted, deduplicated lookup records with immutable locale tables."""
    lines = ["/* Generated emoji annotation tables; do not edit. */",
             f"#define EMOJI_ANNOTATION_SOURCE {quote(source)}"]
    if len({language.lower() for language in locales}) != len(locales):
        raise ValueError("Duplicate case-insensitive locale tag")
    for language, rows in sorted(locales.items()):
        if not re.fullmatch(r"[a-zA-Z0-9_]+", language):
            raise ValueError(f"Invalid locale: {language}")
        records = {}
        for points, name, keywords in rows:
            if any(not isinstance(point, int) or not 0 <= point <= 0x10FFFF or
                   0xD800 <= point <= 0xDFFF for point in points):
                raise ValueError("Invalid annotation scalar")
            sequence = key(points)
            if not sequence:
                continue
            words = {fold(value) for value in [name, *keywords] if value}
            if any(any(ord(char) < 32 for char in word) for word in words):
                raise ValueError("Control character in annotation text")
            if words:
                records.setdefault(sequence, set()).update(words)
        if not records:
            raise ValueError(f"Empty annotation locale: {language}")
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
    parser.add_argument("--coverage", type=Path)
    parser.add_argument("--require-complete", action="append", default=[], metavar="LOCALE")
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
    report = coverage(locales, read_catalog(CATALOG))
    for language in args.require_complete:
        if language not in report or report[language]["missing"]:
            raise ValueError(f"Incomplete direct or tone-base coverage: {language}")
    generate(locales, data["source"], args.output)
    if args.coverage:
        args.coverage.write_text(json.dumps({"source": data["source"],
                                             "locales": report}, indent=2) + "\n",
                                 encoding="utf-8")
    if args.cldr_common:
        encoded = json.dumps(data, ensure_ascii=False, separators=(",", ":")).encode()
        args.source.write_bytes(gzip.compress(encoded, mtime=0))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
