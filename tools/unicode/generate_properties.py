"""Generate the Unicode 17 scalar property snapshot without runtime Python dependencies.

The offline source is regex 2026.5.9 (Unicode 17), FontTools Unicode 17 scripts,
and ICU 76's Unicode 16 age data. Newly assigned scalars have age 17.0.
"""

import argparse
from array import array
import ctypes
import importlib.metadata
import json
from pathlib import Path


LIMIT = 0x110000
# In ICU UCharCategory order; the stable numeric contract is used by native search.
CATEGORIES = (
    'Cn Lu Ll Lt Lm Lo Mn Me Mc Nd Nl No Zs Zl Zp Cc Cf Co Cs '
    'Pd Ps Pe Pc Po Sm Sc Sk So Pi Pf'
).split()
PROPERTIES = (
    ('bc', 'L R AL EN ES ET AN CS NSM BN B S WS ON LRE LRO RLE RLO PDF LRI RLI FSI PDI'.split()),
    ('dt', 'None Can Com Enc Fin Font Fra Init Iso Med Nar Nb Sml Sqr Sub Sup Vert Wide'.split()),
    ('ea', 'N A H W F Na'.split()),
)
FLAGS = ('Bidi_Mirrored', 'Emoji', 'Emoji_Presentation', 'Emoji_Modifier',
         'Default_Ignorable_Code_Point', 'Unified_Ideograph')


def ranges(regex, text: str, prop: str, value: str = 'Yes'):
    """Yield maximal code-point ranges for a property value."""
    for match in regex.finditer(r'\p{' + prop + '=' + value + r'}+', text):
        yield match.start(), match.end()


def column(regex, text: str, prop: str, values: list[str]) -> bytearray:
    """Encode an enumerated property using the declared stable value order."""
    result = bytearray([255]) * LIMIT
    for index, value in enumerate(values):
        for start, end in ranges(regex, text, prop, value):
            result[start:end] = bytes([index]) * (end - start)
    if 255 in result:
        raise ValueError(f'Incomplete property: {prop}')
    return result


def age_column(categories: bytearray) -> bytearray:
    """Use ICU 76's age and mark only newly assigned Unicode 17 scalars as 17.0."""
    library = ctypes.CDLL('libicuuc.so.76')
    age = library.u_charAge_76
    age.argtypes = [ctypes.c_int32, ctypes.POINTER(ctypes.c_uint8)]
    age.restype = None
    version = library.u_getUnicodeVersion_76
    version.argtypes = [ctypes.POINTER(ctypes.c_uint8)]
    version.restype = None
    buffer = (ctypes.c_uint8 * 4)()
    version(buffer)
    if tuple(buffer) != (16, 0, 0, 0):
        raise ValueError('Expected ICU Unicode 16.0')
    result = bytearray(LIMIT)
    for point, category in enumerate(categories):
        if category == 0:
            continue
        age(point, buffer)
        result[point] = (buffer[0] or 17) * 10 + buffer[1]
    return result


def script_columns(scripts, extensions):
    """Encode primary and extension scripts with deterministic IDs and exact sets."""
    codes = sorted(scripts.NAMES)
    indexes = {code: index for index, code in enumerate(codes)}
    primary = bytearray(LIMIT)
    for index, start in enumerate(scripts.RANGES):
        end = scripts.RANGES[index + 1] if index + 1 < len(scripts.RANGES) else LIMIT
        primary[start:end] = bytes([indexes[scripts.VALUES[index]]]) * (end - start)
    sets = sorted({tuple(sorted(value)) for value in extensions.VALUES if value})
    set_ids = {value: index + 1 for index, value in enumerate(sets)}
    extended = array('H', [0]) * LIMIT
    for index, start in enumerate(extensions.RANGES):
        value = extensions.VALUES[index]
        if value:
            end = extensions.RANGES[index + 1] if index + 1 < len(extensions.RANGES) else LIMIT
            extended[start:end] = array('H', [set_ids[tuple(sorted(value))]]) * (end - start)
    return codes, primary, sets, extended


def generate() -> str:
    """Return a complete run-length encoded scalar property snapshot."""
    import regex
    from fontTools.unicodedata import Scripts, ScriptExtensions

    if importlib.metadata.version('regex') != '2026.5.9':
        raise ValueError('Expected regex 2026.5.9 (Unicode 17.0.0)')
    for source in (Scripts, ScriptExtensions):
        if '17.0.0.txt' not in Path(source.__file__).read_text(encoding='utf-8')[:1000]:
            raise ValueError('Expected Unicode 17.0.0 FontTools script data')
    text = ''.join(map(chr, range(LIMIT)))
    category = column(regex, text, 'gc', CATEGORIES)
    codes, primary, sets, extended = script_columns(Scripts, ScriptExtensions)
    age = age_column(category)
    if sum(value == 170 for value in age) != 4803:
        raise ValueError('Unexpected Unicode 17 assignment delta')
    if sum(value not in (0, 18) for value in category) != 297334:
        raise ValueError('Unexpected assigned scalar count')
    ccc = bytearray(LIMIT)
    for value in range(256):
        try:
            for start, end in ranges(regex, text, 'ccc', str(value)):
                ccc[start:end] = bytes([value]) * (end - start)
        except regex.error:
            continue
    flags = bytearray(LIMIT)
    for bit, prop in enumerate(FLAGS):
        for start, end in ranges(regex, text, prop):
            for point in range(start, end):
                flags[point] |= 1 << bit
    columns = [category, primary, extended, age, ccc]
    columns.extend(column(regex, text, prop, values) for prop, values in PROPERTIES)
    columns.append(flags)
    output = ['/* Generated Unicode 17 data; see docs/unicode-engine.md. */',
              '#define PROPERTY_VERSION "17.0.0"',
              'static const struct ScriptEntry script_entries[] = {']
    output.extend(f'    {{ "{code}", N_("{Scripts.NAMES[code]}") }},' for code in codes)
    output.extend(['};', 'static const char *const extension_sets[] = {', '    NULL,'])
    output.extend('    ' + json.dumps(' '.join(value)) + ',' for value in sets)
    output.append('};')
    for name, values in [('category', CATEGORIES), *PROPERTIES]:
        output.append(f'static const char *const {name}_names[] = {{')
        output.extend('    ' + json.dumps(value) + ',' for value in values)
        output.append('};')
    output.append('static const struct PropertyRange property_ranges[] = {')
    start = 0
    previous = None
    for point, row in enumerate(zip(*columns)):
        if row != previous:
            if previous is not None:
                values = ', '.join(map(str, previous))
                output.append(f'    {{ 0x{start:X}, 0x{point - 1:X}, {values} }},')
            start, previous = point, row
    values = ', '.join(map(str, previous))
    output.extend([f'    {{ 0x{start:X}, 0x{LIMIT - 1:X}, {values} }},', '};'])
    return '\n'.join(output) + '\n'


def main() -> int:
    """Generate the header atomically from the pinned offline dependencies."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    content = generate()
    temporary = args.output.with_suffix('.tmp')
    temporary.write_text(content, encoding='utf-8')
    temporary.replace(args.output)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
