"""Generate immutable Unihan and language lookup tables from offline data."""

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import unicodedata
from zipfile import ZipFile

from algorithm_data import han_fold


FIELDS = ('kMandarin', 'kCantonese', 'kJapaneseOn', 'kJapaneseKun', 'kJapanese',
          'kKorean', 'kHangul', 'kDefinition', 'kRSUnicode', 'kTotalStrokes',
          'Mandarin_Fallback', 'Hangul_Fallback')
VERSION = '17.0.0'
# FreeBSD textproc/UCD distinfo for the original Unicode 17 release.
OFFICIAL_SHA256 = 'f7a48b2b545acfaa77b2d607ae28747404ce02baefee16396c5d2d7a8ef34b5e'


def fold(value: str) -> str:
    """Make reading searches case- and tone-mark-insensitive without altering display."""
    return han_fold(value)


def c_string(value: str) -> str:
    """Encode UTF-8 text as a safe C string literal."""
    if any(ord(c) < 32 or 0xD800 <= ord(c) <= 0xDFFF for c in value):
        raise ValueError('Control character or surrogate in generated string')
    return json.dumps(value, ensure_ascii=False)


def read_official(directory: Path) -> dict[int, dict[str, str]]:
    """Load all supported official properties, rejecting mixed versions and duplicates."""
    paths = sorted(directory.glob('Unihan_*.txt'))
    if not paths:
        raise ValueError('No extracted Unihan data files')
    return read_official_sources({path.name: path.read_text(encoding='utf-8')
                                  for path in paths})


def read_official_archive(path: Path) -> dict[int, dict[str, str]]:
    """Verify the pinned release archive before parsing its original text members."""
    with path.open('rb') as stream:
        if hashlib.file_digest(stream, 'sha256').hexdigest() != OFFICIAL_SHA256:
            raise ValueError('Unihan archive does not match the pinned Unicode 17 SHA-256')
    with ZipFile(path) as archive:
        names = archive.namelist()
        sources = {name: archive.read(name).decode('utf-8-sig')
                   for name in names if re.fullmatch(r'Unihan_\w+\.txt', name)}
        if len(set(names)) != len(names) or not sources:
            raise ValueError('Missing or duplicate Unihan archive members')
        return read_official_sources(sources)


def read_official_sources(sources: dict[str, str]) -> dict[int, dict[str, str]]:
    """Parse supported formal fields from one consistently versioned release."""
    records = {}
    seen = set()
    for name, text in sorted(sources.items()):
        versions = re.findall(r'^#\s*Unicode version\s*:?\s*(\d+\.\d+\.\d+)\s*$',
                              text, re.MULTILINE | re.IGNORECASE)
        if versions != [VERSION]:
            raise ValueError(f'Incorrect or missing Unicode version: {name}')
        for line in text.splitlines():
            if not line or line.startswith('#'):
                continue
            code, field, value = line.split('\t', 2)
            if field not in FIELDS[:10]:
                continue
            if not re.fullmatch(r'U\+[0-9A-F]{4,6}', code) or not value:
                raise ValueError(f'Invalid Unihan record: {line}')
            point = int(code[2:], 16)
            if point > 0x10FFFF or 0xD800 <= point <= 0xDFFF:
                raise ValueError(f'Invalid Unicode scalar: {code}')
            values = records.setdefault(point, {})
            if field in values:
                raise ValueError(f'Duplicate {field} for {code}')
            if field == 'kRSUnicode' and any(
                    not re.fullmatch(r"(?:[1-9]\d{0,2})'{0,3}\.-?\d+", token)
                    or int(re.match(r'\d+', token)[0]) > 214
                    for token in value.split()):
                raise ValueError(f'Invalid radical-stroke value: {value}')
            if field == 'kTotalStrokes' and any(
                    not token.isdecimal() or not 1 <= int(token) <= 128
                    for token in value.split()):
                raise ValueError(f'Invalid stroke count: {value}')
            c_string(value)
            values[field] = value
            seen.add(field)
    required = {'kMandarin', 'kCantonese', 'kKorean', 'kDefinition', 'kRSUnicode',
                'kTotalStrokes'}
    if not required <= seen or not seen & {'kJapanese', 'kJapaneseOn', 'kJapaneseKun'}:
        raise ValueError('Incomplete Unihan input: required property files are absent')
    return records


