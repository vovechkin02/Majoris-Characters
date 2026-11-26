"""Check gettext extraction and complete compiled-catalog coverage."""

import gettext
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/localization'))
from catalogs import PRINTF, inventory, literal_messages, template


class TranslationCatalogTest(unittest.TestCase):
    """Exercise literal extraction and standard gettext catalog contracts."""

    def test_literal_extraction(self) -> None:
        """Recognize C/Vala concatenation and ignore comments and dynamic names."""
        source = ('/* _("not a message") */\n'
                  '_("First " + "second"); N_("Third " "fourth");\n'
                  '_(dynamic_name); _("Line\\n");')
        self.assertEqual([value for value, _ in literal_messages(source)],
                         ['First second', 'Third fourth', 'Line\n'])

    def test_template_is_current(self) -> None:
        """Keep the translation template synchronized with the actual marked source."""
        self.assertEqual((ROOT / 'po/majoris-characters.pot').read_text(),
                         template(ROOT))

    def test_compiled_catalogs(self) -> None:
        """Require complete, non-fuzzy translations and unchanged printf operands."""
        messages = inventory(ROOT)
        languages = [line for line in (ROOT / 'po/LINGUAS').read_text().splitlines()
                     if line and not line.startswith('#')]
        with tempfile.TemporaryDirectory() as directory:
            for language in languages:
                with self.subTest(language=language):
                    compiled = Path(directory) / f'{language}.mo'
                    subprocess.run(['msgfmt', '--check', '--check-format',
                                    '-o', str(compiled),
                                    str(ROOT / f'po/{language}.po')], check=True)
                    with compiled.open('rb') as stream:
                        catalog = gettext.GNUTranslations(stream)
                    # GNUTranslations stores only active, nonempty, non-fuzzy entries.
                    self.assertEqual(set(catalog._catalog) - {''}, set(messages))
                    for message in messages:
                        translated = catalog.gettext(message)
                        self.assertTrue(translated)
                        self.assertEqual(PRINTF.findall(message),
                                         PRINTF.findall(translated), message)
                    self.assertEqual(catalog.gettext('U+10FFFF'), 'U+10FFFF')


if __name__ == '__main__':
    unittest.main()
