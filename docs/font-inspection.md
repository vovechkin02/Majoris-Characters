# Font inspection and legacy character sets

## Installed fonts

Open Fonts from the sidebar or main menu. Select an installed family and face;
face identity includes its Fontconfig file and face index. The character grid
uses mapped Unicode scalar coverage, including mapped private-use characters.
Search by name, alias, literal scalar or numeric code point and apply the
available general-category filter. The inspector can open a mapped scalar by
explicit code point even if the Unicode database has no assigned name for it.

Preview size is adjustable from 12 to 72 points. Fallback is disabled by default
for font inspection; enable it explicitly if desired. The inspector's Font
Variations action compares faces covering the selected constituent scalar, with
family/face filtering and bounded pages. Color and variable font flags identify
Fontconfig-reported capabilities. Refresh explicitly or use the periodic
30-second freshness check; inventory operations run off the GTK main thread.

Coverage does not prove correct shaping of a composite sequence, color glyph
presentation, or support for arbitrary variation axes. Pango renders a font
description rather than a direct file/index request; where nominal resolution
selects another face, the browser reports substitution. GTK's Pango backend
may still produce missing glyphs or substitute fonts when fallback is enabled.
There is no arbitrary axis editor, font installation/removal, unencoded glyph
browser, or special handling of symbol-only non-Unicode charmaps. Color rendering
is delegated to the system renderer and requires runtime qualification.

## Legacy character sets

Select Character Sets, then an encoding. Browse distinct mapped scalars, inspect
all associated bytes, copy byte notation, or copy the Unicode text. `byte:80`
and `byte:82 A0` are explicit exact-byte queries; ordinary Unicode queries are
also supported. Invalid, truncated and multi-scalar byte mappings are excluded.
NUL cannot be copied through GTK's string clipboard but its byte and code-point
representations can be copied.

The 35 encodings available with the tested ICU 76 installation are:

- US-ASCII.
- ISO-8859-1 through ISO-8859-10, and ISO-8859-13 through ISO-8859-15.
- Windows-1250 through Windows-1258, and Windows-874.
- IBM 437, 850, 852 and 866.
- KOI8-R and KOI8-U.
- Macintosh Roman.
- Windows-932 (Shift JIS), Windows-936 (GBK), Windows-949 (Korean), and
  Windows-950 (Big5).

Only available stateless single- and double-byte ICU converters are exposed.
ISO-8859-16 is unavailable on the tested ICU installation. The catalog reflects
ICU decoding, including its defined aliases; it is not a guarantee that
re-encoding produces the original bytes or that other platforms use identical
mapping tables. Stateful encodings and multi-scalar mappings are outside this
stage. The encoding browser does not change the process or system locale.

## Ownership and resources

Native font catalogs, coverage snapshots, and encoding tables are private
application resources. The asynchronous source loader owns requests until
completion and only the latest request is published. The existing search
controller copies the selected scalar repertoire before searching it. Grid
pages are bounded to 256 characters or 48 font comparisons. Font previews
use Pango attributes rather than custom CSS. No font files are distributed
with this project.
