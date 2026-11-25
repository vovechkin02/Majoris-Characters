# Priority 5 qualification — 1.0.5

## Implemented source

- Existing 35 encoding names and indexes preserved; 42 additional entries
  appended. All 77 converters loaded successfully with ICU 76.1.
- Independent scalar mapping enumeration retains byte alternatives. EUC-JP,
  GB18030-2022 and EUC-TW use explicit three-/four-byte grammars rather than
  enumerating arbitrary 32-bit byte patterns. GB18030 exposes 1,112,045 valid
  mapped scalars with the tested converter, including supplementary and
  unassigned values; EUC-JP 13,336 and EUC-TW 62,377.
- Stateful ISO-2022, HZ and contextual ISCII expose a verified standalone
  round-trip repertoire, with canonical encoded bytes computed on demand.
- Exact byte streams decode to ordered scalar arrays, including contextual
  multi-scalar mappings and NUL. Reverse conversion verifies identical scalar
  output and rejects non-round-trip results. Each call starts with a fresh
  converter; original decode input is retained separately from canonical output.
- XML byte-sequence dialog with explicit numeric and literal encode actions,
  original-byte copying, edit invalidation, and shared Unicode inspection.
- Existing background source-loading and cancellable repertoire-search paths
  are reused. No new build or runtime dependency.

## Checks executed

| Check | Result |
|---|---|
| Native Unicode engine | 6 groups passed |
| Native character search | 10 groups passed |
| Native character inspector | 5 groups passed |
| Native legacy encodings | 7 groups passed |
| Native emoji catalog | 7 groups passed |
| Native composition | 4 groups passed |
| Python input protocol | 3 tests passed |
| Python Unihan importer | 3 tests passed |
| Python emoji data | 5 tests passed |
| Python release source contracts | 4 tests passed |

All ten native Unicode source files compiled with GCC GNU11, `-O2 -Wall -Wextra
-Werror` against ICU 76.1; all six suites passed (39 groups). The encoding suite
was also rebuilt with all native Unicode sources instrumented using ASan and
UBSan, with leak detection enabled; all seven groups passed. Tests cover every
encoding's repertoire, original known mappings, four-byte supplementary values,
three-byte EUC-JP, EUC-TW planes, ISO-2022 state changes, HZ, ISCII virama/joiner
sequences, state reset, strict malformed/truncated/lossy rejection, NUL,
maximum-length conversions, and the full GB18030 search repertoire.

The 15 Python tests passed. The declaration documentation checker, XML template
and resource references, accessible-control naming, Python syntax, Debian
version metadata and archive integrity were checked. These source checks do not
substitute for compiler or runtime validation.

## Not run / outstanding

The available environment does not contain valac, Meson, the GTK4/libadwaita
and native font development packages, or the GLib schema/resource tools. Full
application compilation, Vala binding tests, native font tests, GTK dialog
interaction, schema/resource compilation and Debian binary package build were
not run. No GUI, desktop, or package qualification success is claimed. New
Vala binding and GUI interaction cases are registered through the existing
Meson suites. Run `python3 tools/qualification/qualify.py build-release --package`
on the target Debian 13 system.

Contextual byte histories and multi-scalar outputs are inspected by explicit
conversion rather than exhaustively enumerated in the scalar grid. Encoding
coverage follows ICU's supported mapping variants, not all historical code
pages. Source enumeration runs on the existing worker and is not interruptible;
superseded results are suppressed. No performance benchmark or memory-peak
measurement was performed. Full Unicode/Unihan/CLDR data conformance, earlier
font and desktop qualification, and external ISO integration remain outstanding.

Next functional priority: structured decomposition, keyboard entry guidance,
Unicode plane and unassigned browsing, and semantic category refinements.
