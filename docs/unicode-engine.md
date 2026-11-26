# Unicode engine contracts and data maintenance

The private static C engine exposes an internal VAPI, not an installed ABI.
Catalog and scalar property data are immutable and safe for independent
worker threads. Search cancellation and GTK scheduling remain outside the
engine.

## Data provenance and limits

| Source | Version | Coverage |
| --- | --- | --- |
| GNOME Characters | 17.0.0 | Explicit names, 346 blocks, 3,944 fully qualified emoji, confusables |
| Generated scalar snapshot | 17.0.0 | Assignment, general category, primary script, script extensions, age, combining class, bidi, decomposition type, East Asian width, supported binary flags |
| ICU system library | Unicode 16.0 on Debian 13 | Stable names, normalization primitives, grapheme segmentation, legacy encodings, case folding |
| Alias supplement | 15.0 baseline plus verified 16/17 deltas | 481 aliases |
| Named-sequence supplement | 15.0 baseline reviewed through 17.0 | 461 records; full official-file comparison pending |

The snapshot is generated from regex 2026.5.9 Unicode 17 property tables,
FontTools Unicode 17 `Scripts` and `ScriptExtensions` tables, and ICU 76 age
values. Newly assigned scalars are given age 17.0. No package code is copied
or needed at runtime. Assignment is determined by the snapshot, not by block
membership or name availability. Surrogates and noncharacters are excluded
from assigned scalar enumeration. The 297,334 assigned scalars include private
use and controls. Canonical names use the GNOME table, stable ICU names,
Unicode 17 algorithmic ideograph names, and seven new scalar emoji names from
UnicodeData. The eight new alias records were verified against UCD 16/17.

The supported scalar fields target Unicode 17, not every UCD property.
The CLI reports effective property and catalog versions. The supplement version
identifies its target release, not a byte-for-byte official-file verification.
The raw UCD archive could not be acquired in the delivery environment. Full
UCD source comparison and the official normalization conformance suite remain
required for formal conformance. Script-extension-aware browsing and a partial
Han database with official Unihan import are provided in Priority 2; full CLDR
annotations and official emoji variation data remain separate priorities.

## Query API

See `native/unicode/unicode-engine.h` and `vapi/characters-unicode.vapi` for
ownership and index contracts. Scalar input is numeric. Surrogates are rejected
except in block-range queries. U+0000 is inspectable but cannot be represented
as standalone NUL-terminated UTF-8. Exact sequences are bounded to 32 scalars;
no implicit normalization is performed.

`Unicode.get_property_value` is implemented in the Vala property metadata service.
It accepts the historically supported ICU aliases for enumerated and Boolean
fields, and exact keys for age, combining class, script extensions, plane and
Han metadata. The native adapter exposes borrowed immutable text values and
raw numeric properties, with no application-level formatting.
Enumerated values use standardized short aliases; script extensions are
space-delimited ISO 15924 codes. Age is a Unicode version; combining class and
plane are decimal. Unknown properties return null. The primary script and
general-category filters use the independent snapshot.

`normalization.c` shares explicit-length UTF-16 normalization across the
engine, inspector and composer. For the Debian 13 baseline, it uses stable ICU
normalization data, Unicode 17 combining classes and the U+A7F1 compatibility
mapping, and canonical composition with ICU pairs. New scalars without
mappings remain unchanged; embedded NUL is preserved in length-aware calls.
The older ICU path remains for unchanged text. New Unicode 17 simple case
pairs also participate in related-character lookup. Grapheme segmentation and
general case folding still use system ICU. Confusable relations are advisory,
not a security guarantee.

## Offline regeneration

Acquire matching official files from https://www.unicode.org/Public/17.0.0/
and emoji/confusables data for the same release. Verify source versions and
checksums. The property generator is pinned to regex 2026.5.9, FontTools
Unicode 17 script tables and ICU 76. It rejects incompatible inputs and
unexpected assignment counts. Generated headers are checked in; no runtime
network access or Python package dependency is introduced.

```sh
python3 tools/unicode/generate_properties.py native/unicode/generated/properties.h
python3 tools/unicode/update_catalog.py /path/to/ucd-inputs 17.0.0
meson compile -C build
meson test -C build --suite unit --print-errorlogs
```

The catalog input directory must contain `Blocks.txt`, `UnicodeData.txt`,
`emoji-test.txt`, `confusables.txt`, `NameAliases.txt`, and `NamedSequences.txt`.
The wrapper stages the catalog and supplement before replacing outputs and
rejects a target version that differs from the property snapshot. For any
future Unicode release, explicitly update the generator version, expected
counts, name and normalization deltas, and regression cases; changing only a
version string is not sufficient.

Official references:

- https://www.unicode.org/versions/Unicode17.0.0/
- https://www.unicode.org/reports/tr44/
- https://www.unicode.org/Public/17.0.0/ucd/UnicodeData.txt
- https://www.unicode.org/Public/17.0.0/ucd/NameAliases.txt
- https://www.unicode.org/Public/17.0.0/ucd/NamedSequences.txt
- https://www.unicode.org/Public/17.0.0/ucd/NormalizationTest.txt

Priority 3 requires `emoji-variation-sequences.txt` in the catalog input directory
alongside `emoji-test.txt`. Catalog replacement stages and validates the new
presentation bases, selector-omission indexes, and annotation coverage report
before replacing generated catalog files. Full CLDR annotations are updated
separately through `generate_emoji_annotations.py`.

The Vala relationship collector owns Unicode 17 case exceptions, reverse accent
range selection, canonical component assembly, and confusable deduplication.
The ICU adapter supplies raw simple case and mirror mappings and recursive
canonical decomposition (not compatibility mappings). The native catalog only
copies the immutable confusable class. See the
[relationship migration report](unicode-relationship-migration.md).
