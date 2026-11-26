# Vala migration pass 3 — 1.0.11

## Scope and design

This source migration extends 1.0.10 without changing the application identity,
command-line options, settings, installed paths, UI layouts or Unicode datasets.

| Responsibility | Implementation |
| --- | --- |
| Semantic collection classification and curated ranges | `src/search/semantic-filters.vala` |
| Han query prefix recognition, field selection and numeric validation | `src/search/han-query.vala` |
| Preparation and worker ownership | Existing `QueryParser` and `SearchRequest` |
| Bounded predicate execution and compact result storage | `native/unicode/character-search.c` |
| Immutable Han record access and ICU reading folding | `native/unicode/unihan.c` |

`SemanticFilters` builds a bounded union of four-word clauses (general-category
mask, decomposition-type mask, first scalar, last scalar) for the selected
symbol collection. A zero decomposition mask imposes no decomposition
constraint. The scanner copies these clauses, validates their size, masks and
ranges, and evaluates them with a single category lookup and lazy decomposition
lookup per candidate. The unfiltered Symbols union preserves the former
classification rule, rather than indiscriminately merging all category results.
The 27 stable collection slots and labels are unchanged. Whitespace uses the
Unicode 17 White_Space ranges, equivalent to the previous ICU binary property.

`HanQuery` folds each term once using the existing bounded ICU adapter. It then
selects the immutable textual field mask or parses a bounded positive radical
or stroke number. An invalid explicit field is represented by a never-matching
predicate, not treated as ordinary metadata or an invalid search request.
Leading zeroes, tone-insensitive matching, fallback readings and AND semantics
are retained. Unknown prefixes continue to be ordinary metadata terms. The C
scanner does not parse Han query prefixes or store curated symbol ranges.
Numeric group membership still interprets official stored radical/stroke data
in the native data adapter, preserving the distinction from collation fallback.

All arrays are copied before execution, bounded to 128 semantic clauses and
64 Han terms. The worker owns the prepared request; only atomic cancellation
may run concurrently. The native scanner still implements broad general-category
lookup, scope traversal, exact identity merging, and field substring matching.
No per-scalar Vala callback or GObject is allocated. This is a focused Adapter
boundary, not an additional hierarchy or runtime dependency. Native enum values
and field bit positions are part of the private prepared-request contract.

## Language accounting

Physical handwritten production lines, excluding generated/vendor material,
headers, VAPI, tests, and scripts:

| Language | 1.0.10 | 1.0.11 |
| --- | ---: | ---: |
| Vala | 6,507 (60.7%) | 6,815 (62.6%) |
| C | 4,220 (39.3%) | 4,066 (37.4%) |

All 37 source, generated and upstream Unicode data files remain byte-identical.
The generated/vendored GitHub Linguist classifications remain unchanged.

## Verification in the delivery environment

| Check | Result |
| --- | --- |
| Native Unicode engine contracts | 6 groups passed |
| Native ICU adapter contracts | 3 groups passed, including 17,086 direct mappings |
| Prepared native request contracts | 6 groups passed, including new semantic and Han descriptor checks |
| Native search compatibility via temporary C harness | 12 groups passed, including all 27 semantic collections and malformed Han terms |
| Legacy compatibility via the same temporary harness | 7 groups passed |
| Semantic comparison versus 1.0.10 | All 28 result counts and ordered identity hashes matched (27 collections plus union) |
| Native C compilation | GCC `-std=gnu11 -O2 -Wall -Wextra -Werror`, ICU 76.1 |
| Python source, input protocol and Unicode generator tests | 19 passed |
| Declaration documentation | 1,548/1,548 detected |
| Full mixed Vala/C regression and Vala/GTK application compilation | Not run |
| Native font backend, desktop runtime and Debian binary packaging | Not run |
| Vala behavior, performance and allocation parity | Not measured |

The compatibility harness is outside the project and not shipped. It uses the
previous C parser and a temporary translation of the new prepared rule policy,
including ranges extracted from the Vala source. It exercises the revised C
scanner and data access, not compiled Vala. The actual Meson test targets use
only the production Vala policy. The 28 semantic comparison results preserve
16,339 identities in the union and the count and order of each individual
collection. The ordinary catalog still contains 297,334 assigned scalars and
300,543 browseable identities. No new test target is necessary; the existing
prepared request and search suites cover the new boundaries.

The delivery environment has GCC and ICU 76.1, but lacks Vala, Meson and the
complete GLib/GTK/libadwaita development toolchain. This remains a source
migration candidate rather than a fully compiler-qualified release. On Debian
13, run the existing fail-closed build and test gate:

```sh
python3 tools/qualification/qualify.py build-release --package
```

## Remaining migration

Establish a successful target build and run the actual mixed-language tests
before proceeding. The next separable migration is legacy encoding workflow
policy (formatting, input parsing and conversion orchestration) followed by
font inventory policy and adapter consolidation. Keep the native ICU converter,
large repertoire enumeration, compact search scanner and FreeType rasterizer
until a replacement is justified by measured correctness and performance.
