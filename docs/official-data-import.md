# Official Unihan and CLDR data import — 1.0.22

## Status

Priority 2 dataset acquisition and integration are complete. Both original
archives supplied for this release matched their pinned digests. The manifest
at `data/unicode/official-data.json` reports `imported`; no network access or
synthetic production records were required. The existing fallback Han snapshot
is retained byte-for-byte and its provenance remains separate.

| Data | Bundled in 1.0.22 |
| --- | ---: |
| Official Unihan version | 17.0.0 |
| Official Han records | 102,998 |
| Retained fallback records | 98,012 |
| Combined unique Han records | 103,039 |
| Official supported property values | 397,196 |
| CLDR version | 48 |
| Annotation locale tables | 171 |
| Resolved annotation records across locale tables | 613,513 |
| English direct catalog coverage | 3,944 of 3,944 |
| Locale tables with complete direct or tone-base coverage | 91 of 171 |

The complete archive import is not a claim that each locale translates every
emoji or each Han character has every property. The supplied release contains
sparse annotations and absent readings. The application preserves these gaps
and retains its existing locale and English fallback behavior. All 24 previously
supported locale tags remain available; 20 now have complete catalog coverage.
Danish direct-or-tone-base coverage decreases from 3,691 to 3,679 because
CLDR 48 has a different vocabulary and gaps. The previous GTK annotations are
not silently merged with the verified CLDR source; English fallback still
applies. Localized keywords can legitimately differ between releases. The full
Unihan archive is retained; the generated runtime database uses the ten
formal fields supported by the application, not every property in Unihan.

## Official Han property counts

| Field | Present values |
| --- | ---: |
| `kCantonese` | 29,936 |
| `kDefinition` | 23,285 |
| `kHangul` | 8,525 |
| `kJapanese` | 51,583 |
| `kJapaneseKun` | 11,296 |
| `kJapaneseOn` | 13,177 |
| `kKorean` | 9,050 |
| `kMandarin` | 44,348 |
| `kRSUnicode` | 102,998 |
| `kTotalStrokes` | 102,998 |

## Pinned source inputs

Use the versions matching the current Unicode 17 character and emoji catalog,
not the latest unversioned download links.

