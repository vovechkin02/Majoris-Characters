# Character inspector

Stage 4 adds a native inspection snapshot and an XML-backed, adaptive libadwaita
inspector. It does not change catalog identities, search filters, the geometry
schema, or the existing package and desktop identity.

## Usage

Select a grid cell, choose **Inspect Character**, or activate the cell using
Enter or a double-click. The inspector presents a large glyph, name, complete
code-point sequence, encodings, normalization, and related identities. A
component dropdown selects the scalar whose properties are shown. Copy
Information includes every component, not just the selected one.

Choose **Identify Characters** in the main menu, or expand the same section in
an open inspector. Paste exact text (including whitespace and combining marks)
and choose Identify Text. Alternatively, enter space-separated `U+`, `0x`, or
`dec:` tokens and choose Identify Code Points. The two actions are deliberately
separate: literal `U+0041` is six scalars, not the letter A. A numeric query may
inspect an unassigned scalar; surrogate values and values above U+10FFFF are
rejected. A numeric query may include U+0000, which is not representable by a
NUL-terminated GTK text string. Both input modes accept 1–32 scalars; numeric
notation has an additional 512-byte bound. Invalid input leaves the prior
snapshot unchanged.

Select a related character to inspect it independently of the browser. Back
restores the previous identity, retaining at most 32 entries. The history is
not the Stage 8 usage history and is discarded when the dialog closes.

## Data and representation contract

| Group | Meaning |
| --- | --- |
| Identity | Canonical Unicode name, standardized sequence or emoji label when available, exact code points and scalar count. |
| Properties | Per-scalar name, aliases, assignment, block, plane, script and extensions, general category, age, bidi class and mirroring, combining class, decomposition type, East Asian width, emoji properties. Property abbreviations follow UCD/ICU short names. |
| UTF-8 | Exact bytes, including 00 for NUL. |
| UTF-16 / UTF-32 | Numeric code units in order, not serialized little- or big-endian bytes and no BOM. |
| Numeric references | Decimal and hexadecimal HTML numeric-reference notation for each scalar. HTML parsers may replace NUL, remap some control codes, or reject other values. |
| Escapes | Fixed-width `\u`/`\U` notation and CSS hexadecimal notation with a terminating space per scalar. These are scalar representations; target language grammar and handling of controls differ. They are not guaranteed valid C, HTML, CSS or other source literals. |
| Normalization | NFC, NFD, NFKC and NFKD as exact code-point sequences. A copy-text action is offered when the original is representable as GTK text and normalization succeeds. NUL is preserved in numeric normalization output. |
| Related | Existing engine's sorted and deduplicated case variants, mirrors, decomposition, accents, confusables, and catalog sequence components. Related characters are suggestions, not a security/confusability guarantee. |

The preview is not used for copying. A dotted circle can accompany a combining
mark and a middle dot can represent controls or invisible characters. Known
emoji sequences are rendered intact; arbitrary sequences with invisible or
formatting scalars may use placeholders. Actual glyphs depend on the installed
font fallback set, with a missing-glyph note when detected. No font selection
or coverage analysis is implemented in this stage. Copy Character preserves
the exact original non-NUL text. For NUL-containing sequences, use code-point
or encoding copy actions. The explicit report and value actions never silently
replace source scalars with the preview.

The catalog is Unicode 17.0.0, ICU property and normalization data are Unicode
16.0, and the name alias/named-sequence supplement is Unicode 15.0.0. Values
unavailable in the corresponding database are shown as unavailable. The
normalization result is withheld if any scalar lacks assigned ICU properties;
this avoids implying that older ICU data cover newer catalog characters.

## Internal API and ownership

`native/unicode/character-inspector.h` defines an internal opaque snapshot and
format enumeration; `vapi/characters-inspector.vapi` binds both. The snapshot
owns an exact bounded scalar array, contains no references to the browser,
and transfers full ownership to the caller. Returned strings and related
vectors are owned; native callers free them with `free`, and Vala releases them
using GLib allocation-compatible functions. The shared explicit code parser
preserves the Stage 3 query syntax without requiring membership in a catalog.
ICU normalization uses length-aware UTF-16, including embedded NUL, and formats
the entire normalized result without imposing the input length limit on output.

The dialog remains on the GTK main context and uses standard action groups,
GDK clipboard and toast feedback. No network, service integration, external
process, or new persistent settings are involved. Stage 5 covers installed
font inspection; Stage 7 covers composition and drag-and-drop.
