# Vala migration pass 6 — 1.0.14

## Scope and ownership

This pass consolidates native adapters after the preceding Vala domain
migrations. It does not rewrite ICU algorithms, change the generated catalog,
or move the unbenchmarked search traversal into Vala.

- `native/scalar-validity.h` supplies one scalar validity predicate for the
  Unicode engine, property lookup, normalization, Fontconfig and FreeType.
  It admits valid unassigned scalars and noncharacters and excludes surrogates
  and values above U+10FFFF. Font coverage and assignment remain separate
  policies. The header requires no ICU or Unicode catalog linkage.
- `native/unicode/icu-scalar-conversion.h` provides bounded UTF-8 decoding,
  explicit-length UTF-16 decoding, and explicit-length UTF-16 encoding into
  caller-owned storage. They are inline native operations, with no heap
  allocation or additional callbacks in search and encoding loops. Failure
  returns zero; callers must discard partial output. Embedded NUL is preserved
  by UTF-16 conversions, while the C UTF-8 string interface cannot represent
  it. Invalid UTF-8, isolated surrogates, and insufficient capacity are rejected.
- `icu-adapter.c` owns catalog scalar-to-text encoding and UTF-8 normalization
  in addition to direct decomposition, grapheme counting and explicit-length
  normalization. The existing `characters_unicode_*` symbols and VAPI
  contracts are unchanged; `unicode-engine.h` includes their owning adapter
  header so existing callers still see the same declarations. Both
  normalization entry points continue using the same Unicode 17-aware engine.
- The Unicode engine uses one recursive canonical decomposition retrieval
  helper for related-character candidates. Immediate decomposition remains a
  distinct ICU operation.
- The FreeType symbol map's borrowed array accessor is private. Indexed raw
  symbol count/code access, cmap selection and rendering are unchanged.
  Fontconfig discovery, selected face identity and renderer configuration
  remain in the existing native adapters.

The Vala application domain and existing GoF Adapter boundary are retained.
No new VAPI, runtime dependency, UI, settings key, database format or public
application protocol is introduced. The inline helpers consolidate source
logic, not runtime state. The normalization streaming loop and native scanner
retain their existing allocation, cancellation and result ordering models.

## Verification in the delivery environment

| Check | Result |
| --- | --- |
| Native Unicode engine | 6 groups passed |
| Native ICU adapter | 4 groups passed (including bounded conversion) |
| Prepared search | 6 groups passed |
| Native legacy adapter | 3 groups passed |
| Native Unicode compilation | GCC, ICU 76.1, `-Wall -Wextra -Werror` |
| Direct decomposition | 17,086 mappings validated |
| Python source, protocol and generator checks | 22 passed |
| Declaration documentation lexical check | 1,595 of 1,595 detected declarations |
| Ordered catalog comparison with 1.0.13 | 300,543 identities, matching hashes of IDs, text, display, name and code |
| Supplementary emoji comparison | 1,819 identities, matching hashes of exact text, name, candidate flag and alias index |
| Encoding comparison | 77 ordered scalar repertoires and sampled packed byte keys, matching hashes |
| Unicode data, generated and upstream sources | Byte-identical to 1.0.13 |
| Font adapter C compilation and tests | Not run; development headers unavailable |
| Full Vala/GTK build, mixed-language tests and Debian binary package | Not run; toolchain unavailable |
| Performance parity | Not measured |

The comparison was a temporary native harness against both versions, not a
compiled Vala integration test. The focused shared conversion tests exercise
NUL, noncharacters, supplementary and unassigned scalars, malformed UTF-8,
isolated surrogates and insufficient output capacity. The existing Meson ICU
adapter test target includes the new contract group. The release source
contract checks the relocated API and shared adapter use. No separate
redundant test executable was introduced. The documentation checker now also
scans project-owned headers directly under `native/`.

This is a source migration candidate. Establish a clean Debian 13 build and
run the existing qualification command before declaring the migration verified:

```sh
python3 tools/qualification/qualify.py build-release --package
```

## Remaining migration decisions

Keep compact immutable catalog access, native ICU conversion and normalization,
Fontconfig and FreeType operations, and the prepared native scanner. Do not
migrate the hot scanner or introduce an extra abstraction solely for language
statistics. First compare runtime, memory and cancellation latency on a
compiled baseline. Fix actual compile or behavioral defects before any
additional rewrite. Full desktop/ISO integration and official data coverage
are separate remaining product activities.
