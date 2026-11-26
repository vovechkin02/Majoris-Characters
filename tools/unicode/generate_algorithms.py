"""Generate Unicode 17 grapheme and folding tables from pinned Unicode sources.

The existing property snapshot uses regex 2026.5.9; ICU 76 supplies stable
Unicode 16 normalization and simple case data. Unicode 17 adds 28 case pairs
and one compatibility decomposition. The default (non-Turkic) fold is used.
"""

import argparse
import ctypes as ct
import importlib.metadata
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LIMIT = 0x110000
UTF16 = 'utf-16-le' if sys.byteorder == 'little' else 'utf-16-be'
BREAKS = ('Other', 'CR', 'LF', 'Control', 'Extend', 'ZWJ', 'Regional_Indicator',
          'Prepend', 'SpacingMark', 'L', 'V', 'T', 'LV', 'LVT')
CONJUNCTS = ('None', 'Extend', 'Consonant', 'Linker')


def intervals(regex, text: str, expression: str):
    """Yield inclusive intervals for an exact Unicode property expression."""
    for match in regex.finditer(r'\p{' + expression + r'}+', text):
        yield match.start(), match.end() - 1


def runs(values: bytearray):
    """Yield maximal nonzero runs in numeric order."""
    start = 0
    for end in range(1, len(values) + 1):
        if end == len(values) or values[end] != values[start]:
            if values[start]:
                yield [start, end - 1, values[start]]
            start = end


def generate() -> dict:
    """Build immutable data with explicit source versions and stable ordering."""
    import regex
    if importlib.metadata.version('regex') != '2026.5.9':
        raise ValueError('Expected regex 2026.5.9 (Unicode 17)')
    text = ''.join(map(chr, range(LIMIT)))
    breaks = bytearray(LIMIT)
    for index, value in enumerate(BREAKS[1:], 1):
        for start, end in intervals(regex, text, 'gcb=' + value):
            breaks[start:end + 1] = bytes([index]) * (end - start + 1)
    for index, value in enumerate(CONJUNCTS[1:], 1):
        for start, end in intervals(regex, text, 'InCB=' + value):
            for point in range(start, end + 1):
                breaks[point] |= index << 4
    for start, end in intervals(regex, text, 'Extended_Pictographic'):
        for point in range(start, end + 1):
            breaks[point] |= 64
    library = ct.CDLL('libicuuc.so.76')
    version = (ct.c_uint8 * 4)()
    library.u_getUnicodeVersion_76(version)
    if tuple(version) != (16, 0, 0, 0):
        raise ValueError('Expected ICU Unicode 16.0')
    error = ct.c_int32()
    instance = library.unorm2_getNFKCCasefoldInstance_76
    instance.argtypes = [ct.POINTER(ct.c_int32)]
    instance.restype = ct.c_void_p
    normalizer = instance(ct.byref(error))
    normalize = library.unorm2_normalize_76
    normalize.argtypes = [ct.c_void_p, ct.POINTER(ct.c_uint16), ct.c_int32,
                          ct.POINTER(ct.c_uint16), ct.c_int32, ct.POINTER(ct.c_int32)]
    normalize.restype = ct.c_int32
    mapping = [library.u_toupper_76, library.u_tolower_76, library.u_totitle_76]
    for function in mapping:
        function.argtypes = [ct.c_int32]
        function.restype = ct.c_int32
    combining = library.u_getCombiningClass_76
    combining.argtypes = [ct.c_int32]
    combining.restype = ct.c_uint8
    ccc = bytearray(LIMIT)
    for value in range(1, 256):
        try:
            for start, end in intervals(regex, text, f'ccc={value}'):
                ccc[start:end + 1] = bytes([value]) * (end - start + 1)
        except regex.error:
            continue
    ignorable = bytearray(LIMIT)
    for start, end in intervals(regex, text, 'Default_Ignorable_Code_Point'):
        ignorable[start:end + 1] = b'\1' * (end - start + 1)
    # These pairs are the Unicode 17 changes to default full case folding.
    pairs = {0xA7CE: 0xA7CF, 0xA7D2: 0xA7D3, 0xA7D4: 0xA7D5}
    pairs.update({point: point + 0x1B for point in range(0x16EA0, 0x16EB9)})
    reverse = {lower: upper for upper, lower in pairs.items()}
    full, compatibility, simple, new_ccc = {}, {}, {}, []
    source = (ct.c_uint16 * 2)()
    output = (ct.c_uint16 * 64)()
    for point in range(LIMIT):
        if 0xD800 <= point <= 0xDFFF:
            continue
        character = chr(point)
        folded = regex._regex.fold_case(regex.FULLCASE | regex.IGNORECASE, character)
        # regex's matching-specific treatment of dotted/dotless I is not UCD folding.
        folded = {'I': 'i', 'İ': 'i\u0307'}.get(character, folded)
        if folded != character:
            full[str(point)] = list(map(ord, folded))
        raw = character.encode(UTF16)
        ct.memmove(source, raw, len(raw))
        error.value = 0
        length = normalize(normalizer, source, len(raw) // 2, output, 64,
                           ct.byref(error))
        if error.value > 0 or length < 0 or length > 64:
            raise ValueError(f'ICU mapping failed for {point:X}')
        normalized = bytes(output)[:length * 2].decode(UTF16)
        if ignorable[point]:
            normalized = ''
        elif point in pairs:
            normalized = chr(pairs[point])
        elif point == 0xA7F1:
            normalized = 's'
        if normalized != character:
            compatibility[str(point)] = list(map(ord, normalized))
        if point in pairs:
            case = [point, pairs[point], point]
        elif point in reverse:
            case = [reverse[point], point, reverse[point]]
        else:
            case = [function(point) for function in mapping]
        if case != [point] * 3:
            simple[str(point)] = case
        if ccc[point] != combining(point):
            new_ccc.append(point)
    return {'version': '17.0.0', 'regex': '2026.5.9', 'icu_base': '76.1',
            'grapheme': list(runs(breaks)), 'casefold': full,
            'nfkc_casefold': compatibility, 'simple_case': simple,
            'ccc': list(runs(ccc)), 'new_ccc': new_ccc,
            'latin': list(intervals(regex, text, 'Script=Latin')),
            'marks': list(intervals(regex, text, 'General_Category=Mark'))}


