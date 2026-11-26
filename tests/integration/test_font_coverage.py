"""Exercise exact font coverage with isolated, deliberately stale Fontconfig metadata."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


def main() -> int:
    """Discover fixture faces, then replace files while retaining the original inventory."""
    executable = Path(sys.argv[1]).resolve()
    fixtures = Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="characters-font-coverage-") as temporary:
        directory = Path(temporary)
        fonts = directory / "fonts"
        fonts.mkdir()
        cache = directory / "cache"
        cache.mkdir()
        unicode_path = fonts / "faces.ttc"
        symbol_path = fonts / "symbol.ttf"
        shutil.copyfile(fixtures / "faces.ttc", unicode_path)
        shutil.copyfile(fixtures / "symbol.ttf", symbol_path)
        root = ET.Element("fontconfig")
        ET.SubElement(root, "dir").text = str(fonts)
        ET.SubElement(root, "cachedir").text = str(cache)
        configuration = directory / "fonts.conf"
        ET.ElementTree(root).write(configuration, encoding="utf-8", xml_declaration=True)
        environment = os.environ.copy()
        environment.update(FONTCONFIG_FILE=str(configuration), FONTCONFIG_PATH=temporary,
                           XDG_CACHE_HOME=str(cache))
        subprocess.run([
            str(executable), "--stale-coverage", str(unicode_path), str(symbol_path),
            str(fixtures / "faces.ttc"), str(fixtures / "symbol.ttf")
        ], env=environment, check=True, timeout=20)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
