"""Verify local collection and preview persistence across independent processes."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> int:
    """Seed and reopen the real repository using an isolated keyfile backend."""
    executable = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="characters-library-") as directory:
        environment = os.environ.copy()
        environment.update(
            HOME=directory,
            XDG_CONFIG_HOME=directory,
            GSETTINGS_BACKEND="keyfile",
            G_DEBUG="fatal-warnings",
        )
        for option in ("--seed", "--verify"):
            subprocess.run(
                [str(executable), option], env=environment, check=True, timeout=15
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
