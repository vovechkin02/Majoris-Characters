# Vala migration pass 2 — 1.0.10

## Scope and responsibility

This pass extends the 1.0.9 source migration. It does not alter the installed
application identity, command-line options, GSettings keys, desktop integration
protocols, XML layouts, or source and generated Unicode databases.

| Responsibility | Implementation |
| --- | --- |
| Bounded query interpretation, numeric syntax, exact and ambiguous identities, term splitting | `src/search/query-parser.vala` |
| Named, locale and configured XKB collection policy | `src/search/language-selection.vala` |
| Related-character input validation, component grouping, presentation alternatives, deduplication and ordering | `src/unicode/relationships.vala` |
| Request lifetime, worker preparation, cancellation and publication | Existing Vala `SearchRequest` and `SearchController` |
| Prepared query storage, Han term folding, repertoire traversal, metadata matching, semantic filters and compact result vectors | `native/unicode/character-search.c` |
| Immutable language registry and ICU script lookup | `native/unicode/language-collections.c` |
| Raw scalar candidates from case, mirror, canonical decomposition, accent and confusable lookups | `native/unicode/unicode-engine.c` |

The old native raw-query parser, explicit-code parser, language selection
policy, and complete related-text assembly are removed. Numeric decoding is
shared by search and inspector rather than duplicated. The search worker
interprets the query before handing a copied, bounded representation to the
native scanner. The scanner owns all retained arrays and the atomic cancellation
flag. Its result vector and deduplication bitset retain their existing compact
representation. Only cancel may run concurrently with worker preparation and
scanning. The Vala relationship service owns its text results and preserves
binary UTF-8 ordering; the native engine returns raw integer candidates rather
than allocating a string for every possible relationship.

These are direct domain functions and focused database/library adapters.
The existing asynchronous Observer contract is retained. There is no new
Strategy hierarchy, installed ABI, GObject per scalar, or runtime dependency.
The C scanner is intentionally retained pending actual Vala performance and
allocation measurements, rather than being rewritten solely to change language
statistics. Configured keyboard browsing continues to support XKB sources;
IBus engine language discovery is separate functional work.

## Language accounting

Physical handwritten production lines (excluding generated and vendored
material, headers, VAPI, tests and scripts):

| Language | 1.0.9 | 1.0.10 |
| --- | ---: | ---: |
| Vala | 6,181 (58.3%) | 6,507 (60.7%) |
| C | 4,426 (41.7%) | 4,220 (39.3%) |

The GitHub Linguist generated/vendored classifications remain unchanged. All
37 original Unicode data, generated and upstream files compared byte-identical.

## Verification in the delivery environment

| Check | Result |
| --- | --- |
| Native Unicode engine contracts, including scalar relationship candidates | 6 groups passed |
| Native ICU adapter contracts | 3 groups passed, including 17,086 direct decompositions |
| Prepared native search boundary | 4 groups passed |
| Native scanner compatibility using a temporary harness derived from the 1.0.9 C parser | 12 original search groups passed; 297,334 assigned scalars, 300,543 browseable identities |
| Legacy encoding compatibility using the same temporary harness | 7 original groups passed; 77 encodings |
| Native Unicode C compilation | GCC, `-std=gnu11 -O2 -Wall -Wextra -Werror`, ICU 76.1 |
| Mixed search, encoding and emoji fixture C source compilation | Passed; not linked against Vala |
| Python source, input protocol, Unihan importer and emoji data tests | 18 passed |
| Declaration documentation | 1,510/1,510 detected |
| XML/UI/JSON source parsing, Debian version and release contracts | Passed |
| Full mixed Vala/C fixtures and migrated Vala relationship and parser tests | Not run |
| Full Meson, Vala/GTK, native font backend and Debian binary build | Not run |
| Vala behavior, performance and allocation parity | Not measured |

The temporary compatibility harness was outside the source tree and is not
part of the delivered project. It confirms the new native scanner when given
prepared values produced by the original parser, but does not exercise the
new Vala parser or language selection. The production and mixed fixture targets
now use only the Vala parser. The original relationship membership and ordering
assertions are retained in the Vala binding tests, with additional NUL and
component assertions. The native prepared-request tests exercise copying,
validation, cancellation, script masks and Han terms. All 28 Meson test
registrations remain covered by the fail-closed release log gate.

The environment has ICU and GCC but no Vala compiler, Meson or complete
GLib/GTK/libadwaita development toolchain. Debian mirror DNS resolution failed.
This is a source migration candidate, not a compiler-qualified or stable release.

## Remaining migration

After compiling and running all existing tests on Debian 13, profile the
remaining native scanner and relationship lookups. Migrate separable semantic
filter and Unihan field policies, legacy conversion workflows, and font
inventory policies only when their correctness and performance are established.
Keep generated tables and the narrowly scoped ICU, FreeType and Fontconfig
interop where justified. Do not force a fixed language percentage.

```sh
python3 tools/qualification/qualify.py build-release --package
```
