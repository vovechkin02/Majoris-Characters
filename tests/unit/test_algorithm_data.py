"""Check the pinned algorithm snapshot and offline search folding policy."""

from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/unicode'))
from algorithm_data import han_fold, nfkc_casefold, normalize, snapshot, translation
from generate_algorithms import write_header


class AlgorithmDataTest(unittest.TestCase):
    """Ensure the runtime tables and offline index rules share the pinned data."""

    def test_snapshot(self) -> None:
        """Verify mapping sizes, ordered ranges, and deterministic header generation."""
        data = snapshot()
        self.assertEqual(data['version'], '17.0.0')
        self.assertEqual(len(data['casefold']), 1585)
        self.assertEqual(len(data['nfkc_casefold']), 10583)
        self.assertEqual(len(data['simple_case']), 2989)
        self.assertEqual(len(data['new_ccc']), 34)
        previous = -1
        for first, last, flags in data['grapheme']:
            self.assertLess(previous, first)
            self.assertLessEqual(first, last)
            self.assertGreater(flags, 0)
            self.assertLess(flags, 128)
            previous = last
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'algorithms.h'
            write_header(data, output)
            self.assertEqual(output.read_bytes(),
                             (ROOT / 'native/unicode/generated/algorithms.h').read_bytes())

    def test_default_folding(self) -> None:
        """Distinguish full case folding from compatibility and ignorable removal."""
        full = translation('casefold')
        self.assertEqual('Straße I İ ı'.translate(full), 'strasse i i\u0307 ı')
        self.assertEqual('\U00016EA0\uA7CE\uA7D2'.translate(full),
                         '\U00016EBB\uA7CF\uA7D3')
        self.assertEqual(nfkc_casefold('ＦＵＬＬ\uA7F1\u200D\uFE0F'), 'fulls')
        self.assertEqual(nfkc_casefold('E\u0301'), 'é')
        self.assertEqual(nfkc_casefold('\u200D'), '')

    def test_canonical_order_and_han(self) -> None:
        """Retain new combining classes and strip only designated Latin tone marks."""
        self.assertEqual(normalize('a\u0301\u1ADD', 'NFD'), 'a\u1ADD\u0301')
        self.assertEqual(normalize('a\u0301\u1ADD', 'NFC'), 'á\u1ADD')
        self.assertEqual(han_fold('SHUǏ'), 'shui')
        self.assertEqual(han_fold('lǜ'), 'lu\u0308')
        self.assertNotEqual(han_fold('が'), han_fold('か'))
        self.assertEqual(han_fold('\U00016EA0'), '\U00016EBB')


if __name__ == '__main__':
    unittest.main()