- [Unicode 17.0.0 Unihan.zip](https://www.unicode.org/Public/17.0.0/ucd/Unihan.zip)
  (8,518,517 bytes). The pinned SHA-256 is
  `f7a48b2b545acfaa77b2d607ae28747404ce02baefee16396c5d2d7a8ef34b5e`,
  corroborated by the original 17.0.0 entry in
  [FreeBSD textproc/UCD distinfo](https://cgit.freebsd.org/ports/commit/?id=24225ab97b817a7a301ba0feb13e5b8e674233c5).
- [CLDR 48 common archive](https://www.unicode.org/Public/cldr/48/cldr-common-48.zip).
  The SHA-512 pin comes from Unicode's
  [published checksum file](https://www.unicode.org/Public/cldr/48/hashes/SHASUM512.txt).
  CLDR 48 includes Unicode 17 emoji annotations; see the
  [release notes](https://cldr.unicode.org/downloads/cldr-48).

Both releases use the Unicode License V3. The project retains its license
notice and the existing fallback source licenses. The pins identify the
original archive bytes; a recompressed archive will not satisfy them.

## Import

To refresh from the same releases, obtain the two original archives in an
external working directory. Run this from the Characters repository root:

```sh
python3 tools/unicode/import_official_data.py \
  --unihan /path/to/Unihan.zip \
  --cldr /path/to/cldr-common-48.zip
python3 tools/unicode/import_official_data.py --verify
```

The workflow validates both digests, checks the Unicode catalog version,
recognizes the Unicode 17 Unihan version header, checks the CLDR LDML DTD
version, and rejects incomplete supported fields or missing prior locales.
It reads archive members directly or extracts only allowlisted paths inside
a temporary directory. All outputs are generated before any destination is
replaced. The manifest is published last and records the source and generated
file hashes. Publication is not a cross-file filesystem transaction; run
`--verify` after an interrupted import and rerun import if necessary.

The workflow imports all ten supported formal Han fields into the current
immutable database. Fallback readings and collation records remain separate.
CLDR base and derived annotations are imported together; explicit parent
locale declarations are applied to missing name and keyword fields. Root
internal identifiers and inheritance markers are excluded from searchable
words. This is annotation-specific resolution, not a general LDML processor;
locale aliases, synthesis beyond provided derived annotations, and the
application's existing English and script fallback behavior remain distinct.
Actual missing coverage is recorded rather than artificially filled.

Generated output and source snapshot locations:

| Path | Purpose |
| --- | --- |
| `data/unicode/han/official-unihan.zip` | Original verified archive |
| `native/unicode/generated/unihan.h` | Formal and fallback data |
| `data/unicode/emoji/annotations.json.gz` | Resolved imported annotation snapshot |
| `native/unicode/generated/emoji-annotations.h` | Native lookup table |
| `data/unicode/emoji/coverage.json` | Per-locale catalog coverage |
| `data/unicode/official-data.json` | Source pins, coverage, output checksums |

The original CLDR archive remains an external input; the compressed snapshot
retains the imported annotation data for local regeneration. No network or
new runtime dependency is introduced into Meson or the application.

## Regeneration and qualification

Once imported, the normal Han regeneration command uses the retained official
archive automatically. The normal emoji regeneration command uses the updated
compressed snapshot. Both produce the same tables from the same inputs and
Python Unicode database version. Use the complete paired import to update
provenance and verify the final outputs before publishing a release.

```sh
python3 tools/unicode/generate_unihan.py data/unicode/han/fallback.json.gz \
  native/unicode/generated
python3 tools/unicode/generate_emoji_annotations.py \
  --coverage data/unicode/emoji/coverage.json
python3 tools/unicode/import_official_data.py --verify
python3 tools/qualification/qualify.py build-release --package
```

## Qualification evidence

- Both original archive checksums matched the pins. Import and `--verify` passed.
- Independent enumeration of the compiled native Han accessor matched all
  397,196 supported formal property values in the original Unihan files.
- Regeneration of both native data tables matched the published output hashes
  or bytes; Python 3.13 with Unicode database 15.1.0 was used for generation.
- The native Unicode, ICU adapter, prepared search, and legacy adapter suites
  passed, including new official field and localized annotation assertions.
- The official-data tests now verify the shipped archive and regenerate the
  full Han table; the emoji snapshot test checks all 171 tables and coverage.
- The generated emoji header is approximately 88 MiB, and the generated Han
  header is approximately 18 MiB. The expanded data increases compilation,
  binary storage and potentially memory requirements. The local native emoji
  translation unit compiled in about 15 seconds and peaked at approximately
  865 MiB RSS. These are local measurements, not target package benchmarks.
  No runtime data format or lookup algorithm was changed in this pass.
- The original CLDR archive is not duplicated in the repository. The imported
  compressed snapshot and verified source digest are retained for offline
  regeneration; the retained Unihan archive supports independent Han auditing.
- GCC compiled all native Unicode modules using ICU 76.1 headers and temporary
  minimal GLib declarations linked against the real installed GLib 2.84 runtime.
  Official GLib development headers, the Vala compiler, Meson, and the complete
  GTK toolchain are unavailable here. Full compilation, mixed-language tests,
  desktop interaction, package building and end-to-end performance qualification
  remain outstanding. The temporary declarations and binaries are not shipped.

The import remains annotation-specific rather than a complete LDML processor.
Unicode 17 grapheme and case-folding consistency, official UCD conformance,
interface translation coverage, and desktop integration remain separate tasks.

As of 1.0.23, both derived search headers are regenerated with the pinned
Unicode 17 folding rules rather than the host Python Unicode version. The
original Unihan archive, CLDR snapshot and coverage report are unchanged; the
manifest records the updated generated-header digests. The historical
qualification above describes 1.0.22.
