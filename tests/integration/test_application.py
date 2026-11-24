"""Exercise the real executable in isolated CLI and graphical sessions."""

import argparse
import base64
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest


APP_ID = "com.majoris.characters"
EXECUTABLE = ""
VERSION = ""


class ApplicationTest(unittest.TestCase):
    """Isolate each test from the developer's settings and user directories."""

    def setUp(self) -> None:
        """Provide independent home, config, cache and data paths."""
        directory = tempfile.TemporaryDirectory(prefix="characters-test-")
        self.addCleanup(directory.cleanup)
        root = Path(directory.name)
        self.env = os.environ.copy()
        for key, name in (
            ("HOME", "home"), ("XDG_CONFIG_HOME", "config"),
            ("XDG_CACHE_HOME", "cache"), ("XDG_DATA_HOME", "data"),
            ("XDG_STATE_HOME", "state"),
        ):
            path = root / name
            path.mkdir()
            self.env[key] = str(path)
        self.env.update(GSETTINGS_BACKEND="keyfile", LC_ALL="C.UTF-8",
                        GTK_A11Y="none", G_DEBUG="fatal-warnings")

    def run_command(
        self, *args: str, check: bool = True, timeout: int = 10
    ) -> subprocess.CompletedProcess:
        """Run a bounded subprocess and preserve its diagnostics for failures."""
        result = subprocess.run(args, env=self.env, text=True, capture_output=True,
                                timeout=timeout, check=False)
        if check:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result


