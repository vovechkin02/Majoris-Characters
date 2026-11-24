# Unicode engine contracts and data maintenance

Stage 2 builds a private static C library using ICU and the supplied GNOME
Characters tables. The VAPI is internal, not an installed ABI. Calls are
synchronous and safe to use from independent worker threads; catalog data is
immutable and ICU normalization instances are read-only. The application
only calls the version functions during local CLI handling. Interactive
search, cancellation and UI scheduling belong to Stage 3.

## Data provenance and limits

| Source | Version | Coverage |
| --- | --- | --- |
| GNOME Characters catalog | 17.0.0 | Explicit names, 346 blocks, 3,944 fully qualified emoji and single-scalar confusable classes |
| ICU 76 (Debian 13) | 16.0 | Canonical names including algorithmic CJK/Hangul, scalar assignment, scripts, properties, age, normalization |
| Unicode supplement | 15.0.0 | 473 name aliases and 461 named sequences |

The versions are independent and `--unicode-version` prints them. ICU is the
canonical name and property authority for its repertoire. The newer GNOME
catalog supplements explicitly named characters and fully qualified emoji.
It is not a complete Unicode 17 assignment or property database: newly assigned
algorithmic ideographs absent from the explicit name table cannot be reliably
recognized by ICU 76, and new single-code-point emoji may have a label but no
canonical name. The engine never guesses assignment from an entire block range.
The older alias/named-sequence supplement can be refreshed from official files.
CLDR localized annotations, Unihan readings, exhaustive emoji variants and
full newer Unicode property coverage are not provided in this release.

## Query API

See `native/unicode/unicode-engine.h` and `vapi/characters-unicode.vapi` for
ownership, invalid inputs and index contracts. The scalar interface uses numeric
code points; surrogate values are rejected except in block-range queries.
U+0000 can be inspected but cannot be returned as a standalone NUL-terminated
UTF-8 string. Exact sequences are bounded to 32 scalars; normalization accepts
up to 1 MiB of NUL-terminated UTF-8. No implicit normalization is performed.

`get_property_value` accepts the standardized ICU property names
`General_Category`, `Script`, `Bidi_Class`, `Bidi_Mirrored`, `Decomposition_Type`,
`East_Asian_Width`, `Emoji`, `Emoji_Presentation`, `Emoji_Modifier`, and the
explicit names `Age`, `Canonical_Combining_Class`, `Script_Extensions`, `Plane`.
Enumerated properties return standard short aliases; script extensions are
space-delimited ISO 15924 codes, age is a Unicode version, and combining class
and plane are decimal strings. Unknown or unavailable properties return null.
The plane is numeric and does not depend on the ICU assignment database.

Related-character queries adapt GNOME's case, mirroring, canonical variants,
Hangul decomposition, emoji decomposition and confusable lookup. Results are
sorted and unique and exclude the input. Confusable classes are advisory,
not a spoof-detection or security guarantee. ICU's metadata version also bounds
canonical relations. Full emoji variation selection belongs to Stage 6.

## Offline catalog updates

Acquire matching official `Blocks.txt`, `UnicodeData.txt`, `emoji-test.txt`,
and `confusables.txt` locally from https://www.unicode.org/Public/ and verify
their release versions and checksums. Place these four files in one directory.
The upstream generators are retained unchanged and are invoked by an offline
wrapper. Do not edit generated tables manually.

```sh
python3 tools/unicode/update_catalog.py /path/to/ucd-inputs 17.0.0
python3 tools/unicode/generate_supplement.py \
  data/unicode/supplement/NameAliases.txt \
  data/unicode/supplement/NamedSequences.txt 15.0.0 \
  native/unicode/generated/supplement.h
meson compile -C build
meson test -C build --suite unit --print-errorlogs
```

For a supplement update, replace the two checked-in source inputs with the
same-version official UCD files and use that version in the generator command.
Record upstream release/checksums and verify all catalog tests. For a newer ICU
property database, use a supported ICU distribution; the engine reports the
actual runtime version. These are developer operations, not runtime downloads.
The full official 17.0.0 input files were not supplied and no network access
was available; regeneration of the GNOME tables was not performed here.