def write_header(data: dict, path: Path) -> None:
    """Emit immutable C tables with deduplicated mapping strings in scalar storage."""
    lines = ['/* Generated by tools/unicode/generate_algorithms.py; Unicode 17.0.0. */',
             'static const struct GraphemeRange grapheme_ranges[] = {']
    lines.extend(f'    {{ 0x{a:X}, 0x{b:X}, {v} }},' for a, b, v in data['grapheme'])
    lines.append('};')
    for key in ('casefold', 'nfkc_casefold'):
        pool = []
        offsets = {(): 0}
        rows = []
        for point, values in data[key].items():
            sequence = tuple(values)
            if sequence not in offsets:
                offsets[sequence] = len(pool)
                pool.extend(sequence)
            rows.append((int(point), offsets[sequence], len(sequence)))
        lines.append(f'static const gunichar {key}_points[] = {{')
        for index in range(0, len(pool), 12):
            lines.append('    ' + ', '.join(f'0x{p:X}' for p in pool[index:index + 12]) + ',')
        lines.extend(['};', f'static const struct FoldMapping {key}_mappings[] = {{'])
        lines.extend(f'    {{ 0x{p:X}, {o}, {n} }},' for p, o, n in rows)
        lines.append('};')
    lines.append('static const struct SimpleCase simple_cases[] = {')
    lines.extend(f'    {{ 0x{int(p):X}, 0x{u:X}, 0x{l:X}, 0x{t:X} }},'
                 for p, (u, l, t) in data['simple_case'].items())
    lines.append('};')
    path.write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main() -> int:
    """Regenerate the checked-in source snapshot and native header."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    args = parser.parse_args()
    data = generate()
    target = args.root / 'data/unicode/supplement/algorithms.json'
    target.write_text(json.dumps(data, separators=(',', ':')) + '\n', encoding='utf-8')
    write_header(data, args.root / 'native/unicode/generated/algorithms.h')
    print(f"Unicode {data['version']}: {len(data['grapheme'])} grapheme ranges, "
          f"{len(data['casefold'])} full and {len(data['nfkc_casefold'])} NFKC folds")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
