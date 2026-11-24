"""Run the complete Meson release gate without silently accepting skipped tests."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def run(command: list[str], timeout: int) -> None:
    """Run one bounded command and stop on failure or timeout."""
    print("+ " + " ".join(command), flush=True)
    subprocess.run(command, cwd=ROOT, timeout=timeout, check=True)


def expected_tests() -> set[str]:
    """Read the registered test names from the project's single test manifest."""
    manifest = (ROOT / "tests/meson.build").read_text()
    return set(re.findall(r"^test\('([^']+)'", manifest, re.MULTILINE))


def verify_test_log(path: Path) -> None:
    """Require every declared test to appear exactly once with an OK result."""
    expected = expected_tests()
    seen = set()
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            if not line.strip():
                continue
            row = json.loads(line)
            name = row["name"].split(" / ")[-1]
            if name not in expected or name in seen or row["result"] != "OK":
                raise ValueError(f"Unexpected test outcome: {name}: {row['result']}")
            seen.add(name)
    missing = expected - seen
    if missing:
        raise ValueError("Missing required tests: " + ", ".join(sorted(missing)))
    print(f"All {len(seen)} required Meson tests reported OK.")


def main() -> int:
    """Build in a fresh directory, test the full inventory, and optionally build Debian."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path, help="A new, unused build directory")
    parser.add_argument("--package", action="store_true", help="Also build Debian binaries")
    options = parser.parse_args()
    build_dir = options.build_dir.resolve()
    if build_dir.exists():
        parser.error("Use a fresh build directory; existing results are not release evidence")
    try:
        run(["meson", "setup", str(build_dir), str(ROOT), "--prefix=/usr"], 120)
        run(["meson", "compile", "-C", str(build_dir)], 600)
        run(["meson", "test", "-C", str(build_dir), "--print-errorlogs"], 600)
        verify_test_log(build_dir / "meson-logs/testlog.json")
        if options.package:
            run(["dpkg-buildpackage", "-us", "-uc", "-b"], 1200)
        else:
            print("Debian binary packaging: not requested; not run.")
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        print(f"Qualification failed or incomplete: {error}", file=sys.stderr)
        return 1
    print("Automated build/test gate passed; real desktop qualification is still required.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
