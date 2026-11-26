"""Validate release identity, resource wiring, and accessible template controls."""

import configparser
import contextlib
import io
import json
from pathlib import Path
import re
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
APP_ID = "com.majoris.characters"
sys.path.insert(0, str(ROOT / "tools/qualification"))
from qualify import expected_tests, verify_test_log

NAMED_CONTROLS = {
    "GtkButton", "GtkToggleButton", "GtkMenuButton", "GtkDropDown", "GtkSearchEntry",
    "GtkEntry", "GtkSpinButton", "GtkGridView", "GtkTextView", "GtkFlowBox", "GtkListBox",
    "GtkSwitch", "GtkScale", "GtkCheckButton", "GtkListView", "AdwSwitchRow", "AdwSpinRow",
}


def load_ini(path: Path) -> configparser.ConfigParser:
    """Parse freedesktop metadata with case-sensitive keys and no interpolation."""
    parser = configparser.ConfigParser(interpolation=None)
    parser.optionxform = str
    with path.open(encoding="utf-8") as stream:
        parser.read_file(stream)
    return parser


class SourceContractsTest(unittest.TestCase):
    """Keep independently installed metadata and actual source templates consistent."""

    def test_release_identity(self) -> None:
        """Match the build version, package version, IDs, launch actions and integration."""
        meson = (ROOT / "meson.build").read_text()
        version = re.search(r"\bversion: '([0-9.]+)'", meson).group(1)
        changelog = (ROOT / "debian/changelog").read_text().splitlines()[0]
        self.assertTrue(changelog.startswith(f"majoris-characters ({version}-"))
        desktop_dir = ROOT / "data/desktop"
        desktop = load_ini(desktop_dir / f"{APP_ID}.desktop")
        entry = desktop["Desktop Entry"]
        self.assertEqual(entry["Icon"], APP_ID)
        self.assertEqual(entry["Exec"], "majoris-characters")
        self.assertEqual(entry["DBusActivatable"], "true")
        self.assertIn("picker", entry["Actions"].split(";"))
        self.assertEqual(desktop["Desktop Action picker"]["Exec"],
                         "majoris-characters --picker")
        metainfo = ET.parse(desktop_dir / f"{APP_ID}.metainfo.xml").getroot()
        self.assertEqual(metainfo.findtext("id"), APP_ID)
        self.assertEqual(metainfo.findtext("launchable"), f"{APP_ID}.desktop")
        self.assertEqual(metainfo.find("releases/release").get("version"), version)
        provider = load_ini(desktop_dir / f"{APP_ID}.search-provider.ini")
        self.assertEqual(provider["Shell Search Provider"]["BusName"], APP_ID)
        self.assertEqual(provider["Shell Search Provider"]["ObjectPath"],
                         "/com/majoris/characters/SearchProvider")
        self.assertEqual(provider["Shell Search Provider"]["Version"], "2")
        service = load_ini(desktop_dir / f"{APP_ID}.service.in")
        self.assertEqual(service["D-BUS Service"]["Name"], "@APP_ID@")
        self.assertEqual(service["D-BUS Service"]["Exec"], "@EXECUTABLE@ --gapplication-service")
        metadata = json.loads((ROOT / "data/shell/metadata.json").read_text())
        self.assertIn("48", metadata["shell-version"])
        self.assertEqual(metadata["uuid"], "characters-picker@majoris.com")
        schema = ET.parse(ROOT / f"data/schemas/{APP_ID}.gschema.xml").getroot()
        self.assertEqual(schema.find("schema").get("id"), APP_ID)
        self.assertTrue((ROOT / "debian/rules").stat().st_mode & 0o111)
        self.assertTrue((ROOT / "tools/input/characters_input.py").stat().st_mode & 0o111)

    def test_resource_and_template_wiring(self) -> None:
        """Check each GtkTemplate resource, exact child ID/type, and duplicate IDs."""
        manifest = ET.parse(ROOT / "data/characters.gresource.xml").getroot()
        resources = {}
        for group in manifest.findall("gresource"):
            for entry in group.findall("file"):
                path = ROOT / "data" / entry.text
                self.assertTrue(path.is_file(), path)
                uri = group.get("prefix") + "/" + entry.get("alias", entry.text)
                self.assertNotIn(uri, resources)
                resources[uri] = path
        for source in sorted((ROOT / "src").rglob("*.vala")):
            text = source.read_text()
            resource = re.search(r'\[GtkTemplate\s*\(ui\s*=\s*"([^"]+)"', text)
            if resource is None:
                continue
            self.assertIn(resource.group(1), resources)
            template = ET.parse(resources[resource.group(1)]).getroot()
            objects = {}
            for obj in template.iter("object"):
                identifier = obj.get("id")
                if identifier is not None:
                    self.assertNotIn(identifier, objects, source)
                    objects[identifier] = obj.get("class")
            fields = re.findall(r'\[GtkChild\]\s+private\s+unowned\s+'
                                r'(\w+)\.(\w+)\s+(\w+)\s*;', text)
            self.assertEqual(len(fields), text.count("[GtkChild]"), source)
            for namespace, name, identifier in fields:
                self.assertEqual(objects.get(identifier), namespace + name,
                                 f"{source}: {identifier}")

    def test_accessible_controls_and_translation_inputs(self) -> None:
        """Require names for interactive XML controls and extraction of translated strings."""
        potfiles = (ROOT / "po/POTFILES.in").read_text().splitlines()
        for path in sorted((ROOT / "data/ui").glob("*.ui")):
            self.assertIn(path.relative_to(ROOT).as_posix(), potfiles)
            for obj in ET.parse(path).iter("object"):
                if obj.get("class") not in NAMED_CONTROLS:
                    continue
                label = obj.find("property[@name='label']")
                title = obj.find("property[@name='title']")
                accessible = obj.find("accessibility/property[@name='label']")
                self.assertTrue((label is not None and label.text) or
                                (title is not None and title.text) or
                                (accessible is not None and accessible.text),
                                f"{path}: {obj.get('id') or obj.get('class')}")
        for path in potfiles:
            if path and not path.startswith("#"):
                self.assertTrue((ROOT / path).is_file(), path)
        for locale in (ROOT / "po/LINGUAS").read_text().splitlines():
            if locale and not locale.startswith("#"):
                self.assertTrue((ROOT / f"po/{locale}.po").is_file(), locale)


    def test_localization_and_accessibility_wiring(self) -> None:
        """Check translated shortcut entry points and recycled item label cleanup."""
        for name in ("main-window", "compact-picker"):
            template = ET.parse(ROOT / f"data/ui/{name}.ui")
            self.assertTrue(any(node.text == "app.shortcuts" for node in template.iter()))
        application = (ROOT / "src/app/application.vala").read_text()
        self.assertIn('set_accels_for_action ("app.shortcuts"', application)
        dialog = (ROOT / "src/views/shortcuts-dialog.vala").read_text()
        for action in ("win.search", "win.open-composer", "app.picker", "app.quit",
                       "app.shortcuts"):
            self.assertIn(f'get_accels_for_action ("{action}")', dialog)
        for name in ("main-window", "font-browser", "library-dialog"):
            source = (ROOT / f"src/views/{name}.vala").read_text()
            self.assertIn("item.accessible_label =", source)
            self.assertIn("item.accessible_label = null;", source)
        inspector = (ROOT / "src/views/character-inspector.vala").read_text()
        self.assertIn("PropertyLabels.describe (keys[index], value)", inspector)
        self.assertIn("new Variant.string (value)", inspector)
        for locale in ("de", "es", "fr"):
            catalog = (ROOT / f"po/{locale}.po").read_text()
            for message in ("Keyboard Shortcuts", "Uppercase letter", "Right-to-left",
                            "Canonical", "Latin", "Hiragana", "Not assigned"):
                self.assertIn(f'msgid "{message}"', catalog)
            self.assertIn(f'"Language: {locale}\\n"', catalog)

    def test_vala_migration_wiring(self) -> None:
        """Require migrated production modules and tests without duplicate native policies."""
        native = (ROOT / "native/unicode/meson.build").read_text()
        sources = (ROOT / "src/meson.build").read_text()
        tests = (ROOT / "tests/meson.build").read_text()
        for name in ("character-inspector", "composition"):
            self.assertNotIn(f"'{name}.c'", native)
            self.assertFalse((ROOT / f"native/unicode/{name}.c").exists())
            self.assertIn(f"unit/test-{name}.vala", tests)
        for name in ("unicode/composition", "unicode/inspection-snapshot",
                     "unicode/scalar-sequence", "unicode/sequence-format",
                     "emoji/variation-family", "emoji/variants"):
            self.assertIn(f"'{name}.vala'", sources)
            self.assertTrue((ROOT / f"src/{name}.vala").is_file())
        self.assertIn("'icu-adapter.c'", native)
        self.assertIn("unit/test-icu-adapter.c", tests)
        self.assertNotIn("characters_emoji_variants_new",
                         (ROOT / "native/unicode/emoji-catalog.c").read_text())
        attributes = (ROOT / ".gitattributes").read_text()
        self.assertIn("native/unicode/generated/** linguist-generated=true", attributes)
        self.assertIn("native/unicode/vendor/** linguist-vendored=true", attributes)

    def test_second_vala_migration_wiring(self) -> None:
        """Keep one query parser and relationship policy, with all fixture targets wired."""
        sources = (ROOT / "src/meson.build").read_text()
        tests = (ROOT / "tests/meson.build").read_text()
        for name in ("search/query-parser", "search/language-selection",
                     "unicode/relationships"):
            self.assertIn(f"'{name}.vala'", sources)
            self.assertTrue((ROOT / f"src/{name}.vala").is_file())
        request = (ROOT / "src/search/search-request.vala").read_text()
        self.assertIn("QueryParser.prepare (job, query)", request)
        self.assertIn("LanguageSelection.configure (job, filter, locale, input_sources)", request)
        inspector = (ROOT / "src/unicode/inspection-snapshot.vala").read_text()
        self.assertIn("QueryParser.decode_code (code)", inspector)
        self.assertIn("Relationships.collect (points)", inspector)
        scanner = (ROOT / "native/unicode/character-search.c").read_text()
        self.assertNotIn("static bool parse_query", scanner)
        self.assertNotIn("characters_search_decode_code", scanner)
        self.assertIn("characters_search_prepare", scanner)
        engine = (ROOT / "native/unicode/unicode-engine.c").read_text()
        self.assertNotIn("characters_unicode_related(", engine)
        self.assertIn("characters_unicode_related_scalars", engine)
        self.assertIn("unit/test-prepared-search.c", tests)
        self.assertIn("support/search-contract-driver.vala", tests)
        self.assertIn("language_vapi", sources)
        for name in ("character-search", "legacy-encoding", "emoji-catalog"):
            fixture = (ROOT / f"tests/unit/test-{name}.c").read_text()
            self.assertIn("characters_query_parser_create", fixture)
            self.assertIn("characters_run_contract_tests", fixture)

    def test_third_vala_migration_wiring(self) -> None:
        """Require Vala semantic and Han policies at the prepared native boundary."""
        sources = (ROOT / "src/meson.build").read_text()
        request = (ROOT / "src/search/search-request.vala").read_text()
        parser = (ROOT / "src/search/query-parser.vala").read_text()
        for name in ("search/semantic-filters", "search/han-query"):
            self.assertIn(f"'{name}.vala'", sources)
            self.assertTrue((ROOT / f"src/{name}.vala").is_file())
        self.assertIn("han_vapi", sources)
        self.assertIn("SemanticFilters.configure (job, scope, filter)", request)
        self.assertIn("SemanticFilters.configure (job, scope, filter)", parser)
        self.assertIn("HanQuery.prepare (job,", parser)
        scanner = (ROOT / "native/unicode/character-search.c").read_text()
        self.assertIn("characters_search_set_rules", scanner)
        self.assertNotIn("symbol_ranges", scanner)
        self.assertNotIn("symbol_matches(", scanner)
        han = (ROOT / "native/unicode/unihan.c").read_text()
        self.assertNotIn("prefix_index(", han)
        self.assertNotIn('"mandarin:"', han)
        self.assertIn("characters_unihan_match_fields", han)

    def test_fourth_vala_migration_wiring(self) -> None:
        """Keep legacy notation and round-trip policy on the Vala side."""
        sources = (ROOT / "src/meson.build").read_text()
        tests = (ROOT / "tests/meson.build").read_text()
        workflow = (ROOT / "src/encodings/legacy-workflow.vala").read_text()
        native = (ROOT / "native/unicode/legacy-encoding.c").read_text()
        binding = (ROOT / "vapi/characters-legacy.vapi").read_text()
        self.assertIn("encodings/legacy-workflow.vala", sources)
        self.assertIn("legacy_sources", tests)
        self.assertIn("unit/test-legacy-adapter.c", tests)
        self.assertIn("decode_raw (encoding, encoded)", workflow)
        self.assertIn("points[index] != decoded[index]", workflow)
        self.assertIn("table.keys (point)", workflow)
        for obsolete in ("parse_hex(", "format_bytes(", "characters_legacy_decode_sequence(",
                         "characters_legacy_encode_sequence("):
            self.assertNotIn(obsolete, native)
        self.assertIn("characters_legacy_decode_raw", binding)
        self.assertIn("characters_legacy_encode_raw", binding)

    def test_fifth_vala_migration_wiring(self) -> None:
        """Require Vala inventory policy and raw-index remapping at the native boundary."""
        sources = (ROOT / "src/meson.build").read_text()
        inventory = (ROOT / "src/fonts/font-inventory.vala").read_text()
        record = (ROOT / "src/fonts/font-face-record.vala").read_text()
        browser = (ROOT / "src/views/font-browser.vala").read_text()
        request = (ROOT / "src/fonts/font-source-request.vala").read_text()
        native = (ROOT / "native/fonts/font-catalog.c").read_text()
        self.assertIn("fonts/font-face-record.vala", sources)
        self.assertIn("ordered.sort (FontFaceRecord.compare)", inventory)
        self.assertIn("family.validate ()", record)
        self.assertIn("catalog->faces = listed;", native)
        self.assertNotIn("qsort(", native)
        self.assertNotIn("FcFontSetAdd(", native)
        self.assertIn("catalog.open_face (faces[index].native_index)", inventory)
        self.assertIn("catalog.matches (faces[index].native_index, font)", inventory)
        self.assertIn("catalog.comparisons (compare_point,", browser)
        self.assertIn("catalog.find_selection (previous_file,", browser)
        self.assertIn("inventory.open_face (index)", request)
        self.assertNotIn("get_catalog ()", browser + request)

    def test_native_adapter_consolidation(self) -> None:
        """Require one native conversion boundary and no exported raw symbol pointer."""
        unicode = ROOT / "native/unicode"
        adapter = (unicode / "icu-adapter.c").read_text()
        engine = (unicode / "unicode-engine.c").read_text()
        self.assertIn("characters_unicode_normalize(", adapter)
        self.assertIn("characters_unicode_encode(", adapter)
        self.assertNotIn("char *characters_unicode_normalize(", engine)
        self.assertNotIn("char *characters_unicode_encode(", engine)
        for name in ("character-search", "emoji-catalog"):
            source = (unicode / f"{name}.c").read_text()
            self.assertIn("characters_icu_decode_utf8(", source)
            self.assertNotIn("U8_NEXT(", source)
        legacy = (unicode / "legacy-encoding.c").read_text()
        self.assertIn("characters_icu_decode_utf16(", legacy)
        self.assertIn("characters_icu_encode_utf16(", legacy)
        for name in ("font-catalog", "font-face"):
            source = (ROOT / f"native/fonts/{name}.c").read_text()
            self.assertIn("characters_scalar_is_valid(", source)
            self.assertNotIn("static bool is_scalar(", source)
        header = (ROOT / "native/fonts/font-face.h").read_text()
        self.assertNotIn("characters_font_face_symbol_points(", header)
        self.assertIn("characters_font_face_symbol_code(", header)

    def test_release_gate(self) -> None:
        """Prove missing and skipped test records cannot pass the release log gate."""
        names = sorted(expected_tests())
        self.assertGreater(len(names), 0)
        with tempfile.TemporaryDirectory(prefix="characters-log-") as directory:
            path = Path(directory) / "testlog.json"
            rows = [{"name": f"majoris-characters:unit / {name}", "result": "OK"}
                    for name in names]
            path.write_text("\n".join(json.dumps(row) for row in rows))
            with contextlib.redirect_stdout(io.StringIO()):
                verify_test_log(path)
            path.write_text("\n".join(json.dumps(row) for row in rows[:-1]))
            with self.assertRaises(ValueError):
                verify_test_log(path)
            rows[-1]["result"] = "SKIP"
            path.write_text("\n".join(json.dumps(row) for row in rows))
            with self.assertRaises(ValueError):
                verify_test_log(path)


if __name__ == "__main__":
    unittest.main()
