# International browsing and Han metadata

## Browsing and search

Scripts match the Unicode 17 primary script and Script_Extensions. Languages
resolve to script unions using ICU locale-to-script data. The language filter
contains Current Locale, Configured Keyboard Languages, and 275 named language
labels. A language scope is a writing-system repertoire, not a dictionary or an
exact alphabet. Configured keyboards use registered XKB layout/variant IDs from
the optional GNOME input-sources GSettings schema; changes refresh the current
search. Unknown or non-XKB input sources are ignored. The active IBus engine is
not detected. Without a configured XKB source the corresponding scope is empty.

Radical and stroke collections provide additional filters. Official records,
when imported, use kRSUnicode and kTotalStrokes. Where no formal value exists,
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
| `cantonese:seoi2` | Cantonese field (requires imported data) |
| `japanese:mizu` | Japanese field (requires imported data) |
| `definition:water` | Definition field (requires imported data) |

Field values are individual terms. Multiple terms combine with AND. Reading
matching is case-insensitive and ignores Latin tone marks, but preserves
umlaut and Japanese voicing marks. Exact literal and numeric queries retain
the existing behavior. Han inspector rows identify formal fields and fallback
fields separately; absent data are not inferred.

## Bundled coverage and provenance

98,012 records include 41,422 Mandarin readings (xpinyin, Unicode 14-derived),
27,786 Korean Hangul readings (XeTeXko 4.4), 97,999 approximate ICU radical
collation memberships, and 24,040 explicit stroke collation memberships. These
counts overlap. The official Unihan version is reported as `none`; the fallback
is not represented as Unicode 17 readings. Cantonese, Japanese readings and
English definitions require an official import; no fabricated values are used.
Unicode 17 assignment and script coverage remain independent of Han metadata.

See `data/unicode/han/README.md` for source licenses and attribution.

## Regeneration

Regenerate the checked-in tables from the pinned compressed snapshot:

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
