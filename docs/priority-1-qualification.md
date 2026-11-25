# Priority 1 qualification — Unicode data completion

Version 1.0.1 is a source qualification candidate based on Stage 10. The target
Unicode release remains 17.0.0, matching the supplied GNOME catalog. Debian 13
continues to use system ICU 76; no unstable dependency or runtime download is
required.

## Changes and observed coverage

- Independent, immutable run-length encoded scalar property snapshot: 297,334
  assigned scalars (including private use and controls); 300,543 distinct
  browseable identities after existing deduplication.
- Full assigned-repertoire name check: zero missing canonical names among
  assigned scalars for which names are required; controls/private use excluded.
- New script, script-extension, age and category information is exposed by the
  existing engine and inspector. Primary script browsing uses this snapshot.
- Eight verified alias additions bring the supplement to 481 alias records.
  The 461 existing named-sequence records are retained after release review.
- Shared normalization handles the Unicode 17 combining-class delta and new
  compatibility mapping; new simple case pairs participate in related lookup.
- Catalog refresh stages the alias and named-sequence files alongside other
  catalogs and checks the snapshot version before replacing any output.

## Executed checks

| Check | Result |
| --- | --- |
| Native Unicode engine | 6 groups passed |
| Character search | 7 groups passed |
| Character inspector | 5 groups passed |
| Legacy encodings | 4 groups passed |
| Emoji catalog | 4 groups passed |
| Composition | 4 groups passed |
| GCC native build (`-Wall -Wextra -Werror`) | All six suites passed |
| ASan, UBSan and leak detection | Five native suites passed; composition sanitizer build did not finish within the time limit |
| Python input protocol and release source contracts | 7 tests passed |
| Property and supplement regeneration | Byte-for-byte comparison passed |
| Declaration documentation checker | 1,110 / 1,110 detected declarations |
| Python AST and XML syntax | Passed |
| Debian changelog version | 1.0.1-1 |

The native suites use ICU 76.1. No claim of full official Unicode conformance
is made: the complete official UCD archive and NormalizationTest.txt could not
be acquired. The property generator relies on pinned Unicode 17 library data;
the alias and named-sequence files have not been compared in full against the
matching official distribution. The catalog regeneration wrapper was not run
against the missing official source files. Full Meson/Vala/GTK compilation,
font backend qualification, schema compilation, Debian package building and
desktop integration tests remain unrun in this environment. See
[Unicode engine](unicode-engine.md) for exact provenance and update procedure.

Unihan, script-extension matching in browsing, complete emoji annotations,
standardized emoji variation data and other later priorities are unchanged.
