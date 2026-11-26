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
        plan = (ROOT / "src/search/search-plan.vala").read_text()
        self.assertIn("SearchPlan.prepare (job, scope, filter, query, locale, input_sources,", request)
        self.assertIn("QueryParser.prepare (job, query, locale)", plan)
        self.assertIn("LanguageSelection.configure (job, filter, locale, input_sources)", plan)
        inspector = (ROOT / "src/unicode/inspection-snapshot.vala").read_text()
        self.assertIn("QueryParser.decode_code (code)", inspector)
        self.assertIn("Relationships.collect (points)", inspector)
        scanner = (ROOT / "native/unicode/character-search.c").read_text()
        self.assertNotIn("static gboolean parse_query", scanner)
        self.assertNotIn("characters_search_decode_code", scanner)
        self.assertIn("characters_search_prepare", scanner)
        engine = (ROOT / "native/unicode/unicode-engine.c").read_text()
        self.assertNotIn("characters_unicode_related(", engine)
        self.assertIn("characters_unicode_confusables", engine)
        self.assertNotIn("characters_unicode_related_scalars", engine)
        self.assertIn("unit/test-prepared-search.c", tests)
        self.assertIn("support/search-contract-driver.vala", tests)
        self.assertIn("language_vapi", sources)
        for name in ("character-search", "legacy-encoding", "emoji-catalog"):
            fixture = (ROOT / f"tests/unit/test-{name}.c").read_text()
            self.assertIn("characters_query_parser_create", fixture)
            self.assertIn("characters_run_contract_tests", fixture)

    def test_relationship_migration_wiring(self) -> None:
        """Keep scalar relationship selection in Vala and raw mappings in the native layer."""
        policy = (ROOT / "src/unicode/relationships.vala").read_text()
        engine = (ROOT / "native/unicode/unicode-engine.c").read_text()
        icu = (ROOT / "native/unicode/icu-adapter.c").read_text()
        binding = (ROOT / "vapi/characters-icu.vapi").read_text()
        for name in ("append_case_variants", "append_accent_variants",
                     "append_scalar_relationships"):
            self.assertIn(name, policy)
            self.assertNotIn(name, engine)
        for name in ("uppercase", "lowercase", "titlecase", "mirror",
                     "canonical_decomposition"):
            self.assertIn(f"Icu.{name} (", policy)
            self.assertIn(f"characters_icu_{name}", icu)
            self.assertIn(f"characters_icu_{name}", binding)
        self.assertIn("Unicode.confusables (point)", policy)
        self.assertNotIn("struct ScalarCandidates", engine)
        self.assertNotIn("unorm2_getDecomposition", engine)

    def test_third_vala_migration_wiring(self) -> None:
        """Require Vala semantic and Han policies at the prepared native boundary."""
        sources = (ROOT / "src/meson.build").read_text()
        plan = (ROOT / "src/search/search-plan.vala").read_text()
        parser = (ROOT / "src/search/query-parser.vala").read_text()
        for name in ("search/semantic-filters", "search/han-query"):
            self.assertIn(f"'{name}.vala'", sources)
            self.assertTrue((ROOT / f"src/{name}.vala").is_file())
        self.assertIn("han_vapi", sources)
        self.assertIn("SemanticFilters.configure (job, scope, filter)", plan)
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
        self.assertNotIn("gchar *characters_unicode_normalize(", engine)
        self.assertNotIn("gchar *characters_unicode_encode(", engine)
        scanner = (unicode / "character-search.c").read_text()
        emoji = (unicode / "emoji-catalog.c").read_text()
        self.assertIn("characters_icu_decode_utf8(", emoji)
        self.assertNotIn("U8_NEXT(", scanner + emoji)
        self.assertNotIn("characters_icu_decode_utf8(", scanner)
        self.assertIn("characters_unicode_emoji_points(", scanner)
        self.assertIn("characters_unicode_named_points(", scanner)
        legacy = (unicode / "legacy-encoding.c").read_text()
        self.assertIn("characters_icu_decode_utf16(", legacy)
        self.assertIn("characters_icu_encode_utf16(", legacy)
        for name in ("font-catalog", "font-face"):
            source = (ROOT / f"native/fonts/{name}.c").read_text()
            self.assertIn("characters_scalar_is_valid(", source)
            self.assertNotIn("static gboolean is_scalar(", source)
        header = (ROOT / "native/fonts/font-face.h").read_text()
        self.assertNotIn("characters_font_face_symbol_points(", header)
        self.assertIn("characters_font_face_symbol_code(", header)

    def test_native_glib_contracts(self) -> None:
        """Require direct GLib dependencies, consistent ownership and native ABI types."""
        native = ROOT / "native"
        self.assertIn("glib_dep = dependency('glib-2.0'", (ROOT / "meson.build").read_text())
        for component in ("unicode", "fonts"):
            build = (native / component / "meson.build").read_text()
            self.assertEqual(build.count("dependencies: [glib_dep,"), 2)
        paths = list(native.glob("*.h")) + list(native.glob("*/*.h"))
        paths += list(native.glob("*/*.c"))
        for path in paths:
            source = path.read_text()
            code = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
            self.assertIn("#include <glib.h>", source, str(path))
            self.assertNotRegex(code, r"\b(?:malloc|calloc|realloc|strdup|free)\s*\(", str(path))
            self.assertNotRegex(code, r"\bbool\b", str(path))
            self.assertNotRegex(code, r"typedef[^;]+\bgunichar\s*;", str(path))
        scanner = (native / "unicode/character-search.c").read_text()
        self.assertIn("g_try_realloc_n(search->ids", scanner)
        self.assertIn("atomic_load_explicit", scanner)
        for path in (ROOT / "vapi").glob("*.vapi"):
            self.assertNotIn('array_length_type = "size_t"', path.read_text())

    def test_character_presentation_migration(self) -> None:
        """Keep UI formatting and collection policy out of the native scanner."""
        scanner = (ROOT / "native/unicode/character-search.c").read_text()
        header = (ROOT / "native/unicode/character-search.h").read_text()
        source = (ROOT / "src/meson.build").read_text()
        presentation = (ROOT / "src/unicode/character-presentation.vala").read_text()
        collections = (ROOT / "src/search/collection-catalog.vala").read_text()
        parser = (ROOT / "src/search/query-parser.vala").read_text()
        for obsolete in ("characters_search_display", "characters_search_code",
                         "characters_search_name", "characters_search_filter_name",
                         "characters_search_category_name", "symbol_names[]", "plane_names[]"):
            self.assertNotIn(obsolete, scanner + header)
        self.assertIn("characters_search_set_metadata", scanner)
        self.assertIn("search->category_groups[type]", scanner)
        self.assertIn("Search.configure_metadata (job)", parser)
        self.assertIn("job.set_metadata (masks, labels, fallbacks)", collections)
        self.assertIn("Inspector.format_points (values, Inspector.Format.CODE)", presentation)
        self.assertIn("Search.points (id)", (ROOT / "src/unicode/inspection-snapshot.vala").read_text())
        self.assertIn("search/collection-catalog.vala", source)
        self.assertIn("unicode/character-presentation.vala", source)
        self.assertIn("src/search/collection-catalog.vala", (ROOT / "po/POTFILES.in").read_text())

    def test_emoji_annotation_migration(self) -> None:
        """Keep locale and alias naming policies outside the native annotation scan."""
        catalog = (ROOT / "native/unicode/emoji-catalog.c").read_text()
        scanner = (ROOT / "native/unicode/character-search.c").read_text()
        policy = (ROOT / "src/emoji/annotation-policy.vala").read_text()
        presentation = (ROOT / "src/emoji/presentation.vala").read_text()
        parser = (ROOT / "src/search/query-parser.vala").read_text()
        han = (ROOT / "src/search/han-query.vala").read_text()
        sources = (ROOT / "src/meson.build").read_text()
        self.assertNotIn("matches_locale_chain", catalog)
        self.assertNotIn("characters_emoji_alias_name", catalog + scanner)
        self.assertNotIn("characters_emoji_fold_term", scanner)
        self.assertNotIn("characters_search_set_locale", scanner)
        self.assertIn("search->emoji_locales", scanner)
        self.assertIn("query->emoji_terms", scanner)
        self.assertIn('append_locale (locales, ref count, "en")', policy)
        self.assertIn("Emoji.configure_locale (job, locale)", parser)
        self.assertIn("Emoji.configure_terms (job, text, terms, literal_letter)", han)
        self.assertIn("catalog_sequence_name (points)", presentation)
        self.assertIn("emoji/annotation-policy.vala", sources)
        self.assertIn("emoji/presentation.vala", sources)

    def test_property_metadata_migration(self) -> None:
        """Keep property selection and decimal formatting in Vala, not C."""
        native = ROOT / "native/unicode"
        source = (ROOT / "src/unicode/property-metadata.vala").read_text()
        for module, obsolete in (("scalar-properties", "characters_scalar_property("),
                                 ("unicode-engine", "characters_unicode_property("),
                                 ("unihan", "characters_unihan_property(")):
            for suffix in (".c", ".h"):
                self.assertNotIn(obsolete, (native / (module + suffix)).read_text())
        self.assertIn("Scalar.age (point)", source)
        self.assertIn("Scalar.combining_class (point).to_string ()", source)
        self.assertIn("Han.value (point, Han.Value.MANDARIN_FALLBACK)", source)
        self.assertIn("Han.collation (point, radical)", source)
        self.assertIn("unicode/property-metadata.vala", (ROOT / "src/meson.build").read_text())
        self.assertIn('public static unowned string? text',
                      (ROOT / "vapi/characters-icu.vapi").read_text())
        self.assertIn('public static unowned string? value',
                      (ROOT / "vapi/characters-han.vapi").read_text())

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
