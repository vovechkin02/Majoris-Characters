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
with the same Unicode database. The checked-in generated supplement is
reproducible from these two local files. No Perl source code is incorporated.

ICU is a system dependency, not vendored or modified. Its Unicode property and
normalization data version is reported at runtime. See
https://github.com/unicode-org/icu for its licensing and source.

## Emoji annotations

Stage 6 includes annotation records exported from Debian GTK 3.24.49-3 emoji
chooser resources (Emojibase-derived). The original GTK Debian copyright file
is preserved in `data/unicode/emoji/GTK-DEBIAN-COPYRIGHT`; it identifies the
Emojibase source as Expat-licensed, copyright 2017–2019 Miles Johnson. Unicode
source data retain their Unicode license, included under `data/unicode/supplement/`.
The source export and generated lookup tables are data, not a GTK3 runtime
requirement. See `data/unicode/emoji/README.md` for exact provenance and regeneration.
