# Han and language data provenance

This directory contains a reproducible fallback snapshot, not official Unihan
17.0.0 data. `sources/xpinyin-database.def` is the unmodified Unicode 14-derived
xpinyin database (copyright 2012–2022 Qing Lee); the three `hanja*_hangul.tab`
files are unmodified XeTeXko 4.4 tables (copyright 2013–2025 Dohyun Kim).
Both are distributed under the LaTeX Project Public License 1.3c or later.
The license text is included in `LPPL-1.3c.tex`.

`evdev.xml` is the unmodified installed XKB registry; its upstream copyrights
and permissions are preserved in `XKB-DEBIAN-COPYRIGHT`. ICU 76.1 collation and
locale data are accessed via its public APIs during export; see the complete
`ICU-DEBIAN-COPYRIGHT`. Unicode-derived data are also subject to the Unicode
license included in `../supplement/LICENSE.unicode`. `source-sha256.txt`
identifies the source inputs. No external runtime is needed to read the snapshot.

The official-data importer accepts separately acquired Unicode 17 Unihan files.
See `docs/international-browsing.md` for regeneration and limitations.
