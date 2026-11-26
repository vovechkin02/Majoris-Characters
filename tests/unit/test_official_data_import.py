"""Test the verified archive workflow without downloading or inventing release data."""

import gzip
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/unicode'))
import generate_emoji_annotations as emoji
import generate_unihan as han
import import_official_data as importer


class OfficialDataImportTest(unittest.TestCase):
    """Exercise successful staging, checksum rejection and locale inheritance."""

    def setUp(self) -> None:
        """Create a private miniature repository and synthetic input archives."""
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.data = self.root / 'data/unicode'
        self.generated = self.root / 'native/unicode/generated'
        for path in (self.data / 'han', self.data / 'emoji', self.generated,
                     self.root / 'native/unicode/vendor/gnome'):
            path.mkdir(parents=True)
        (self.generated / 'catalog-version.h').write_text(
            '#define CATALOG_VERSION "17.0.0"\n', encoding='utf-8')
        (self.root / 'native/unicode/vendor/gnome/emoji.h').write_text(
            '{ { 0x1F600 }, 1, "GRINNING FACE" },\n', encoding='utf-8')
        (self.data / 'han/fallback.json.gz').write_bytes(gzip.compress(
            json.dumps({'records': [[0x6C34, 'shui', '수', 85, 4]]}).encode(), mtime=0))
        (self.data / 'emoji/annotations.json.gz').write_bytes(gzip.compress(
            json.dumps({'source': 'fixture', 'locales': {'en': [], 'fr_CA': []}}).encode(),
            mtime=0))
        for name in ('unihan.h', 'emoji-annotations.h'):
            (self.generated / name).write_text('previous', encoding='utf-8')
        self.unihan = self.root / 'Unihan.zip'
        self.cldr = self.root / 'cldr-common-48.zip'
        fields = ('shuǐ', 'seoi2', 'SUI', 'MIZU', 'スイ', 'SWU', '수', 'water', '85.0', '4')
        with ZipFile(self.unihan, 'w') as archive:
            archive.writestr('Unihan_Readings.txt', '# Unicode Version 17.0.0\n' + ''.join(
                f'U+6C34\t{field}\t{value}\n' for field, value in zip(han.FIELDS, fields)))
        with ZipFile(self.cldr, 'w') as archive:
            archive.writestr('common/dtd/ldml.dtd',
                             '<!ATTLIST version cldrVersion CDATA #FIXED "48" >')
            archive.writestr('common/supplemental/supplementalData.xml',
                             '<supplementalData><parentLocales><parentLocale parent="fr" '
                             'locales="fr_CA"/></parentLocales></supplementalData>')
            archive.writestr('common/annotations/en.xml',
                             '<ldml><annotation cp="😀">grin</annotation></ldml>')
            archive.writestr('common/annotationsDerived/en.xml',
                             '<ldml><annotation cp="😀" type="tts">grinning face'
                             '</annotation></ldml>')
            archive.writestr('common/annotations/fr.xml',
                             '<ldml><annotation cp="😀">sourire</annotation></ldml>')
            archive.writestr('common/annotations/fr_CA.xml',
                             '<ldml><annotation cp="😀" type="tts">visage'
                             '</annotation></ldml>')
            archive.writestr('../../ignored', 'not extracted')

    def test_import_and_regenerate(self) -> None:
        """Publish both databases, preserve provenance and regenerate byte-identically."""
        with patch.object(han, 'OFFICIAL_SHA256', importer.digest(self.unihan)), \
                patch.object(importer, 'CLDR_SHA512', importer.digest(self.cldr, 'sha512')):
            importer.import_archives(self.root, self.unihan, self.cldr)
            manifest = json.loads((self.data / 'official-data.json').read_text())
            self.assertEqual(manifest['status'], 'imported')
            self.assertEqual(manifest['unihan']['records'], 1)
            self.assertEqual(manifest['cldr']['english_missing'], 0)
            for name, checksum in manifest['outputs_sha256'].items():
                self.assertEqual(importer.digest(self.root / name), checksum)
            self.assertFalse((self.root / 'ignored').exists())
            bundled = self.data / 'han/official-unihan.zip'
            official = han.read_official_archive(bundled)
            temporary = self.root / 'regenerated.h'
            han.generate(importer.load_json(self.data / 'han/fallback.json.gz'),
                         official, temporary)
            self.assertEqual(temporary.read_bytes(), (self.generated / 'unihan.h').read_bytes())
            snapshot = importer.load_json(self.data / 'emoji/annotations.json.gz')
            self.assertEqual(snapshot['locales']['fr_CA'], [[[0x1F600], 'visage', ['sourire']]])
            emoji.generate(snapshot['locales'], snapshot['source'], temporary)
            self.assertEqual(temporary.read_bytes(),
                             (self.generated / 'emoji-annotations.h').read_bytes())
            importer.import_archives(self.root, self.unihan, self.cldr)
            self.assertEqual(manifest, json.loads((self.data / 'official-data.json').read_text()))
            importer.verify_installed(self.root)
            (self.generated / 'unihan.h').write_text('changed', encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'output has changed'):
                importer.verify_installed(self.root)

    def test_rejected_archives_leave_previous_outputs(self) -> None:
        """Reject a mismatched Unihan or CLDR archive without partial publication."""
        with self.assertRaisesRegex(ValueError, 'Unihan archive'):
            importer.import_archives(self.root, self.unihan, self.cldr)
        with patch.object(han, 'OFFICIAL_SHA256', importer.digest(self.unihan)):
            with self.assertRaisesRegex(ValueError, 'CLDR archive'):
                importer.import_archives(self.root, self.unihan, self.cldr)
        self.assertFalse((self.data / 'official-data.json').exists())
        self.assertFalse((self.data / 'han/official-unihan.zip').exists())
        for name in ('unihan.h', 'emoji-annotations.h'):
            self.assertEqual((self.generated / name).read_text(), 'previous')

    def test_parent_override_and_marker(self) -> None:
        """Respect non-default parent locales, missing fields, and no-inheritance markers."""
        root = self.root / 'common'
        for name in ('annotations', 'annotationsDerived', 'supplemental'):
            (root / name).mkdir(parents=True)
        (root / 'supplemental/supplementalData.xml').write_text(
            '<supplementalData><parentLocales><parentLocale parent="es_419" locales="es_MX"/>'
            '<parentLocale parent="root" locales="zh_Hant"/></parentLocales>'
            '</supplementalData>', encoding='utf-8')
        for name, values in {
            'es': '<annotation cp="😀">base</annotation>',
            'es_419': '<annotation cp="😀">regional</annotation>',
            'es_MX': '<annotation cp="😀" type="tts">∅∅∅</annotation>',
            'zh': '<annotation cp="😀">simplified</annotation>',
            'zh_Hant': '<annotation cp="😀" type="tts">traditional</annotation>',
        }.items():
            (root / f'annotations/{name}.xml').write_text(f'<ldml>{values}</ldml>',
                                                       encoding='utf-8')
        rows = emoji.resolve_cldr(root, set())
        self.assertEqual(rows['es_MX'], [[[0x1F600], '', ['regional']]])
        self.assertEqual(rows['zh_Hant'], [[[0x1F600], 'traditional', []]])


class BundledOfficialDataTest(unittest.TestCase):
    """Qualify the shipped release data rather than only isolated fixtures."""

    def test_bundled_provenance_and_han_regeneration(self) -> None:
        """Verify the release hashes, complete field counts and deterministic Han output."""
        importer.verify_installed(ROOT)
        data = ROOT / 'data/unicode'
        manifest = json.loads((data / 'official-data.json').read_text(encoding='utf-8'))
        official = han.read_official_archive(data / 'han/official-unihan.zip')
        self.assertEqual(len(official), 102998)
        counts = {field: sum(field in row for row in official.values())
                  for field in han.FIELDS[:10]}
        self.assertEqual(counts, manifest['unihan']['property_counts'])
        self.assertTrue(all(counts.values()))
        self.assertEqual(official[0x6C34]['kDefinition'], 'water, liquid, lotion, juice')
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'unihan.h'
            han.generate(importer.load_json(data / 'han/fallback.json.gz'), official, output)
            self.assertEqual(importer.digest(output),
                             manifest['outputs_sha256']['native/unicode/generated/unihan.h'])


if __name__ == '__main__':
    unittest.main()
