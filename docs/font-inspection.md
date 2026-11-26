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

## Exact faces, variation axes and symbol maps

Disable fallback (the default) to render the mapped scalar directly from the
selected file and exact collection/named-instance index using FreeType. The grid
uses actual Unicode cmap enumeration for the selected face. Comparisons use
Fontconfig coverage discovery and exact rendering when fallback is disabled.
A mapped whitespace glyph can be transparent; an unsupported raster format or
unavailable font is explicitly marked unavailable, never silently substituted.
With fallback enabled, Pango renders as before and nominal face substitution is
reported. Scalar coverage does not prove correct composite shaping.

Select **Inspect Font Face and Symbol Map** to open a private exact-face preview.
Choose an available OpenType design axis by its four-character tag and edit its
coordinate within the font's declared range. The remaining axes retain their
coordinates. Reset restores the selected face or named instance rather than
forcing every axis to the base font's default. These edits are confined to this
preview and never modify installed fonts or the browser's selected face.

Microsoft Symbol charmaps (platform 3, encoding 0) are exposed as raw mapped
codes and glyph indexes. A symbol-only face is accessible even when there are
no Unicode mappings. The searchable raw code selector preserves the first
Microsoft Symbol cmap's exact entries; no low-byte alias is synthesized. Raw
symbol entries cannot enter the Unicode inspector, clipboard character actions,
favorites, history or composer. Only the hexadecimal code and glyph index can
be copied from this view. Other non-Unicode legacy font charmaps and unencoded
glyph browsing are outside this scope.

The exact preview rasterizer supports outline, monochrome, grayscale and
FreeType BGRA color bitmaps, selecting a nearby fixed strike when required.
It centers a single glyph rather than performing sequence shaping. Unsupported
formats (including SVG or COLR paint graphs not rasterized by the installed
FreeType build) produce an unavailable state. Pango remains available with
fallback enabled. High-DPI scaling, color font appearance, and theme transitions
require desktop qualification.

## Legacy character sets

The encoding browser supports 77 ICU character sets, with independent scalar
mappings (up to four bytes), contextual canonical repertoires, and exact sequence
conversion. See [legacy encodings](legacy-encodings.md) for the full catalog,
conversion semantics, examples and explicit limitations.

## Ownership and resources

Native font catalogs, coverage snapshots, and encoding tables are private
application resources. The asynchronous source loader owns requests until
completion and only the latest request is published. The existing search
controller copies the selected scalar repertoire before searching it. Grid
pages are bounded to 256 characters or 48 font comparisons. The selected face
is opened on a worker and transferred once; comparison cells and the face
inspector open independent faces on demand. The main-context raster work is
bounded per glyph but not benchmarked. No custom CSS is used. Three original
miniature fonts are included as test fixtures only; they are never installed.

## Inventory policy boundary

Vala `FontInventory` builds validated, sorted face records and family offsets
on the source worker. The native adapter retains raw Fontconfig patterns;
application face positions are translated to original pattern indexes for
coverage, Pango resolution and FreeType opening. Comparison filtering and
selection restoration use the same immutable inventory.
