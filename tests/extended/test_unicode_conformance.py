"""Run optional official UCD tests or reproducible Unicode 17 differential checks."""

import argparse
import gzip
import itertools
import json
from pathlib import Path
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/unicode'))
from algorithm_data import han_fold, nfkc_casefold, snapshot, translation


def hex_points(text: str) -> str:
    """Encode the exact scalar sequence for the native probe protocol."""
    return ' '.join(f'{ord(character):X}' for character in text)


def rows(path: Path, version_required: bool = True):
    """Read uncommented UCD rows, rejecting a different release header."""
    text = path.read_text(encoding='utf-8')
    if version_required and '17.0.0' not in text[:300]:
        raise ValueError(f'Expected Unicode 17 test input: {path}')
    for line in text.splitlines():
        value = line.partition('#')[0].strip()
        if value and not value.startswith('@'):
            yield value


def official_cases(root: Path):
    """Yield every normative grapheme, folding and normalization test row."""
    for row in rows(root / 'auxiliary/GraphemeBreakTest.txt'):
        parts = row.split()
        count = 0
        prefix = []
        for marker in parts[::2][:-1]:
            count += marker == '÷'
            prefix.append(str(count))
        yield 'G ' + ' '.join(parts[1::2]), ' '.join(prefix)
    for row in rows(root / 'CaseFolding.txt'):
        source, status, target = [part.strip() for part in row.split(';')[:3]]
        if status in ('C', 'F'):
            yield f'C {source}', ' '.join(f'{int(p, 16):X}' for p in target.split())
    for row in rows(root / 'DerivedNormalizationProps.txt'):
        fields = [part.strip() for part in row.split(';')]
        if len(fields) < 3 or fields[1] != 'NFKC_CF':
            continue
        ends = [int(point, 16) for point in fields[0].split('..')]
        target = ' '.join(f'{int(p, 16):X}' for p in fields[2].split())
        for point in range(ends[0], ends[-1] + 1):
            yield f'K {point:X}', target
    for row in rows(root / 'NormalizationTest.txt'):
        columns = [' '.join(f'{int(p, 16):X}' for p in field.split())
                   for field in row.split(';')[:5]]
        for index, value in enumerate(columns):
            yield f'N {value}', columns[1 if index < 3 else 3]
            yield f'D {value}', columns[2 if index < 3 else 4]
            yield f'P {value}', columns[3]
            yield f'Q {value}', columns[4]


def differential_cases():
    """Compare native boundaries with pinned regex and folds with the offline policy."""
    import importlib.metadata
    import regex
    if importlib.metadata.version('regex') != '2026.5.9':
        raise ValueError('Expected regex 2026.5.9')
    data = snapshot()
    representatives = {0: 0x61}
    for first, _, flags in data['grapheme']:
        if not 0xD800 <= first <= 0xDFFF:
            representatives.setdefault(flags, first)
    points = list(representatives.values())
    for sequence in itertools.product(points, repeat=3):
        text = ''.join(map(chr, sequence))
        yield 'G ' + hex_points(text), ' '.join(
            str(len(regex.findall(r'\X', text[:end]))) for end in range(1, 4))
    rng = random.Random(1700)
    # Sample all packed property combinations with longer context and real range endpoints.
    pool = points + [p for a, b, _ in data['grapheme'] for p in (a, b)
                     if not 0xD800 <= p <= 0xDFFF]
    for _ in range(10000):
        text = ''.join(chr(rng.choice(pool)) for _ in range(rng.randint(1, 16)))
        yield 'G ' + hex_points(text), ' '.join(
            str(len(regex.findall(r'\X', text[:end]))) for end in range(1, len(text) + 1))
    for key, mode in (('casefold', 'C'), ('nfkc_casefold', 'K')):
        for point, target in data[key].items():
            yield f'{mode} {int(point):X}', ' '.join(f'{p:X}' for p in target)
    # Actual annotation/reading values, sampled deterministically across locales.
    with gzip.open(ROOT / 'data/unicode/emoji/annotations.json.gz', 'rt') as stream:
        locales = json.load(stream)['locales']
    for values in locales.values():
        for _, name, keywords in rng.sample(values, min(100, len(values))):
            for word in [name, *keywords]:
                if word and len(word) <= 256 and '\0' not in word:
                    yield 'K ' + hex_points(word), hex_points(nfkc_casefold(word))
    for text in ('SHUǏ', 'lǜ', 'が', 'I İ ı', 'A\u1ADD\u0301', '\U00016EA0',
                 'a\u0301\u1ADD', '\uA7F1'):
        yield 'H ' + hex_points(text), hex_points(han_fold(text))
        yield 'C ' + hex_points(text), hex_points(text.translate(translation('casefold')))
        yield 'K ' + hex_points(text), hex_points(nfkc_casefold(text))


def run_cases(probe: Path, cases) -> int:
    """Compare batches without storing an unbounded reference corpus in memory."""
    total = 0
    iterator = iter(cases)
    while batch := list(itertools.islice(iterator, 2048)):
        result = subprocess.run([str(probe.resolve())], input='\n'.join(
            command for command, _ in batch) + '\n', text=True, capture_output=True,
            check=True, timeout=60).stdout.splitlines()
        if len(result) != len(batch):
            raise ValueError('Native probe returned the wrong number of rows')
        for (command, expected), actual in zip(batch, result):
            if actual != expected:
                raise AssertionError(f'{command}: expected {expected!r}, got {actual!r}')
        total += len(batch)
    return total


def main() -> int:
    """Run one selected qualification suite against a compiled probe."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('probe', type=Path)
    parser.add_argument('--ucd', type=Path, help='Extracted official Unicode 17 UCD directory')
    args = parser.parse_args()
    cases = official_cases(args.ucd) if args.ucd else differential_cases()
    print(f'{run_cases(args.probe, cases)} native conformance comparisons passed.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
