"""Validate deterministic emoji data generation and strict offline import contracts."""

import gzip
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/unicode"))
import generate_emoji_annotations as annotations
import generate_emoji_sequences as sequences


class EmojiDataTest(unittest.TestCase):
    """Exercise the bundled repertoire, official-file validators, and CLDR importer."""

    def test_snapshot(self) -> None:
        """Check published repertoire counts and both generated tables byte-for-byte."""
        catalog = sequences.read_catalog(sequences.CATALOG)
        bases = sequences.read_bases(sequences.DATA / "presentation-bases.txt")
        self.assertEqual(len(catalog), 3944)
        self.assertEqual(len(bases), 371)
        self.assertEqual(len(sequences.selector_omissions(catalog)), 1029 + 243)
        self.assertEqual(len(sequences.build_aliases(catalog, bases)), 1819)
        with gzip.open(sequences.DATA / "annotations.json.gz", "rt", encoding="utf-8") as stream:
            data = json.load(stream)
        self.assertEqual(len(data["locales"]), 24)
        report = annotations.coverage(data["locales"], catalog)
        expected = json.loads((sequences.DATA / "coverage.json").read_text())
        self.assertEqual(report, expected["locales"])
        self.assertEqual(report["en"], {"direct": 1914, "tone_base": 1790, "missing": 240})
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "generated.h"
            sequences.generate(bases, catalog, output)
            self.assertEqual(output.read_bytes(), sequences.OUTPUT.read_bytes())
            annotations.generate(data["locales"], data["source"], output)
            reference = ROOT / "native/unicode/generated/emoji-annotations.h"
            self.assertEqual(output.read_bytes(), reference.read_bytes())

    def test_presentation_import(self) -> None:
        """Require a complete and nonduplicated pair, with a matching style designation."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "emoji-variation-sequences.txt"
            valid = "231A FE0E ; text style; # watch\n231A FE0F ; emoji style; # watch\n"
            path.write_text(valid)
            self.assertEqual(sequences.read_official(path), [0x231A])
            for invalid in (valid.splitlines()[0], valid + valid,
                            valid.replace("emoji style", "text style")):
                path.write_text(invalid)
                with self.assertRaises(ValueError):
                    sequences.read_official(path)

    def test_qualification_import(self) -> None:
        """Compare exact official identities, not just their published counts."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "emoji-test.txt"
            valid = ("00A9 FE0F ; fully-qualified # copyright\n"
                     "00A9 ; unqualified # copyright\n")
            catalog = [(0xA9, 0xFE0F)]
            path.write_text(valid)
            sequences.verify_emoji_test(path, catalog)
            path.write_text(valid.replace("00A9 ;", "00AE ;"))
            with self.assertRaises(ValueError):
                sequences.verify_emoji_test(path, catalog)

    def test_cldr_import(self) -> None:
        """Merge derived keys and selectors, skip inherited/alternate records, report gaps."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("annotations", "annotationsDerived"):
                (root / name).mkdir()
            (root / "annotations/en.xml").write_text(
                '<ldml><annotations><annotation cp="©">copyright | legal</annotation>'
                '<annotation cp="©" type="tts">copyright</annotation>'
                '<annotation cp="©" alt="variant">ignored</annotation>'
                '<annotation cp="👍">approval</annotation></annotations></ldml>',
                encoding="utf-8")
            (root / "annotationsDerived/en.xml").write_text(
                '<ldml><annotations><annotation cp="©️">rights</annotation>'
                '<annotation cp="😀">↑↑↑</annotation></annotations></ldml>', encoding="utf-8")
            locales = annotations.read_cldr(root)
            report = annotations.coverage(locales, [(0xA9, 0xFE0F), (0x1F44D, 0x1F3FB),
                                                    (0x1F600,)])
            self.assertEqual(report["en"], {"direct": 1, "tone_base": 1, "missing": 1})
            output = root / "result.h"
            annotations.generate(locales, "fixture", output)
            source = output.read_text()
            self.assertIn("copyright | legal | rights", source)
            self.assertNotIn("ignored", source)
            self.assertNotIn("↑↑↑", source)
            self.assertEqual(len(locales["en"]), 2)

    def test_invalid_annotation(self) -> None:
        """Reject invalid scalar values and invalid locale tags rather than emitting C code."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "result.h"
            for locales in ({"en": [[[0xD800], "bad", []]]},
                            {"en-bad!": [[[0xA9], "bad", []]]},
                            {"en": [[[0xA9], "bad\x00", []]]}):
                with self.assertRaises(ValueError):
                    annotations.generate(locales, "fixture", path)
                self.assertFalse(path.exists())


if __name__ == "__main__":
    unittest.main()
