# Priority 3 qualification — 1.0.3

## Implemented scope

- All 371 Unicode 17 presentation bases, representing 742 standardized text and
  emoji presentation pairs. The compact base list was transcribed from the
  official Unicode publication; both selectors are supported for every base.
- 1,819 exact supplementary identities, including 1,272 selector omissions of
  the 3,944 fully qualified catalog entries. The published `emoji-test.txt`
  counts are 1,029 minimally qualified and 243 unqualified entries. The counts
  match; complete official-file set comparison is not claimed.
- Exact literal/numeric search, inspector naming and related presentation
  alternatives, preserved original variation selection, and structural family
  grouping. Ordinary browse counts and persistent text identities are unchanged.
- CLDR 48 English annotations for seven new single-scalar emoji and the ballet
  dancer base sequence; tone-base fallback applies to its five modified forms.
  Locale matching now uses case-insensitive locale tags, common Chinese regional
  script fallbacks, and per-term parent-locale fallback.
- Reproducible generators, offline official-file verification, annotation
  coverage reporting, optional completeness gate, and catalog regeneration
  that stages dependent alias indexes and coverage before replacing the catalog.

## Executed checks

- GCC native builds using `-std=gnu11 -Wall -Wextra -Werror -O1`, ICU 76.1.
- All six ICU-based suites: Unicode engine (6 groups), search (10), inspector
  (5), legacy encodings (4), emoji (7), composition (4): 36 groups passed.
- Emoji checks exercise all 371 presentation bases, every supplementary identity,
  exact alias search and filter behavior, invalid selectors, existing family
  relationships, exact original selection and the new CLDR search terms.
- The complete emoji test suite passed AddressSanitizer, UndefinedBehaviorSanitizer
  and leak detection; other sanitizer suites were not rerun.
- Python input protocol (3), release source contracts (4), Unihan importer (3),
  and emoji data contracts (5): 15 tests passed. Official-file importer checks
  use isolated fixtures; both checked-in generated emoji tables reproduce
  byte-for-byte from their pinned inputs.
- Documentation checker: 1,238/1,238 detected declarations documented. This is a
  lexical check, not compiler or Valadoc qualification.
- Python, XML/UI and JSON syntax, strict GLib GSettings schema dry-run, and
  Debian changelog version validation passed.
- Global repertoire remains 297,334 assigned scalars and 300,543 ordinary
  browseable identities. The 1,819 supplementary identities are only exposed
  through exact queries or relevant variation selection, not a duplicate grid.

## Outstanding data and runtime qualification

The complete CLDR 48 archive and official Unicode files could not be downloaded
into the build environment. The presentation bases were available through the
published Unicode source, as were the eight new English annotation records.
The rest of the 24-language dataset retains its original GTK/Emojibase provenance.
The coverage report records 1,914 direct and 1,790 tone-base English matches
(240 uncovered entries); each of the other 23 locales has 253 uncovered entries.
English catalog names remain available for all 3,944 entries. The report is a
key-coverage measure, not a translation quality or completeness certificate.
Full official-file set comparison and full multilingual CLDR import remain
outstanding. Resolved CLDR input is preferred; the importer is not a complete
LDML inheritance/alias processor. Structural variation families are application
policy rather than an official semantic-family taxonomy.

No new runtime dependencies are required. Meson, valac and the GTK4/libadwaita
and font development toolchain are unavailable in this environment. The Vala
binding test was updated but not compiled or run. Full GUI, Pango color glyph
rendering, desktop integration, Debian binary packaging, and external ISO
integration remain unqualified. The preceding Unicode and Unihan official-data
limitations also remain. Run the complete release gate on the Debian 13 target
before declaring this a stable release.
