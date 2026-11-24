# Stage 10 qualification

Version: 1.0.0 (Debian 1.0.0-1), qualification candidate. Target: Debian 13 /
Majoris OS. The supplied Stage 9 source is the baseline. No prior test suites
or feature modules were removed.

## Source changes

- Compact picker observes the shared grid preview size, sizes its results
  accordingly, and disconnects the observer when closed. Local collection
  filtering folds the query once rather than once per candidate.
- Input searches use a bounded EOF-terminated UTF-8 stdin protocol instead of
  exposing typed search text in subprocess arguments. The original CLI remains
  compatible. Modified keystrokes cancel the picker instead of inadvertently
  committing a candidate. Sensitive-context checks also guard publication and
  commit. Explicit insertion history still passes the selected identity as a
  command-line argument.
- The picker result collection has an explicit accessible name. Source checks
  enforce accessible labels or native titles for interactive XML controls.
  Real screen-reader and keyboard behavior still require runtime testing.
- A partial French interface catalog contains 203 translated messages, including
  common controls, collection labels, and desktop launch labels. Other UI
  strings fall back to the original English. Canonical Unicode names and
  technical identifiers remain unchanged. The desktop file is merged with the
  gettext catalog at build time; AppStream name and summary have French entries.
- New release-source tests check template field types and resource paths,
  localization inputs, accessibility labels, and installed identity metadata.
  The qualification tool requires a fresh Meson build and fails for missing,
  skipped, or non-OK test results. Its log parser is tested with synthetic logs;
  these are not actual Meson execution results.
- The target desktop and ISO checklist documents integration requirements
  without modifying the separate Desktop Shell or ISO builder repositories.

## Executed checks

| Check | Result |
| --- | --- |
| Six native Unicode/ICU suites (29 groups) | Passed |
| GCC GNU11, `-O2 -Wall -Wextra -Werror` | Passed |
| Six native suites under AddressSanitizer, UndefinedBehaviorSanitizer and leak detection | Passed |
| Input response protocol (3 Python cases) | Passed |
| Release source contracts and fail-closed log parser (4 Python cases) | Passed |
| Strict GSettings schema dry-run | Passed using installed GLib schema compiler |
| French catalog parse, format checks and in-memory MO lookup | Passed using available Babel and Python gettext; GNU msgfmt not run |
| Declaration documentation lexical check | 1,072/1,072 detected declarations documented |
| XML, JSON, Python syntax and template references | Passed |
| Shell extension JavaScript syntax | Passed using Node module syntax parser; not a GJS runtime test |
| Debian changelog version and executable source permissions | Passed |
| Full automated gate | Blocked at Meson setup: executable absent |
| Debian dependency preflight | Failed: required build packages absent |

The native tests used ICU 76.1 and include the unchanged full catalog and legacy
mapping checks. Available source-level measurements on this container, using
`/usr/bin/time` with the optimized native character-search contract suite, were
0.94 s and 9,632 KiB peak RSS on Stage 9 and 0.95 s and 9,648 KiB on Stage 10.
These single-run measurements include several search operations and test
assertions; they are not a latency benchmark, cannot establish a meaningful
performance change, and say nothing about GTK startup or rendering. No
unmeasured native algorithm rewrite was introduced. The native suite continued
to enumerate 292,988 assigned scalars and 296,197 distinct browseable identities.

## Not run / outstanding

The container has GCC and ICU but does not have Meson, valac, the GTK4,
libadwaita, GLib development, Fontconfig/Pango development, GNU msgfmt, GJS or
IBus runtime toolchain needed for full qualification. Debian mirror name
resolution failed. Full Vala and native font compilation, Meson tests (21
registered tests), new stdin CLI and picker preview tests, GTK runtime,
installed-package activation, real GJS and IBus, and Debian binary package
build are not run. The new translation merge has not been built. The manual
checklist in [release integration](release-integration.md) is outstanding.

Remaining functional/data limitations: ICU properties are Unicode 16 while
catalog names and emoji are Unicode 17; the alias/sequence supplement is
Unicode 15. The full official data sets and matching ICU upgrade were not
available. Localized emoji annotations do not cover all Unicode 17 entries;
full standardized text/emoji presentation data and Unihan readings are not
bundled. Arbitrary variable-font axes and unencoded glyph browsing remain
outside the implemented browser; exact color rendering depends on installed
fonts and Pango. Legacy encodings are bounded to the documented stateless
single-/double-byte mappings. Full UI translation and runtime accessibility
qualification remain incomplete. The application is not certified stable.

No source-level assertion of full parity or successful compilation is made.
