# Priority 4 qualification — 1.0.4

## Implemented source

- Exact file/face/named-instance FreeType rasterization for fallback-disabled
  font grids and comparisons. Pango remains available with fallback enabled.
- The selected installed face is opened on the source worker and its exact
  Unicode cmap enumerated; its independently owned FreeType state is transferred
  once to the GTK consumer. Fontconfig remains the inventory and comparison
  prefilter. The separate face inspector opens an independent copy.
- Design axes are enumerated by OpenType tag with declared range, default and
  current value. Coordinate validation is finite and range checked. Changing an
  axis retains the remaining axes; reset restores the original selected instance.
  Adjustments affect the dedicated face preview, not installed fonts or the grid.
- The first Microsoft Symbol cmap is browsable through raw hexadecimal code and
  glyph-index metadata. No synthetic Unicode identity is published. A font with
  no Unicode cmap remains accessible through the face inspector.
- Bound glyph raster output (8–192 pixel request, at most 384-square RGBA),
  direct grayscale/monochrome/BGRA conversion, fixed-strike selection and explicit
  unavailable state rather than an implicit fallback. FreeType handles supported
  outline/color formats; unsupported paint formats are not claimed to render.
- Original TTC, variable and symbol-only fixtures, native contract suite, Vala
  binding tests, and GTK dialog checks. FreeType is an explicit build dependency.

## Checks executed in this delivery environment

- Recompiled all existing ICU-based native sources with GCC, GNU11,
  `-O2 -Wall -Wextra -Werror` against ICU 76.1; 36 groups passed (Unicode 6,
  search 10, inspector 5, legacy encodings 4, emoji 7, composition 4).
- Python input protocol (3), release source contracts (4), Unihan importer (3),
  and emoji generation/import (5): 15 tests passed.
- Generated the three font fixtures twice with FontTools 4.63.0; all binaries
  matched byte-for-byte. Independently opened the fixtures through the installed
  FreeType shared library using a temporary ctypes smoke probe: both TTC indices
  opened; variable coordinates were 100 for the base and 900 for the Heavy named
  instance (index 65536); the symbol font had no Unicode cmap and enumerated
  exactly raw 0xF041 and 0xF042 mapped to glyphs 1 and 2. This probes the fixtures
  and underlying runtime API, not the newly authored C adapter.
- Verified DejaVu Sans's 5,918 FontTools cmap entries match Fontconfig's reported
  coverage for that installed file. This is not a system-wide coverage audit.
- Declaration documentation checker, XML resources and accessible-control
  source checks, Python syntax, Debian version metadata and archive integrity.

## Not run / outstanding

The container lacks FreeType, Fontconfig, GLib and Pango development headers,
GTK4/libadwaita development libraries, valac, Meson and GLib resource/schema
compiler tools. `apt-get update` failed to resolve Debian mirrors. Consequently
new native font C compilation, deterministic C font tests, Vala binding tests,
GTK dialog interaction, full build, strict schema compilation, sanitizers for
new font code and Debian binary packaging were not run. No native font adapter
or GUI runtime success is claimed. Run the complete release gate on Debian 13.

No performance benchmarks were run. Selected-face loading uses the existing
worker; comparison cells and the dedicated inspector open faces on demand on
the GTK main context. Large font collections, color font rendering, unusual
bitmap strikes, high-DPI/theme transitions, and named-instance adjustments
require target qualification. Symbol mapping support is deliberately limited to
Microsoft Symbol; other legacy non-Unicode cmap formats and unencoded glyphs
are not implemented. All earlier Unicode/Unihan/CLDR data limitations remain.
The next functional priority is legacy encoding expansion.
