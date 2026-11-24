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
    "GtkSwitch", "GtkScale", "GtkCheckButton", "AdwSwitchRow", "AdwSpinRow",
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
