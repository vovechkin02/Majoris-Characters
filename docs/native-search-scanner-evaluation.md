# Native search scanner evaluation — 1.0.20

## Scope and design decision

Priority 6 of the revised migration plan evaluates the remaining native scanner.
The common Vala `SearchPlan` now selects the traversal phases and numeric
bounds for all asynchronous and synchronous requests, including the explicit
repertoires used by font and encoding browsing. Preparation must succeed before
the plan is committed. The native contract rejects missing plans, invalid
phase bits, reversed/out-of-range bounds and incompatible repertoire phases.
A required filtered-language mask must be supplied. An explicit empty repertoire
remains empty, not a global search. Scope and filter validation remains at the
native boundary, while the Vala caller owns semantic plan selection.

The native scanner still owns hot predicate evaluation, compact result vectors,
bitset deduplication, recoverable allocations and atomic cancellation. It walks
immutable Unicode property spans instead of binary-searching assignment for
every code point, excludes surrogates, and checks cancellation before each span
and at the existing 256-point interval within included spans. The sequence
scanner borrows validated catalog arrays instead of allocating and decoding
UTF-8 for every emoji or named sequence. It caches the deduplication limit once
per request. Native text accessors for presentation and copy remain unchanged.
No database content or public installed interface changes are intended.

A full Vala rewrite of the hot loops is deferred until the actual mixed-language
build can be compiled and compared. This decision is based on responsibility,
correctness and measured performance, not a source-language percentage quota.
The native static-library/VAPI boundary remains the existing Adapter; no new
abstract backend or per-character GObject is required.

## Native compatibility and timings

GCC with `-O2 -Wall -Wextra -Werror`, ICU 76.1 headers and the installed GLib
2.84 runtime were used on the local Debian 13 container. Official GLib headers,
Vala and Meson were unavailable, so a temporary minimal GLib declaration header
was used. The original 1.0.19 scanner and the revised 1.0.20 scanner were compiled
separately against the same runtime. A temporary native fixture supplied
identical prepared operands and the corresponding traversal plan. One warm-up
was discarded per workload; the median of 3–11 repetitions measures `run()`
only, excluding request preparation, result-signature computation, GTK and
main-context publication. Counts and ordered FNV-1a-style identity signatures
matched in all 19 workloads. Timings are exploratory, not a release threshold.

| Workload | Results | Ordered signature | 1.0.19 ms | 1.0.20 ms |
| --- | ---: | --- | ---: | ---: |
| all | 300543 | `ab5b391bebac0f62` | 21.126 | 2.301 |
| emoji | 3944 | `d57f3fe750bb225b` | 0.183 | 0.056 |
| named | 461 | `dc8950bf87dd220f` | 0.017 | 0.006 |
| emoji French keyword | 111 | `8ed036fbf7202212` | 1.880 | 1.985 |
| emoji group | 2418 | `ddfd4b2e3976bcbf` | 0.128 | 0.038 |
| exact emoji alias | 1 | `c762ed4caec8566f` | 0.178 | 0.050 |
| exact scalar | 1 | `af44214c85cc01cb` | 0.197 | 0.054 |
| global metadata | 11 | `63cddafb66582741` | 133.724 | 109.119 |
| Han reading | 115 | `546f4f8ab105398b` | 29.568 | 10.820 |
| symbols rule | 112 | `f9f61e371b7cfe55` | 22.229 | 5.778 |
| block | 128 | `356c6cc8137514a5` | 0.004 | 0.003 |
| script | 92 | `46367051602f3142` | 22.153 | 5.189 |
| language | 104377 | `c473b60b3bf1ed08` | 23.844 | 5.963 |
| radical | 83 | `90af31cbbe6a011c` | 27.322 | 10.214 |
| strokes | 217 | `8629f66ec3fa4f50` | 25.388 | 10.056 |
| plane | 65534 | `5a1f3c00e00b3116` | 1.290 | 0.429 |
| unassigned | 814730 | `c64b2035a52626c8` | 22.240 | 5.382 |
| category | 137468 | `33a3ccc8f38691a1` | 23.273 | 5.786 |
| explicit repertoire | 5 | `4e1736a6d3bb2c22` | 0.002 | 0.002 |

The fixture uses an empty query for ordinary collections, French locale plus
English fallback for emoji keywords, `euro` for metadata, `mandarin:zhong` for
Han reading, U+20AC and U+2764 U+FE0E for exact inputs, a prepared U+2190–U+21FF
symbol range, Basic Latin block, script index zero, Japanese language mask,
radical index one, stroke index three, private-use plane 15, category index
seven, and explicit points `[20AC, 0041, 0000, 20AC, 1F600, 0378]` in hexadecimal.
The fixture is not the compiled Vala plan. The local French keyword timing was
slightly slower; no universal speedup or complete performance parity is claimed.
The larger traversal improvements are evident, but timings must be repeated on
the intended target.

## Repeatable target benchmark

The optional `benchmark-search` Meson executable measures actual production
Vala preparation plus native execution. Build it on Debian 13:

```sh
meson setup build-benchmark --buildtype=release
meson compile -C build-benchmark benchmark-search
./build-benchmark/tests/benchmark-search
```

It reports the result count, an ordered identity signature, and the median of
seven measured samples following a warm-up. Use identical compiler options and
hardware for revision comparisons. The target is not installed or run as part
of the release gate; absolute timing thresholds would be machine dependent.
The target and the mixed-language tests have not been executed in this container.

## Qualification evidence

| Check | Result |
| --- | --- |
| Native Unicode contracts | 7 groups passed, including every valid scalar category span |
| Native ICU adapter contracts | 5 groups passed; 17,086 decomposition mappings |
| Prepared search contracts | 9 groups passed |
| Native legacy adapter contracts | 3 groups passed |
| Python protocol, source and importer tests | 27 passed |
| Native scanner compatibility | 19 ordered result signatures and counts matched |
| Borrowed sequence checks | 3,944 emoji and 461 named sequences |
| Declaration documentation | 1,722 of 1,722 detected declarations documented |
| Official GLib development-header compilation | Not run |
| Vala, GTK, native fonts and mixed-language tests | Not run |
| Optional production benchmark and Debian binary package | Not run |

All 37 Unicode source, generated and upstream data files are unchanged. The
handwritten production implementation contains 7,812 Vala lines and 3,927 C
lines (66.5% Vala), excluding headers, tests, bindings, and generated or vendor
material. Native lines increased because the retained scanner now includes
bounded traversal and borrowed-data adapter contracts. This is not a regression
in separation of responsibilities. Temporary compilation headers, comparison
fixtures and build outputs are not included in the source archive.

## Follow-up

Run the existing full release qualification gate with the official development
toolchain, then run the optional benchmark and real desktop qualification.
Unicode dataset completeness, desktop integration and packaging remain separate
outstanding activities. No additional scanner migration is recommended without
compiled behavioral and performance evidence.
