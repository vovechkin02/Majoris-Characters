"""Export a versioned offline CJK fallback from ICU 76, xpinyin, and XeTeXko.

This is not an export of official Unihan 17 properties. The radical and stroke
values are collation groups, not formal kRSUnicode or kTotalStrokes records.
"""

import argparse
from bisect import bisect_right
import ctypes as ct
import gzip
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET


class Icu:
    """Own ICU handles and typed bindings used only by the offline exporter."""

    def __init__(self):
        """Require the pinned ICU release and load its public APIs."""
        self.uc = ct.CDLL('libicuuc.so.76')
        self.i18n = ct.CDLL('libicui18n.so.76')
        self.handles = []
        self.open = self.bind(self.i18n, 'ucol_open', ct.c_void_p,
                              [ct.c_char_p, ct.POINTER(ct.c_int)])
        self.close = self.bind(self.i18n, 'ucol_close', None, [ct.c_void_p])
        self.key = self.bind(self.i18n, 'ucol_getSortKey', ct.c_int,
                             [ct.c_void_p, ct.c_void_p, ct.c_int, ct.c_void_p, ct.c_int])
        self.rules = self.bind(self.i18n, 'ucol_getRules', ct.c_void_p,
                               [ct.c_void_p, ct.POINTER(ct.c_int)])
        self.age = self.bind(self.uc, 'u_charAge', None, [ct.c_int, ct.c_void_p])
        self.count = self.bind(self.uc, 'uloc_countAvailable', ct.c_int, [])
        self.locale = self.bind(self.uc, 'uloc_getAvailable', ct.c_char_p, [ct.c_int])
        self.display = self.bind(self.uc, 'uloc_getDisplayName', ct.c_int,
                                 [ct.c_char_p, ct.c_char_p, ct.c_void_p, ct.c_int,
                                  ct.POINTER(ct.c_int)])

    @staticmethod
    def bind(library, name, result, arguments):
        """Return a typed ICU 76 symbol; never guess a versioned ABI."""
        function = getattr(library, name + '_76')
        function.restype = result
        function.argtypes = arguments
        return function

    def collator(self, locale: str):
        """Open a collator, rejecting missing-resource/default-locale fallback."""
        status = ct.c_int()
        handle = self.open(locale.encode(), ct.byref(status))
        if not handle or status.value != 0:
            if handle:
                self.close(handle)
            raise ValueError(f'ICU collator {locale}: {status.value}')
        self.handles.append(handle)
        return handle

    def primary(self, handle, text: str) -> bytes:
        """Return the primary collation key without secondary levels."""
        encoded = text.encode('utf-16-le')
        output = ct.create_string_buffer(64)
        length = self.key(handle, encoded, len(encoded) // 2, output, len(output))
        if not 0 < length <= len(output):
            raise ValueError('Unexpected collation key length')
        return output.raw[:length].split(b'\x01')[0]

    def tailoring(self, handle) -> str:
        """Copy the public collator's UTF-16 tailoring rules."""
        length = ct.c_int()
        pointer = self.rules(handle, ct.byref(length))
        return ct.string_at(pointer, length.value * 2).decode('utf-16-le')

    def is_assigned(self, point: int) -> bool:
        """Reject Unicode points unknown to this version of ICU."""
        version = (ct.c_uint8 * 4)()
        self.age(point, version)
        return bool(version[0])

    def language_labels(self) -> dict[str, str]:
        """Enumerate language and script-specific locale labels, without regions."""
        tags = set()
        for index in range(self.count()):
            pieces = self.locale(index).decode().split('_')
            tags.add(pieces[0])
            if len(pieces) > 1 and len(pieces[1]) == 4:
                tags.add('_'.join(pieces[:2]))
        labels = {}
        for tag in sorted(tags - {'root', 'und'}):
            status = ct.c_int()
            buffer = ct.create_string_buffer(512)
            length = self.display(tag.encode(), b'en', buffer, 256, ct.byref(status))
            if status.value <= 0 and 0 < length < 256:
                labels[tag] = buffer.raw[:length * 2].decode('utf-16-le')
        return labels

    def release(self):
        """Release all opened collators, including on failed export."""
        for handle in self.handles:
            self.close(handle)
        self.handles.clear()


def read_mandarin(path: Path) -> dict[int, str]:
    """Read xpinyin's Unihan 14 primary and alternate readings, preserving tones."""
    records = {}
    for match in re.finditer(r'^\\XPYU(M?)\{(.)\}\{(\d+)\}\{([^{}]+)\}',
                             path.read_text(), re.MULTILINE):
        alternate, character, number, values = match.groups()
        point = int(number)
        if ord(character) != point:
            raise ValueError('Mismatched xpinyin identity')
        readings = records.setdefault(point, [])
        for value in values.split(','):
            if value not in readings:
                readings.append(value)
    return {point: ' '.join(values) for point, values in records.items()}


def read_korean(directory: Path) -> dict[int, str]:
    """Read XeTeXko Hanja pronunciation tables at their declared range offsets."""
    result = {}
    for name, start, length in [('hanjaexa_hangul.tab', 0x3400, 6582),
                                ('hanja_hangul.tab', 0x4E00, 20902),
                                ('hanjacom_hangul.tab', 0xF900, 302)]:
        values = [int(value) for value in (directory / name).read_text().split()]
        if len(values) != length or any(not 0xAC00 <= value <= 0xD7A3 for value in values):
            raise ValueError(f'Invalid XeTeXko table: {name}')
        result.update((start + index, chr(value)) for index, value in enumerate(values))
    return result


def stroke_groups(icu: Icu) -> dict[int, int]:
    """Export only explicit CLDR stroke-tailoring entries, excluding reset rules."""
    rules = icu.tailoring(icu.collator('zh@collation=stroke'))
    markers = list(re.finditer("<'\\ufdd0(.)'", rules))
    if len(markers) < 30:
        raise ValueError('Missing stroke index markers')
    result = {}
    for index, marker in enumerate(markers):
        end = markers[index + 1].start() if index + 1 < len(markers) else len(rules)
        group = ord(marker[1]) - 0x2800
        text = rules[marker.end():end].split('&', 1)[0]
        if not 1 <= group <= 128 or any(ord(c) < 128 and c not in '<*' for c in text):
            raise ValueError('Unexpected stroke collation syntax')
        for character in text:
            if character not in '<*':
                result[ord(character)] = group
    return result


def read_layouts(path: Path) -> dict[str, list[str]]:
    """Map configured XKB layout and variant IDs to registry ISO language codes."""
    root = ET.parse(path).getroot()
    result = {}
    for layout in root.findall('layoutList/layout'):
        base = layout.find('configItem')
        name = base.findtext('name')
        languages = [node.text for node in base.findall('languageList/iso639Id')]
        result[name] = languages
        for variant in layout.findall('variantList/variant/configItem'):
            variant_id = name + '+' + variant.findtext('name')
            codes = [node.text for node in variant.findall('languageList/iso639Id')]
            result[variant_id] = codes or languages
    return result


def export(icu: Icu, xpinyin: Path, korean: Path, xkb: Path) -> dict:
    """Build deterministic fallback records and language mappings from local sources."""
    import regex

    mandarin = read_mandarin(xpinyin)
    hangul = read_korean(korean)
    strokes = stroke_groups(icu)
    radical = icu.collator('zh@collation=unihan')
    # Canonical Kangxi radicals decompose into the index characters used by CLDR.
    import unicodedata

    keys = [icu.primary(radical, unicodedata.normalize('NFKC', chr(0x2F00 + index)))
            for index in range(214)]
    if not all(left < right for left, right in zip(keys, keys[1:])):
        raise ValueError('Unexpected radical index ordering')
    text = ''.join(map(chr, range(0x110000)))
    points = set(ord(match[0]) for match in regex.finditer(r'\p{Unified_Ideograph}', text))
    points.update(mandarin)
    points.update(hangul)
    points.update(strokes)
    records = []
    for point in sorted(points):
        if not icu.is_assigned(point):
            continue
        key = icu.primary(radical, chr(point))
        # Only the primary-weight range reserved for the explicit Han ordering.
        group = bisect_right(keys, key) if keys[0] <= key and key[0] <= keys[-1][0] else 0
        row = [point, mandarin.get(point, ''), hangul.get(point, ''), group,
               strokes.get(point, 0)]
        if any(row[1:]):
            records.append(row)
    return {'source': 'ICU 76.1 CLDR collation; xpinyin Unicode 14; XeTeXko 4.4',
            'records': records, 'languages': icu.language_labels(),
            'layouts': read_layouts(xkb)}


def main() -> int:
    """Write a reproducible compressed snapshot with independent source attribution."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('xpinyin', type=Path)
    parser.add_argument('korean', type=Path)
    parser.add_argument('xkb', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    icu = Icu()
    try:
        data = export(icu, args.xpinyin, args.korean, args.xkb)
    finally:
        icu.release()
    encoded = json.dumps(data, ensure_ascii=False, sort_keys=True,
                         separators=(',', ':')).encode()
    args.output.write_bytes(gzip.compress(encoded, mtime=0))
    print(f'{len(data["records"])} fallback records; {len(data["languages"])} languages; '
          f'{len(data["layouts"])} XKB layouts and variants')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
