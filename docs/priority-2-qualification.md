# Priority 2 qualification — 1.0.2

## Executed checks

- Compiled all native Unicode C modules with GCC, `-std=gnu11 -Wall -Wextra
  -Werror -O2`, against Debian 13 ICU 76.1.
- Passed six native suites: Unicode engine (6 groups), search (10), inspector
  (5), legacy encodings (4), emoji (4), composition (4): 33 groups total.
- Search tests include shared Japanese script characters, locale and configured
  keyboard language collections, tone-insensitive Mandarin and Korean readings,
  CJK field restrictions, and combined radical/stroke filters. Global counts
  remain 297,334 assigned scalars and 300,543 distinct browseable identities.
- Passed Python input protocol (3), release source contracts (4), and offline
  Unihan importer tests (3): 10 tests total. Official importer tests use synthetic
  isolated fixtures, not the complete official dataset.
- Checked 1,201/1,201 lexically detected declarations for documentation.
- Reproduced the two generated headers from the pinned snapshot byte-for-byte.
- Passed strict GSettings schema dry-run, Python/XML/JSON syntax checks and
  Debian changelog parsing (1.0.2-1).

## Outstanding checks and limitations

The official Unicode 17 Unihan archive could not be retrieved from this
execution environment. Formal Cantonese/Japanese readings, definitions,
radical-stroke and total-stroke properties are supported by the importer and
consumer code but absent from the bundled fallback. See international-browsing.md
for precisely labeled provenance and limits. The fallback supports Mandarin and
Hangul readings and partial ICU radical and stroke collation grouping. Language
collections are script unions, not exact orthographies; non-XKB input engines
are not resolved. The existing Unicode 17 normalization conformance gap remains.

Meson, valac, GTK4/libadwaita development dependencies and font dependencies
are unavailable. Full application compilation, GTK interaction and IBus/Shell
runtime checks, Debian binary packaging, and sanitizer reruns are not performed
for this delivery. No claim of release qualification is made. The updated
Meson inventory includes the new importer tests. Run the release gate on Debian
13 and verify language/radical/stroke navigation, settings visibility, live
XKB source changes, and inspector metadata copying on the real desktop.
