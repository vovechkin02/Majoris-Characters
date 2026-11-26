"""Use the checked-in Unicode 17 mappings for reproducible offline search indexes.

ICU 76 provides stable canonical decomposition/composition. The snapshot supplies
new combining classes, full default case folding and NFKC_CF mappings, avoiding
reliance on the host Python Unicode database version.
"""

from bisect import bisect_right
import ctypes as ct
from functools import lru_cache
import json
import sys
from pathlib import Path

SNAPSHOT = (Path(__file__).resolve().parents[2] /
            'data/unicode/supplement/algorithms.json')
UTF16 = 'utf-16-le' if sys.byteorder == 'little' else 'utf-16-be'


@lru_cache(maxsize=1)
def snapshot() -> dict:
    """Load the pinned source data once per generator process."""
    data = json.loads(SNAPSHOT.read_text(encoding='utf-8'))
    if data['version'] != '17.0.0':
        raise ValueError('Expected Unicode 17 algorithm data')
    return data


@lru_cache(maxsize=2)
def translation(key: str) -> dict[int, str]:
    """Return a Python translation table for the chosen full scalar mapping."""
    return {int(point): ''.join(map(chr, values))
            for point, values in snapshot()[key].items()}


@lru_cache(maxsize=1)
def normalizers():
    """Open the pinned ICU canonical normalizers and validate their Unicode version."""
    library = ct.CDLL('libicuuc.so.76')
    version = (ct.c_uint8 * 4)()
    library.u_getUnicodeVersion_76(version)
    if tuple(version) != (16, 0, 0, 0):
        raise ValueError('Expected ICU Unicode 16.0 for offline normalization')
    error = ct.c_int32()
    handles = {}
    for name in ('NFC', 'NFD'):
        function = getattr(library, f'unorm2_get{name}Instance_76')
        function.argtypes = [ct.POINTER(ct.c_int32)]
        function.restype = ct.c_void_p
        handles[name] = function(ct.byref(error))
        if error.value > 0 or not handles[name]:
            raise ValueError('ICU normalization initialization failed')
    normalize = library.unorm2_normalize_76
    normalize.argtypes = [ct.c_void_p, ct.POINTER(ct.c_uint16), ct.c_int32,
                          ct.POINTER(ct.c_uint16), ct.c_int32, ct.POINTER(ct.c_int32)]
    normalize.restype = ct.c_int32
    compose = library.unorm2_composePair_76
    compose.argtypes = [ct.c_void_p, ct.c_int32, ct.c_int32]
    compose.restype = ct.c_int32
    return library, handles, normalize, compose


def normalize_base(text: str, form: str) -> str:
    """Normalize with the stable ICU baseline without truncating UTF-16."""
    if text.isascii():
        return text
    _, handles, normalize, _ = normalizers()
    raw = text.encode(UTF16)
    length = len(raw) // 2
    source = (ct.c_uint16 * length).from_buffer_copy(raw)
    capacity = length * 4 + 1
    while True:
        output = (ct.c_uint16 * capacity)()
        error = ct.c_int32()
        count = normalize(handles[form], source, length, output, capacity, ct.byref(error))
        if error.value == 15 and count >= capacity:
            capacity = count + 1
            continue
        if error.value > 0 or count < 0:
            raise ValueError('ICU canonical normalization failed')
        return bytes(output)[:count * 2].decode(UTF16)


@lru_cache(maxsize=3)
def range_index(key: str):
    """Index immutable numeric intervals by their inclusive starting point."""
    rows = snapshot()[key]
    return [row[0] for row in rows], rows


def property_value(point: int, key: str) -> int:
    """Return a nonzero interval value or zero for the default property."""
    starts, rows = range_index(key)
    index = bisect_right(starts, point) - 1
    if index < 0 or point > rows[index][1]:
        return 0
    return rows[index][2] if len(rows[index]) == 3 else 1


@lru_cache(maxsize=1)
def new_combining_classes() -> frozenset:
    """Identify the scalar assignments whose combining class changed since ICU 76."""
    return frozenset(snapshot()['new_ccc'])


def normalize(text: str, form: str) -> str:
    """Apply Unicode 17 canonical ordering and composition to changed combining classes."""
    if text.isascii():
        return text
    if not any(ord(character) in new_combining_classes() for character in text):
        return normalize_base(text, form)
    points = list(map(ord, normalize_base(text, 'NFD')))
    ordered = []
    marks = []
    for point in points:
        if property_value(point, 'ccc'):
            marks.append(point)
        else:
            ordered.extend(sorted(marks, key=lambda item: property_value(item, 'ccc')))
            marks.clear()
            ordered.append(point)
    ordered.extend(sorted(marks, key=lambda item: property_value(item, 'ccc')))
    if form == 'NFD':
        return ''.join(map(chr, ordered))
    _, handles, _, compose = normalizers()
    result = []
    starter = -1
    previous = 0
    for point in ordered:
        combining = property_value(point, 'ccc')
        if starter >= 0 and (previous == 0 or previous < combining):
            composite = compose(handles['NFC'], result[starter], point)
            if composite >= 0:
                result[starter] = composite
                continue
        if combining == 0:
            starter = len(result)
        result.append(point)
        previous = combining
    return ''.join(map(chr, result))


@lru_cache(maxsize=65536)
def nfkc_casefold(text: str) -> str:
    """Use the same per-scalar NFKC_CF mapping and NFC as the native matcher."""
    return normalize(text.translate(translation('nfkc_casefold')), 'NFC')


@lru_cache(maxsize=65536)
def han_fold(text: str) -> str:
    """Fold readings and remove only the four designated Latin tone marks."""
    result = []
    latin = False
    for character in normalize(text.translate(translation('casefold')), 'NFD'):
        point = ord(character)
        if not property_value(point, 'marks'):
            latin = bool(property_value(point, 'latin'))
        if latin and character in '\u0300\u0301\u0304\u030c':
            continue
        result.append(character)
    return ''.join(result)
