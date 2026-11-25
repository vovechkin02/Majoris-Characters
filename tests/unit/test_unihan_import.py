"""Verify offline Han data import without distributing synthetic Unicode data."""

import tempfile
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools' / 'unicode'))
from generate_unihan import fold, generate, read_official


class UnihanImportTest(unittest.TestCase):
    """Exercise provenance, validation and generated table contracts."""

    def setUp(self) -> None:
        """Create a private fixture directory with representative property syntax."""
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.source = self.root / 'Unihan_Test.txt'
        self.records = (
            'U+6C34\tkMandarin\tshuǐ\nU+6C34\tkCantonese\tseoi2\n'
            'U+6C34\tkJapaneseKun\tMIZU\nU+6C34\tkKorean\tSWU\n'
            'U+6C34\tkDefinition\twater\nU+6C34\tkRSUnicode\t85.0 85\'.-1\n'
            'U+6C34\tkTotalStrokes\t4\n'
        )
        self.write_fixture(self.records)

    def write_fixture(self, records: str, version: str = '17.0.0') -> None:
        """Write isolated fixture text with a selectable version header."""
        self.source.write_text(f'# Unicode version: {version}\n' + records,
                               encoding='utf-8')

    def test_import_and_generation(self) -> None:
        """Keep formal values independent from available fallback data."""
        official = read_official(self.root)
        self.assertEqual(official[0x6C34]['kDefinition'], 'water')
        self.assertEqual(official[0x6C34]['kRSUnicode'], "85.0 85'.-1")
        output = self.root / 'unihan.h'
        data = {'records': [[0x6C34, 'shui', '수', 85, 4]]}
        generate(data, official, output)
        content = output.read_text(encoding='utf-8')
        self.assertIn('UNIHAN_OFFICIAL_VERSION "17.0.0"', content)
        self.assertIn('water', content)
        self.assertIn('shui', content)
        generate(data, {}, output)
        self.assertIn('UNIHAN_OFFICIAL_VERSION "none"', output.read_text())

    def test_invalid_imports(self) -> None:
        """Reject mixed releases, duplicates, invalid radicals and incomplete input."""
        cases = ((self.records, '16.0.0'),
                 (self.records + 'U+6C34\tkMandarin\tshui\n', '17.0.0'),
                 (self.records.replace('85.0', '215.0'), '17.0.0'),
                 (self.records.replace('kTotalStrokes\t4', 'kTotalStrokes\t0'), '17.0.0'),
                 ('U+6C34\tkMandarin\tshui\n', '17.0.0'))
        for records, version in cases:
            with self.subTest(records=records, version=version):
                self.write_fixture(records, version)
                with self.assertRaises(ValueError):
                    read_official(self.root)

    def test_reading_folding(self) -> None:
        """Ignore Latin tone marks but retain meaningful Japanese diacritics."""
        self.assertEqual(fold('SHUǏ'), 'shui')
        self.assertNotEqual(fold('lǜ'), fold('lu'))
        self.assertNotEqual(fold('が'), fold('か'))


if __name__ == '__main__':
    unittest.main()
