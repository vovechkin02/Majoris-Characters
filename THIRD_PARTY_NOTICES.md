# Third-party notices

The Unicode catalog in `native/unicode/vendor/gnome/` contains unmodified
header tables and generators from the supplied GNOME Characters 51.0 source
snapshot (Unicode 17.0.0), available upstream at
https://gitlab.gnome.org/GNOME/gnome-characters. The block, name and emoji
binary-search and related-character algorithms in the native engine are
adapted from `lib/gc.c`. The upstream README assigns BSD-3-Clause to files
from gtk-js-app and GPL-2.0-or-later to other files. Both upstream license
notices are preserved in the vendor directory; the adapted engine is
licensed GPL-3.0-or-later. Original generated table formatting is preserved.

Unicode character data, emoji data, and confusables are derived from Unicode
Consortium publications. The Unicode data license and attribution are in
`data/unicode/supplement/LICENSE.unicode`. The supplemental alias records
were exported from the Unicode 15.0.0 `Name_Alias` property through Perl's
public `Unicode::UCD::prop_invmap` interface; `NamedSequences.txt` was supplied
with the same Unicode database. The alias baseline includes eight documented Unicode 16 and 17 additions.
The named-sequence baseline is retained after review, without a full official-file
comparison. The checked-in generated supplement is
reproducible from these two local files. No Perl source code is incorporated.

The new scalar snapshot derives Unicode 17 property data from regex 2026.5.9
and FontTools Unicode 17 script tables, with stable age values from ICU 76.
No Python package implementation code is bundled. Unicode data attribution
and license are retained as above. ICU is a system dependency, not vendored
or modified; the application reports the effective snapshot version. See
https://github.com/unicode-org/icu for its licensing and source.

## Emoji annotations

The production annotation tables and compressed snapshot are derived from the
verified CLDR 48 base and derived annotation release (Unicode-3.0, copyright
1991–2025 Unicode, Inc.). The original GTK Debian copyright file is retained in
`data/unicode/emoji/GTK-DEBIAN-COPYRIGHT` for historical provenance of the
superseded Emojibase-derived snapshot. No GTK3 runtime dependency is introduced.
See `data/unicode/emoji/README.md` and the import manifest for exact provenance.

## International browsing data

The Han fallback includes readings extracted from xpinyin (Qing Lee,
2012–2022, LPPL 1.3c or later) and XeTeXko 4.4 (Dohyun Kim, 2013–2025,
LPPL 1.3c or later). Their original source tables are retained unchanged in
`data/unicode/han/sources/`. ICU 76.1 supplies collation groups and language
labels, and the XKB registry supplies configured keyboard language mappings.
Full original copyright notices, source hashes, and the LPPL license are
retained in `data/unicode/han/`. The derived snapshot and generated tables
retain the source data attribution and licensing. The verified official Unihan 17.0.0 archive is bundled and retains the Unicode data license.

## Priority 3 emoji sequence and annotation data

The 371 presentation bases are transcribed from Unicode 17.0
`emoji-variation-sequences.txt` (Copyright 2025 Unicode, Inc.), at
https://www.unicode.org/Public/17.0.0/ucd/emoji/emoji-variation-sequences.txt.
Selector omissions derive from the existing GNOME catalog. Eight English
base annotations derive from CLDR 48 `common/annotations/en.xml`
(Copyright 1991–2025 Unicode, Inc.), at
https://github.com/unicode-org/cldr/blob/release-48/common/annotations/en.xml.
These Unicode data are licensed under Unicode-3.0; see the included license
under `data/unicode/supplement/`. The historical eight-entry source remains attributed; production annotation
data now derive from the complete verified CLDR annotation import.

## Priority 8 localization

The French, German and Spanish catalogs in `po/` include script display names
from CLDR locale data distributed with Babel 2.18.0 (`Locale.scripts`). These
names are Unicode data, copyright 2004–2025 Unicode, Inc., and retain the exact
Unicode License V3 notice in `po/LICENSE.cldr`. Babel's BSD license notice is
preserved in `po/LICENSE.babel` for provenance. No Babel library or Python
implementation is shipped or required to build or run Characters. Original
interface translations and property descriptions are project-owned; all three
catalogs are partial and require native-speaker review.

## Official data archive workflow (1.0.22)

The verified official inputs are Unicode 17.0.0 `Unihan.zip` and CLDR 48
`cldr-common-48.zip`, copyright 1991–2025 Unicode, Inc., Unicode-3.0.
Their source URLs and pinned digests are documented in
`docs/official-data-import.md`. The retained Unicode License V3 notice in
`data/unicode/supplement/LICENSE.unicode` applies to official data and derived
lookup tables. The archive workflow has populated the official datasets in this release.
The Unihan archive and resolved CLDR snapshot are retained; fallback Han data
and historical source notices remain unchanged.
