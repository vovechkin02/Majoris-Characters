# Emoji functionality

The existing nine category collections retain all 3,944 fully qualified Unicode
17 emoji entries. Browsing and exact code-point search continue to use the
native catalog and its stable private IDs. Global results deduplicate scalar
and sequence identities as before. No sequence is normalized when selected,
inspected or copied.

## Variation selection

The Emoji Variations action is available from the browser selection and the
character inspector. The searchable selector shows the original selection,
preview, complete code points and English catalog label. Select another entry,
then copy or inspect it. The native collection owns exact UTF-8 text and is
independent of search jobs and inspector history.

Variation families are based on existing catalog entries, not arbitrary modifier
products. Skin-tone modifiers, gender suffixes, gendered people, hair and
right-facing ZWJ suffixes are grouped by a comparison key. Families, handshakes,
relationships, regional and subdivision flags, and keycaps use structural keys
rather than English label prefixes. These are application browsing groups, not
an official Unicode semantic-family taxonomy. No arbitrary ZWJ sequence is
synthesized. The exact incoming sequence is the first alternative, including
minimally qualified or unqualified forms.

Priority 3 adds the 371 standardized Unicode 17 presentation bases, with both
FE0E and FE0F (742 pairs). This covers both text-default and emoji-default
characters, including standalone keycap bases. The generated supplementary table
contains 1,819 noncatalog identities, including the 1,272 selector omissions
obtained from the fully qualified catalog. The omission count matches the sum
of the published 1,029 minimally qualified and 243 unqualified entries. The
full official emoji-test file was not available as a local file for set-level
comparison; the generator can perform that comparison when supplied. The nine
emoji component entries are not standalone RGI emoji and remain available in
All Characters as scalars.

Exact literal and explicit code-point searches return the exact supplementary
identity in All Characters and Emoji, respecting category filters. Normal browsing
continues to expose only the existing 3,944 catalog entries; saved collections
and clipboard operations use text, not supplementary table indexes. The inspector
shows the canonical association and explicit presentation labels and links to
standardized presentation alternatives. All forms remain distinct Unicode text.

Pango renders exact strings with system font fallback. The Debian package
requires Noto Color Emoji; no glyph artwork is copied into the application.
Font and shaping support determine whether a particular composite is displayed
as one color glyph. Unknown-glyph indications are advisory and do not establish
that a composite was shaped into one glyph. New Unicode 17 emoji may not be
available in Debian's installed fonts. The font browser's explicit face and
fallback settings are unchanged.

## Annotations

`--unicode-version` reports annotation provenance independently of the catalog,
ICU properties, and alias supplements. The verified CLDR 48 release supplies
base and derived annotations for 171 locale tables, including all 24 previously
bundled languages, regional variants and script variants. This replaces the
older GTK/Emojibase annotation snapshot. Annotation locales are not interface
translation locales. See `data/unicode/emoji/coverage.json` for the complete
list of bundled tags and their coverage.

A request captures the locale on the main context before worker dispatch.
Locale lookup uses case-insensitive normalized tags, less-specific tags,
Chinese regional script fallbacks, and English fallback for individual terms,
not only for missing locale tables. English catalog names and semantic groups remain searchable. The
annotation table compares exact sequence keys without presentation selectors;
when no exact keyword matches, an unmodified skin-tone base may also supply
keywords. Case folding uses ICU. Multiple terms are ANDed across the catalog
label, category and annotation keywords. A single non-ASCII Unicode letter in
the Emoji scope can also act as an annotation term (for example a Chinese
keyword); explicit numeric code-point queries remain exact. The original
sequence is never modified by the annotation-key normalization. Uncovered
localized names fall back to English catalog metadata. The interface includes a partial French translation in Stage 10. Other languages
use the English interface unless a translation is separately installed.

The native lookup tables are immutable, compiled into the application and
regenerated offline. See [annotation input](../data/unicode/emoji/README.md)
for provenance and importing direct CLDR annotation XML. No network requests,
service accounts, font installation commands or usage tracking are introduced.

The per-locale `data/unicode/emoji/coverage.json` report counts direct normalized
annotation keys, tone-base fallback keys, and uncovered catalog entries after
import-time parent field resolution. English has direct keys for all 3,944
catalog emoji. Of the 171 locale tables, 91 have complete coverage by direct
or tone-base keys. Remaining tables contain upstream gaps; runtime English
fallback is not counted as localized coverage. Catalog names remain searchable
for every entry. Locale aliases and synthesis beyond the supplied derived
annotations are not implemented as a general LDML processor.

## Implementation boundary

The Vala `Variants` collection owns exact text and labels. `variation-family.vala`
constructs structural keys from scalar arrays; no generated data is duplicated.
The native catalog retains immutable alias and annotation lookups used by both
the search worker and Vala. See [migration qualification](vala-migration.md).

The 1.0.22 dataset was populated by the verified CLDR 48 workflow. See
[official data import](official-data-import.md) for provenance and qualification.
