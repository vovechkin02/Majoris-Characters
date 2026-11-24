"""Exercise the real executable in isolated CLI and graphical sessions."""

import argparse
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

    def run_command(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        """Run a bounded subprocess and preserve its diagnostics for failures."""
        result = subprocess.run(args, env=self.env, text=True, capture_output=True,
                                timeout=10, check=False)
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


def main() -> int:
    """Run the selected integration profile and return its test status."""
    global EXECUTABLE, VERSION
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("version")
    parser.add_argument("profile", choices=("cli", "gui"))
    args = parser.parse_args()
    EXECUTABLE = str(args.executable.resolve())
    VERSION = args.version
    case = CommandLineTest if args.profile == "cli" else LifecycleTest
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(case)
    return int(not unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful())


if __name__ == "__main__":
    raise SystemExit(main())
