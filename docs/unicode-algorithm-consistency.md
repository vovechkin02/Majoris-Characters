# Unicode 17 algorithm consistency — 1.0.23

## Implementation

Revised Priority 3 pins the default extended grapheme and case operations to
Unicode 17 instead of accepting the host ICU 76 or Python Unicode version as
an implicit policy. The existing immutable scalar snapshot and normalization
engine remain in place. No application data, clipboard identity, serialization,
installed API, preference, or runtime dependency changes.

- The native algorithm adapter implements UAX #29 default extended grapheme
  rules with fixed left-context state, including CR/LF, controls, Hangul,
  extending marks, spacing marks, prepend, Indic conjuncts (GB9c), emoji ZWJ
  context (GB11), and regional indicator pairing. It validates UTF-8 and
  supports explicit scalar arrays containing NUL for qualification.
- Simple case mappings are immutable rather than delegated to an unknown
  system case table. Unicode 17 adds 28 pairs (25 Beria Erfe and three Latin
  pairs). The Vala relationship service no longer duplicates these exceptions.
- Default full case folding retains multiscalar expansions and non-Turkic
  dotted/dotless I semantics. NFKC_Casefold maps each scalar through the
  pinned NFKC_CF table and applies NFC using the existing Unicode 17-aware
  normalization engine. Default ignorables are removed only for the latter
  operation, never from exact copied text.
- Han query and index folding share full case folding, NFD, and removal of
  the four existing tone marks only in Latin-script context. Emoji query and
  index folding share NFKC_Casefold. Vala saved-collection and font-label
  filtering use full case folding rather than system GLib's Unicode version.
- ICU retains its stable normalization, mirror, name and encoding primitives.
  Canonical normalization still uses the established ICU 76 baseline and
  snapshot combining classes, including the 34 Unicode 17 combining-class
  additions and the U+A7F1 compatibility mapping. No claim is made that every
  ICU-dependent operation has been independently qualified for all UCD versions.

The generated header is a private table adapter with GLib types and recoverable
allocation. Segmentation is a single forward pass with bounded state and binary
property lookup. The Vala domain remains responsible for presentation and
selection; this is not a new extensible backend or pattern hierarchy.

## Source data and regeneration

The checked-in JSON and C table are generated from the same pinned regex
2026.5.9 Unicode 17 tables used by the existing scalar property snapshot, with
stable ICU 76.1 normalization/simple-case data and explicit Unicode 17 changes.
The full fold table contains 1,585 mappings; NFKC_CF contains 10,583 mappings
(including deletion mappings); 2,989 scalar records have nonidentity simple
case mappings; 1,631 ranges cover the nondefault packed grapheme properties.
The original UCD ZIP and official test files could not be downloaded in this
container. This is a derived version-pinned snapshot, not a claim that every
row has been compared to the original UCD. The source reference is
https://www.unicode.org/Public/17.0.0/ucd/ and the Unicode license is retained.

Regenerate the algorithm snapshot in the pinned development environment:

```sh
python3 tools/unicode/generate_algorithms.py
```

The index generators consume the checked-in JSON and pinned ICU 76 canonical
normalization through Python ctypes. They do not require regex to regenerate
indexes or run the standard unit suite. The complete project ships generated
headers; ordinary compilation does not generate data or access the network.
On Debian 13, ICU 76 is already available through the existing dependency.
The original official Unihan archive and CLDR 48 compressed annotation snapshot
and coverage report are byte-identical to 1.0.22. Both derived search headers
were regenerated and their SHA-256 values updated in the official data manifest.

## Verification

| Check | Result |
| --- | --- |
| Native Unicode engine | 7 groups passed |
| Native ICU/Unicode adapter | 7 groups passed (including 17,086 direct mappings) |
| Prepared search | 9 groups passed |
| Legacy adapter | 3 groups passed |
| Python protocol, import, source and algorithm tests | 35 passed |
| Extended native differential checks | 127,551 cases passed |
| Offline native header regeneration | Byte-identical |
| Official Unihan and CLDR retained inputs | Unchanged |
| Official UCD normative corpus | Not run; local original files unavailable |
| Official GLib development-header and full Vala/GTK build | Not run |
| Mixed-language, package and performance qualification | Not run |

The native build used GCC, `-O2 -Wall -Wextra -Werror`, ICU 76.1 headers and
minimal temporary GLib declarations linked to the installed real GLib runtime.
The temporary headers and binaries are not shipped. The full toolchain remains
unavailable. Differential checks compare grapheme boundary prefixes to the
pinned regex engine, all stored full/NFKC folds to snapshot mappings, and
sampled actual annotation inputs to the Python index-folding policy. These
checks are not the independent official UCD conformance corpus.

## Official corpus qualification

The optional probe and driver are not installed and do not run on every unit
suite invocation. On Debian 13:

```sh
meson setup build-conformance --buildtype=release
meson compile -C build-conformance unicode-conformance-probe
python3 tests/extended/test_unicode_conformance.py \
  build-conformance/tests/unicode-conformance-probe
```

The differential mode requires regex 2026.5.9. For independent normative
qualification, extract the original Unicode 17 UCD archive locally and run:

```sh
python3 tests/extended/test_unicode_conformance.py \
  build-conformance/tests/unicode-conformance-probe --ucd /path/to/ucd
```

This mode checks GraphemeBreakTest.txt, CaseFolding.txt, DerivedNormalizationProps.txt
and NormalizationTest.txt. The actual Vala/GTK build and package gate remain
separate (`python3 tools/qualification/qualify.py build-release --package`).
The next functional priority is IBus-aware international browsing.