class CommandLineTest(ApplicationTest):
    """Check local CLI handling without access to a graphical display."""

    def setUp(self) -> None:
        """Remove graphical session addresses before invoking CLI options."""
        super().setUp()
        for key in ("DISPLAY", "WAYLAND_DISPLAY", "DBUS_SESSION_BUS_ADDRESS"):
            self.env.pop(key, None)

    def test_version(self) -> None:
        """Return the configured product version without GTK initialization."""
        result = self.run_command(EXECUTABLE, "--version")
        self.assertEqual(result.stdout.strip(), f"Characters {VERSION}")

    def test_unicode_version(self) -> None:
        """Report each independent Unicode database version without a display."""
        result = self.run_command(EXECUTABLE, "--unicode-version")
        self.assertRegex(result.stdout, r"Catalog: \d+\.\d+\.\d+")
        self.assertRegex(result.stdout, r"Properties: \d+\.\d+")
        self.assertRegex(result.stdout, r"Supplement: \d+\.\d+\.\d+")
        self.assertIn("Emoji annotations:", result.stdout)

    def test_input_search(self) -> None:
        """Return an exact composite sequence without a display or session bus."""
        result = self.run_command(EXECUTABLE, "--input-search=U+1F469 U+200D U+1F4BB")
        rows = [line.split("\t") for line in result.stdout.splitlines()]
        self.assertEqual(len(rows), 1)
        self.assertEqual(base64.b64decode(rows[0][0]).decode(), "👩‍💻")
        self.assertTrue(base64.b64decode(rows[0][1]).decode())
        empty = self.run_command(EXECUTABLE, "--input-search=U+0000")
        self.assertEqual(empty.stdout, "")
        invalid = self.run_command(EXECUTABLE, "--input-search=", check=False)
        self.assertNotEqual(invalid.returncode, 0)

    def test_input_search_stdin(self) -> None:
        """Match the legacy CLI results without placing the query in process arguments."""
        query = "👩‍💻"
        expected = self.run_command(EXECUTABLE, "--input-search=" + query).stdout.encode()
        result = subprocess.run([EXECUTABLE, "--input-search-stdin"],
                                input=query.encode(), env=self.env, capture_output=True,
                                timeout=10, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, expected)
        for invalid in (b"", b" ", b"A\x00B", b"\xff", b"x" * 513):
            with self.subTest(invalid=invalid):
                result = subprocess.run([EXECUTABLE, "--input-search-stdin"], input=invalid,
                                        env=self.env, capture_output=True, timeout=10,
                                        check=False)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, b"")

    def test_help(self) -> None:
        """Advertise the supported local version option."""
        result = self.run_command(EXECUTABLE, "--help")
        self.assertIn("--version", result.stdout)

    def test_invalid_option(self) -> None:
        """Reject an unknown option rather than starting a window."""
        result = self.run_command(EXECUTABLE, "--invalid-option", check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("invalid-option", result.stderr)


class LifecycleTest(ApplicationTest):
    """Verify primary-instance activation and closing through the Quit action."""

    def test_activation_and_settings(self) -> None:
        """Launch the XML window, reactivate it, and save normal window geometry."""
        self.run_command("gsettings", "set", APP_ID, "window-width", "800")
        self.run_command("gsettings", "set", APP_ID, "window-height", "600")
        self.run_command("gsettings", "set", APP_ID, "window-maximized", "false")
        with tempfile.TemporaryFile(mode="w+t") as log:
            process = subprocess.Popen([EXECUTABLE], env=self.env, stdout=log,
                                       stderr=subprocess.STDOUT, text=True)
            try:
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    self.assertIsNone(process.poll(), "Primary process exited at startup")
                    result = self.run_command(
                        "gdbus", "call", "--session", "--dest", "org.freedesktop.DBus",
                        "--object-path", "/org/freedesktop/DBus", "--method",
                        "org.freedesktop.DBus.NameHasOwner", APP_ID,
                    )
                    if "true" in result.stdout:
                        break
                    time.sleep(0.05)
                else:
                    self.fail("Primary application did not acquire its D-Bus name")
                self.run_command(EXECUTABLE)
                self.assertIsNone(process.poll(), "Reactivation closed the primary instance")
                actions = self.run_command("gapplication", "list-actions", APP_ID)
                self.assertIn("about", actions.stdout.split())
                self.assertIn("quit", actions.stdout.split())
                self.run_command("gapplication", "action", APP_ID, "quit")
                self.assertEqual(process.wait(timeout=10), 0, "Quit did not exit cleanly")
                for key, expected in (("window-width", "800"), ("window-height", "600")):
                    result = self.run_command("gsettings", "get", APP_ID, key)
                    self.assertEqual(result.stdout.strip(), expected, key)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5)
                log.seek(0)
                print(log.read(), end="")


    def test_desktop_search_and_picker(self) -> None:
        """Exercise the real SearchProvider2 export and remote application actions."""
        path = "/com/majoris/characters/SearchProvider"
        interface = "org.gnome.Shell.SearchProvider2"
        with tempfile.TemporaryFile(mode="w+t") as log:
            process = subprocess.Popen([EXECUTABLE, "--picker"], env=self.env,
                                       stdout=log, stderr=subprocess.STDOUT, text=True)
            try:
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    self.assertIsNone(process.poll())
                    introspection = self.run_command(
                        "gdbus", "introspect", "--session", "--dest", APP_ID,
                        "--object-path", path, check=False,
                    )
                    if interface in introspection.stdout:
                        break
                    time.sleep(0.05)
                else:
                    self.fail("Search provider was not exported")
                common = ("gdbus", "call", "--session", "--dest", APP_ID,
                          "--object-path", path, "--method")
                initial = self.run_command(*common, interface + ".GetInitialResultSet",
                                           "['U+20AC']")
                self.assertIn("U+20AC", initial.stdout)
                refined = self.run_command(*common, interface + ".GetSubsearchResultSet",
                                           "['U+20AC']", "['U+1F600']")
                self.assertIn("U+1F600", refined.stdout)
                meta = self.run_command(*common, interface + ".GetResultMetas",
                                        "['U+20AC', 'invalid']")
                self.assertIn("EURO SIGN", meta.stdout)
                self.assertNotIn("invalid", meta.stdout)
                self.run_command(EXECUTABLE, "--picker")
                self.run_command(EXECUTABLE, "--search=U+20AC")
                self.run_command(*common, interface + ".ActivateResult", "U+20AC",
                                 "['U+20AC']", "0")
                self.run_command(EXECUTABLE, "--record-insertion=👩‍💻")
                history = self.run_command("gsettings", "get", APP_ID, "usage-history")
                self.assertIn("👩‍💻", history.stdout)
                self.run_command("gapplication", "action", APP_ID, "quit")
                self.assertEqual(process.wait(timeout=10), 0)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5)
                log.seek(0)
                print(log.read(), end="")


class BrowserTest(ApplicationTest):
    """Exercise the compiled GTK browser in the same isolated user environment."""

    def test_interaction(self) -> None:
        """Run the real template, model, filtering, paging, and supersession test."""
        self.env["GSETTINGS_BACKEND"] = "memory"
        result = self.run_command(EXECUTABLE, timeout=45)
        print(result.stdout, end="")


def main() -> int:
    """Run the selected integration profile and return its test status."""
    global EXECUTABLE, VERSION
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("version")
    parser.add_argument("profile", choices=("cli", "gui", "browser"))
    args = parser.parse_args()
    EXECUTABLE = str(args.executable.resolve())
    VERSION = args.version
    case = {
        "cli": CommandLineTest, "gui": LifecycleTest, "browser": BrowserTest,
    }[args.profile]
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(case)
    return int(not unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful())


if __name__ == "__main__":
    raise SystemExit(main())
