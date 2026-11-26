# Character presentation and collection migration — 1.0.16

## Scope

This pass implements priority 2 of the revised native migration plan. It moves
collection and character presentation policy out of the C search module without
rewriting repertoire traversal or changing installed application interfaces.
The 27 semantic collection names, 17 plane names, eight general-category group
names, group masks, and scalar fallback vocabulary now belong to the Vala
collection catalog. Registry-backed names remain in their respective immutable
native data modules. Existing translation message IDs and filter indexes are
preserved; the new catalog is a gettext extraction input.

The Vala character presentation module resolves exact scalar arrays, canonical
names and aliases, fallback names, safe tile display, and U+ notation. It uses
`Inspector.format_points()` rather than maintaining a second code formatter.
The inspector no longer formats and reparses the numeric identity. NUL remains
inspectable as U+0000 while exact NUL-terminated clipboard text remains absent.
Unassigned values, noncharacters, controls, separators, combining marks and the
existing exceptional invisible glyphs retain their presentation rules.

## Native boundary and ownership

The raw native text and sequence label APIs retain ownership of opaque catalog
identity decoding for sequences. Scalar metadata retrieval and prepared search
matching remain native to avoid per-character Vala callbacks. The Vala parser
supplies category masks and untranslated label arrays once per search. The
scanner validates mask disjointness, array bounds and label lengths, then copies
the vocabulary into the request and compiles the masks into a constant-time
category lookup. It rejects a request without a complete metadata configuration.
The category slot count and symbol/plane index bounds remain native validation
constants; the catalog names and actual group masks are not duplicated there.

Native code uses the GLib contracts established in 1.0.15. The search vector,
bitset, C11 atomic cancellation, result ordering, allocation failure handling,
and raw Unicode data are unchanged. Presentation functions return owned Vala
strings; mixed-language test prototypes and label cleanup have been updated.
The new code does not introduce a new pattern hierarchy or runtime dependency.

## Verification

| Check | Result |
| --- | --- |
| Native Unicode engine | 6 groups passed |
| ICU adapter | 4 groups passed; 17,086 direct mappings |
| Prepared search boundary | 7 groups passed |
| Legacy adapter | 3 groups passed |
| Python protocol, import and source checks | 24 passed |
| Documentation lexical checker | 1,618 of 1,618 declarations |
| Native compatibility probe | All 300,543 ordinary ordered identities and exact texts; sequence labels; all eight category result sets; ten prepared metadata queries; 1,819 supplementary emoji identities matched 1.0.15 |
| Collection source comparison | All 52 semantic, plane and general-category labels match the prior spelling and order |
| Unicode source, generated and vendored files | 37 byte-identical files |
| XML, UI, JSON and Python syntax; Debian changelog | Passed |
| Full Vala compilation and Vala/mixed-language tests | Not run |
| Standard GLib development-header build and font backend | Not run |
| GTK runtime, package build and performance qualification | Not run |

The native Unicode modules were rebuilt with GCC, ICU 76.1 development headers,
`-Wall -Wextra -Werror`, and temporary minimal GLib declarations, linking to the
real GLib 2.84 runtime. Official GLib headers and the Vala/GTK toolchain remain
unavailable in this environment. The declarations and comparative native probes
are not shipped. The compatibility probe used C fixture operands identical to
the Vala category policy; it did not execute the Vala parser or presentation
functions. The added Vala regression cases and existing mixed-language contracts
must be compiled and run on Debian 13. No runtime performance claim is made.

The source distribution, measured as handwritten production `.vala` and `.c`
physical lines only, changed from 7,120 Vala / 4,145 C to 7,426 Vala / 3,962 C.
Generated headers, bindings and tests are excluded. The 1.0.15 GLib pass added
multiline declarations; the baseline here is measured directly from its archive.

## Follow-up

The next revised migration priority is Unicode relationship construction: case,
accent and decomposition orchestration can move into the existing Vala
relationship service while retaining raw database and ICU operations. Establish
full compiler and behavioral qualification before performance-sensitive scanner
migration.
