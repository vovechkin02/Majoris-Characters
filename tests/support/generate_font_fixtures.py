"""Generate original, deterministic miniature font fixtures (requires FontTools)."""

import argparse
from pathlib import Path
from tempfile import TemporaryDirectory

from fontTools.designspaceLib import AxisDescriptor, DesignSpaceDocument
from fontTools.designspaceLib import InstanceDescriptor, SourceDescriptor
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTCollection, TTFont
from fontTools.ttLib.tables._c_m_a_p import CmapSubtable
from fontTools.varLib import build


EPOCH = 2082844800


def outline(width: int):
    """Create a simple rectangular glyph with variable width and stable topology."""
    pen = TTGlyphPen(None)
    if width:
        pen.moveTo((50, 0))
        pen.lineTo((50 + width, 0))
        pen.lineTo((50 + width, 700))
        pen.lineTo((50, 700))
        pen.closePath()
    return pen.glyph()


def master(path: Path, width: int, style: str, symbol: bool = False) -> None:
    """Create an original three-glyph face, optionally with only a raw symbol cmap."""
    builder = FontBuilder(1000, isTTF=True)
    builder.setupGlyphOrder([".notdef", "g1", "g2"])
    builder.setupCharacterMap({0x41: "g1", 0x20: "g2"})
    builder.setupGlyf({".notdef": outline(0), "g1": outline(width), "g2": outline(0)})
    builder.setupHorizontalMetrics({glyph: (1000, 0) for glyph in builder.font.getGlyphOrder()})
    builder.setupHorizontalHeader(ascent=800, descent=-200)
    builder.setupNameTable({
        "familyName": "Characters Fixture", "styleName": style,
        "uniqueFontIdentifier": f"CharactersFixture-{style}",
        "fullName": f"Characters Fixture {style}",
        "psName": f"CharactersFixture-{style}",
    })
    builder.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800,
                     usWinDescent=200)
    builder.setupPost(keepGlyphNames=False)
    builder.setupMaxp()
    if symbol:
        table = CmapSubtable.newSubtable(4)
        table.platformID = 3
        table.platEncID = 0
        table.language = 0
        table.cmap = {0xF041: "g1", 0xF042: "g2"}
        builder.font["cmap"].tables = [table]
    builder.font["head"].created = EPOCH
    builder.font["head"].modified = EPOCH
    builder.font.recalcTimestamp = False
    builder.save(path)


def generate(output: Path) -> None:
    """Create a two-face collection, a one-axis variable face and a symbol-only face."""
    output.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory() as temporary:
        work = Path(temporary)
        regular = work / "regular.ttf"
        bold = work / "bold.ttf"
        master(regular, 160, "Regular")
        master(bold, 650, "Bold")
        collection = TTCollection()
        collection.fonts = [TTFont(regular, recalcTimestamp=False),
                            TTFont(bold, recalcTimestamp=False)]
        collection.save(output / "faces.ttc")
        for font in collection.fonts:
            font.close()
        document = DesignSpaceDocument()
        axis = AxisDescriptor()
        axis.name = "Weight"
        axis.tag = "wght"
        axis.minimum = 100
        axis.default = 100
        axis.maximum = 900
        document.addAxis(axis)
        for path, coordinate in ((regular, 100), (bold, 900)):
            source = SourceDescriptor()
            source.path = str(path)
            source.name = path.stem
            source.location = {"Weight": coordinate}
            source.copyInfo = coordinate == 100
            document.addSource(source)
        instance = InstanceDescriptor()
        instance.name = "Heavy"
        instance.familyName = "Characters Fixture"
        instance.styleName = "Heavy"
        instance.location = {"Weight": 900}
        document.addInstance(instance)
        font, _, _ = build(document)
        font.recalcTimestamp = False
        font["head"].created = EPOCH
        font["head"].modified = EPOCH
        font.save(output / "variable.ttf")
        master(output / "symbol.ttf", 420, "Symbol", symbol=True)


def main() -> int:
    """Regenerate the checked-in original fonts into the requested directory."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    generate(parser.parse_args().output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
