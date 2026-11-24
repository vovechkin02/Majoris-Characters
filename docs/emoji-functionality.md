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
right-facing ZWJ suffixes are grouped by a comparison key. Family, handshake,
relationship, regional flag and keycap groups also use explicit semantic keys.
Every ZWJ and modified alternative is already in the catalog. The original
input is selected when it is one of the supported variants. A text-default
base with a fully qualified base + FE0F entry exposes the bare scalar, FE0E
text presentation and FE0F emoji presentation. This is a bounded presentation
subset: the full standardized variation-sequence file is not bundled, and
explicit presentation variants for emoji-default scalars absent from that subset
are not synthesized. Minimally qualified or non-RGI arbitrary sequences can
still be identified and copied in the inspector but are not all variation
picker identities.

Pango renders exact strings with system font fallback. The Debian package
requires Noto Color Emoji; no glyph artwork is copied into the application.
Font and shaping support determine whether a particular composite is displayed
as one color glyph. Unknown-glyph indications are advisory and do not establish
that a composite was shaped into one glyph. New Unicode 17 emoji may not be
available in Debian's installed fonts. The font browser's explicit face and
fallback settings are unchanged.

## Annotations

`--unicode-version` reports annotation provenance independently of the catalog,
ICU properties, and alias supplements. The bundled dataset is from Debian GTK
3.24.49-3 Emojibase annotation resources (24 languages, 1,915 source records per
language), not an asserted full CLDR 48 dataset. Supported primary languages:
Bengali, Danish, Dutch, English, Estonian, Finnish, French, German, Hindi,
Hungarian, Italian, Japanese, Korean, Lithuanian, Malay, Norwegian Bokmal,
Polish, Portuguese, Russian, Spanish, Swedish, Thai, Ukrainian, and Chinese.

A request captures the locale on the main context before worker dispatch.
Locale lookup uses the normalized language tag, less-specific tags, and English
fallback. English catalog names and semantic groups remain searchable. The
annotation table compares exact sequence keys without presentation selectors;
when no exact keyword matches, an unmodified skin-tone base may also supply
keywords. Case folding uses ICU. Multiple terms are ANDed across the catalog
label, category and annotation keywords. A single non-ASCII Unicode letter in
the Emoji scope can also act as an annotation term (for example a Chinese
keyword); explicit numeric code-point queries remain exact. The original
sequence is never modified by the annotation-key normalization. Uncovered
localized names fall back to English catalog metadata. The UI itself remains
English unless translations are separately installed.

The native lookup tables are immutable, compiled into the application and
regenerated offline. See [annotation input](../data/unicode/emoji/README.md)
for provenance and importing direct CLDR annotation XML. No network requests,
service accounts, font installation commands or usage tracking are introduced.
