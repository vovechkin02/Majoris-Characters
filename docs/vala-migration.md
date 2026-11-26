# Vala migration pass 1 — 1.0.9

## Scope and design

This pass is based on the 1.0.8 Priority 8 source. It changes the private domain
implementation, not the application ID, command-line options, GSettings keys,
XML layouts, clipboard behavior, picker protocol, or Unicode database inputs.
No new runtime dependency or public library ABI is introduced.

| Responsibility | Implementation |
| --- | --- |
| Scalar validation and bounded decoding | `src/unicode/scalar-sequence.vala` |
| All nine exact scalar representations | `src/unicode/sequence-format.vala` |
| Snapshot lifetime, preview, metadata and normalization orchestration | `src/unicode/inspection-snapshot.vala` |
| Composition limits, validation and normalization policy | `src/unicode/composition.vala` |
| Emoji family and relationship keys | `src/emoji/variation-family.vala` |
| Exact alternative collection and presentation ordering | `src/emoji/variants.vala` |
| ICU grapheme count, raw decomposition, explicit-length normalization | `native/unicode/icu-adapter.c` |
| Immutable Unicode 17 property and catalog access, annotation lookup | Existing C database modules |

The old inspector and composition C implementations and headers, their VAPI,
and the native variation collection implementation are removed. The Vala
call sites retain their existing namespace and method contracts. Vala owns
scalar arrays, snapshots and variation strings. The adapter returns explicit
scalar counts, including embedded NUL. Scalar formatting uses a shared bounded
builder; the composition validation-only path does not allocate a scalar array.
Exact variant construction preserves selected text, insertion order, canonical
association and duplicate suppression. No unsupported emoji sequences are
invented. A native adapter is justified by the ICU UTF-16 API and library-owned
operations; no additional Strategy, Bridge or Factory hierarchy is needed.

## Language accounting

Physical handwritten production lines, excluding generated and vendored data,
C headers, VAPI, tests, and scripts:

| Language | 1.0.8 | 1.0.9 |
| --- | ---: | ---: |
| Vala | 5,689 (53.7%) | 6,181 (58.3%) |
| C | 4,909 (46.3%) | 4,426 (41.7%) |

The .gitattributes file classifies only generated and vendored Unicode material
for GitHub Linguist. Handwritten C is not hidden. All existing Unicode source
and generated data, upstream material and notices remain unchanged. These
physical-line statistics are not GitHub's byte-based language percentages.

## Verification in the delivery environment

| Check | Result |
| --- | --- |
| Native Unicode engine contracts | 6 groups passed |
| Native search contracts | 12 groups passed |
| Native emoji alias, related presentation and annotation contracts | 4 groups passed |
| Native legacy encoding contracts | 7 groups passed |
| Native ICU adapter contracts | 3 groups passed; 17,086 direct mappings |
| Native Unicode source compilation | GCC, `-std=gnu11 -O2 -Wall -Wextra -Werror`, ICU 76.1 |
| Python source, input protocol, Unihan importer and emoji data tests | 17 passed |
| Declaration documentation | 1,475/1,475 detected |
| XML and JSON parse, Debian version and source release contracts | Passed |
| Generated, vendored and original Unicode data comparison to 1.0.8 | Byte-identical |
| Migrated Vala domain tests and full application build | Not run |
| Native font backend, GTK/Wayland and Debian binary packaging | Not run |
| Vala performance and allocation parity | Not measured |

The current environment has ICU development libraries and GCC, but no Vala
compiler, Meson, or complete GLib/GTK/libadwaita development toolchain. Debian
mirror access failed DNS resolution. The C regressions do not establish Vala
compilation or behavioral parity. The former C inspector and composition tests
were ported to Vala; emoji family and presentation assertions were relocated to
the Vala variation tests. The separate native ICU test covers the adapter's
length and normalization contracts. The full Meson gate now declares 27 tests.

## Remaining migration work

Establish a complete successful build and run the existing regression suites
before moving more performance-sensitive code. The remaining candidates are the
native search query and filtering policy, language collection resolution, Unihan
search policy, higher-level Unicode relationships, legacy encoding workflows,
and font inventory policy. Retain the immutable generated data, ICU algorithms,
FreeType pixel handling, and narrowly scoped Fontconfig operations unless an
alternative demonstrates equal correctness and performance. Do not force a
numerical language quota or convert generated databases into Vala literals.

On Debian 13, run the existing fail-closed gate:

```sh
python3 tools/qualification/qualify.py build-release --package
```

This is a source migration candidate, not a fully qualified release or a claim
that all native application policy has been migrated.
