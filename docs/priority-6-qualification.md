# Priority 6 qualification — 1.0.6

## Functional changes

- Unicode Planes and Unassigned & Noncharacters are independent, scalar-only
  browser scopes with optional plane filters, appended to the native enum to
  preserve existing slot values. All Characters remains unchanged.
- All 17 planes partition 1,112,064 valid scalar values into 297,334 assigned
  and 814,730 unassigned values (including the 66 permanent noncharacters).
  Surrogate code points are never returned. Unassigned previews use a middle
  dot and noncharacters have distinct status labels. Explicit copying still
  preserves valid non-NUL scalar values, including legacy repertoire mappings.
- Ten new semantic collections (27 total) use indexed curated range data and
  Unicode properties. Existing collection indexes are retained. The technical
  collection is expanded. Semantic collections are intentionally overlapping
  and are not a normative or exhaustive ontology.
- Direct decomposition displays the immediate mapping and type independently
  of recursive normalization, with copy and inspector navigation. Uses ICU 76
  raw decomposition plus the Unicode 17 U+A7F1 delta; covers algorithmic Hangul.
- Linux Unicode keyboard-entry guidance is per assigned non-NUL scalar, with
  explicit compatibility notes. It does not inject input or guarantee that
  every application supports the GTK input method.
- Both new scope visibility preferences persist through GSettings. New strings
  are marked for translation; additional translated catalogs are not supplied.

## Executed evidence

The native Unicode modules were compiled with GCC using GNU11, `-O2`,
`-Wall -Wextra -Werror`, and ICU 76.1. Each native test executable linked the
same rebuilt Unicode object files and ran successfully.

| Check | Result |
| --- | --- |
| Unicode engine | 6 groups passed |
| Character search | 12 groups passed |
| Character inspector | 6 groups passed |
| Legacy encoding | 7 groups passed |
| Emoji catalog | 7 groups passed |
| Composition | 4 groups passed |
| Input protocol | 3 Python tests passed |
| Unihan import | 3 Python tests passed |
| Emoji data | 5 Python tests passed |
| Release source contracts | 4 Python tests passed |
| Strict GSettings schema dry run | Passed |
| Declaration documentation checker | 1,396/1,396 detected |
| Debian changelog | 1.0.6-1 parsed successfully |

Search contracts exhaustively verify all 17 plane partitions, exact unassigned
and noncharacter lookup, invalid filter handling, and representative membership
for all ten new semantic groups plus technical symbols. The complete assigned
catalog remains 300,543 browseable identities including sequences. Inspector
contracts validate all 17,086 non-None decomposition properties against
available direct mappings and parse each mapping back into valid scalars;
representative canonical, compatibility, Hangul and Unicode 17 results have
exact expected values. This is not a full official UCD conformance comparison.

## Not run

The container lacks the Vala, Meson, GTK4/libadwaita and font development
toolchain. Native font compilation, Vala binding tests, GTK interaction tests,
full application build, and Debian binary packaging remain unqualified.
Sanitizer regression runs were not performed in this pass. Run the existing
Meson release gate on Debian 13 and manually qualify accessibility, font
rendering, keyboard entry and desktop integration. No external project has
been modified. Official Unihan and complete multilingual annotation coverage
remain open from prior priorities.