def generate(data: dict, official: dict[int, dict[str, str]], output: Path) -> None:
    """Deduplicate strings and write numeric scalar records with independent provenance."""
    records = {point: ({'Mandarin_Fallback': mandarin, 'Hangul_Fallback': hangul},
                       radical, strokes)
               for point, mandarin, hangul, radical, strokes in data['records']}
    for point, properties in official.items():
        values, radical, strokes = records.setdefault(point, ({}, 0, 0))
        values.update(properties)
    pool = {'': 0}
    size = 1

    def offset(value: str) -> int:
        """Intern an immutable UTF-8 string and return its byte offset."""
        nonlocal size
        if value not in pool:
            pool[value] = size
            size += len(value.encode('utf-8')) + 1
        return pool[value]

    rows = []
    stroke_groups = set()
    for point, (values, radical, strokes) in sorted(records.items()):
        original = [values.get(field, '') for field in FIELDS]
        if values.get('kTotalStrokes'):
            stroke_groups.update(int(value) for value in values['kTotalStrokes'].split())
        elif strokes:
            stroke_groups.add(strokes)
        offsets = [offset(value) for value in original]
        folded = [offset(fold(value)) for value in original]
        rows.append(f'    {{ 0x{point:X}, {radical}, {strokes}, '
                    f'{{ {", ".join(map(str, offsets))} }},\n'
                    f'      {{ {", ".join(map(str, folded))} }} }},\n')
    with output.open('w', encoding='utf-8', newline='\n') as stream:
        stream.write('/* Generated offline data; see docs/international-browsing.md. */\n')
        stream.write(f'#define UNIHAN_OFFICIAL_VERSION "{VERSION if official else "none"}"\n')
        stream.write('static const char han_strings[] =\n')
        for value in pool:
            stream.write(f'    {c_string(value)} "\\0"\n')
        stream.write('    ;\nstatic const struct HanRecord han_records[] = {\n')
        stream.writelines(rows)
        stream.write('};\n')
        stream.write('static const char *const radical_labels[] = {\n')
        for index in range(214):
            radical = unicodedata.normalize('NFKC', chr(0x2F00 + index))
            stream.write(f'    "{index + 1:03d} {radical}",\n')
        stream.write('};\nstatic const char *const stroke_labels[] = {\n')
        for index in sorted(stroke_groups):
            stream.write(f'    "{index}",\n')
        stream.write('};\nstatic const unsigned stroke_values[] = {\n')
        stream.write('    ' + ', '.join(map(str, sorted(stroke_groups))) + ',\n};\n')
    print(f'{len(records)} Han records; {len(official)} official records; {size} string bytes')


def generate_languages(data: dict, output: Path) -> None:
    """Write language labels and XKB registry mappings, preserving stable source IDs."""
    with output.open('w', encoding='utf-8', newline='\n') as stream:
        stream.write('/* Generated from ICU 76 and XKB registry; see international-browsing.md. */\n')
        stream.write('static const struct LanguageEntry language_entries[] = {\n')
        for tag, label in sorted(data['languages'].items(), key=lambda x: (x[1].casefold(), x[0])):
            stream.write(f'    {{ {c_string(tag)}, N_({c_string(label)}) }},\n')
        stream.write('};\nstatic const struct LayoutEntry layout_entries[] = {\n')
        for layout, codes in sorted(data['layouts'].items()):
            if codes:
                stream.write(f'    {{ {c_string(layout)}, {c_string(" ".join(codes))} }},\n')
        stream.write('};\n')


def main() -> int:
    """Regenerate both checked-in headers from pinned local source data."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('fallback', type=Path)
    parser.add_argument('output', type=Path)
    source = parser.add_mutually_exclusive_group()
    source.add_argument('--official', type=Path)
    source.add_argument('--official-archive', type=Path)
    args = parser.parse_args()
    data = json.loads(gzip.decompress(args.fallback.read_bytes()))
    bundled = args.fallback.parent / 'official-unihan.zip'
    if args.official:
        official = read_official(args.official)
    elif args.official_archive or bundled.is_file():
        official = read_official_archive(args.official_archive or bundled)
    else:
        official = {}
    generate(data, official, args.output / 'unihan.h')
    generate_languages(data, args.output / 'languages.h')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
