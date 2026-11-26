# International browsing and Han metadata

## Browsing and search

Scripts match the Unicode 17 primary script and Script_Extensions. Languages
resolve to script unions using ICU locale-to-script data. The language filter
contains Current Locale, Configured Keyboard Languages, and 275 named language
labels. A language scope is a writing-system repertoire, not a dictionary or an
exact alphabet. Configured keyboards use registered XKB layout/variant IDs and
IBus engine IDs from the optional GNOME input-sources GSettings schema. The
application resolves exact configured IBus IDs against the running daemon's
engine registry, then combines their declared writing systems with XKB scripts.
Changes refresh the selected collection; unavailable engines and neutral
language descriptors are ignored, without guessing from engine names. The
application never changes the configured or active input source. Without a
resolved source the corresponding scope is empty. See
[IBus-aware browsing](ibus-aware-browsing.md) for discovery and limitations.

Radical and stroke collections provide additional filters. Bundled official records
use kRSUnicode and kTotalStrokes. Where no formal value exists,
the available ICU 76 Chinese collation groups are used and explicitly labeled.
Radical groups are approximate sort-key groups and stroke groups come from
explicit stroke tailoring entries. They are not authoritative radical or stroke
properties. Not every ideograph has a fallback stroke group.

| Query | Meaning |
| --- | --- |
| `shui` or `shuǐ` | Reading or ordinary metadata search |
| `mandarin:shui` | Mandarin reading field |
| `korean:수` | Korean reading field |
| `radical:85 strokes:4` | Han radical and stroke intersection |
| `cantonese:seoi2` | Cantonese field |
| `japanese:mizu` | Japanese field |
| `definition:water` | Definition field |

Field values are individual terms. Multiple terms combine with AND. Reading
matching is case-insensitive and ignores Latin tone marks, but preserves
umlaut and Japanese voicing marks. Exact literal and numeric queries retain
the existing behavior. Han inspector rows identify formal fields and fallback
fields separately; absent data are not inferred.

## Bundled coverage and provenance

The verified Unicode 17.0.0 Unihan archive supplies 102,998 official records.
All ten supported fields are populated wherever present in the source: Mandarin,
Cantonese, Japanese (combined, on and kun), Korean, Hangul, definitions,
radical-stroke classification and total strokes. Both classification fields
cover all 102,998 official records. The independent fallback snapshot retains
98,012 records, including 41,422 Mandarin readings, 27,786 Hangul readings,
97,999 radical collation memberships and 24,040 stroke collation memberships.
The combined database has 103,039 distinct records. The formal fields remain
separate from fallback data and missing values are never fabricated. See the
[import manifest](../data/unicode/official-data.json) for exact field counts.

See `data/unicode/han/README.md` for source licenses and attribution.

## Regeneration

Regenerate the checked-in tables from the pinned compressed snapshot and
bundled verified Unihan archive (discovered automatically):

```sh
python3 tools/unicode/generate_unihan.py data/unicode/han/fallback.json.gz \
  native/unicode/generated
```

The optional fallback exporter requires ICU 76.1, Python regex Unicode 17 data,
and the retained input tables. It is not part of the application build:

```sh
python3 tools/unicode/export_han_fallback.py \
  data/unicode/han/sources/xpinyin-database.def data/unicode/han/sources \
  data/unicode/han/sources/evdev.xml data/unicode/han/fallback.json.gz
```

Acquire the official Unicode 17.0.0 Unihan archive from
https://www.unicode.org/Public/17.0.0/ucd/Unihan.zip, extract its original text
files to a local directory, retain the license and source hashes, then run:

```sh
python3 tools/unicode/generate_unihan.py data/unicode/han/fallback.json.gz \
  native/unicode/generated --official /path/to/extracted-unihan
```

The importer checks version headers, scalar validity, duplicate properties,
required property presence, and radical/stroke grammar. It does not certify the
completeness or authenticity of arbitrary user-provided data files. Formal
classification syntax follows Unicode Standard Annex #38
(https://www.unicode.org/reports/tr38/). After import run the full test suite
and compare coverage to the official source before shipping. Rebuild the native
engine to use the new snapshot. No network access occurs during application use
or a normal Meson build.

For complete release inputs, use the verified original-archive workflow in
[official data import](official-data-import.md). The directory-based importer
remains available for development and fixture checks; it is not a proof of
release authenticity.
