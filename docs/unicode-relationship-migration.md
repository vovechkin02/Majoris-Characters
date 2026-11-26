# Unicode relationship migration — 1.0.17

## Scope

Priority 3 of the revised migration plan moves scalar relationship assembly
from the native Unicode catalog to the existing Vala `Relationships` service.
It preserves the Unicode 17 case additions, ICU simple upper/lower/titlecase
and mirror mappings, recursive canonical components, reverse accent variants,
and advisory single-scalar confusable relationships. The prior Latin,
Hiragana and Katakana ranges, primary-script selection, assigned-character
filtering, NUL rejection, exact-text deduplication and binary UTF-8 ordering
remain unchanged. Sequence and emoji presentation relationships are unchanged.
No new dependencies, settings, user-facing strings or UI controls are needed.

The native ICU adapter exposes individual mapping primitives and recursive
canonical decomposition into caller-owned storage. This is intentionally
separate from immediate decomposition (which includes compatibility mappings)
used by the inspector. The Vala collector reuses a 32-scalar scratch array
for the reverse accent scan. The native catalog returns a recoverably allocated
copy of the immutable confusable class with explicit length, or null on error.
The old native candidate vector and policy functions have been removed. Native
interfaces retain GLib types and allocation contracts. No new design-pattern
hierarchy is needed for this stateless domain service.

## Verification

| Check | Result |
| --- | --- |
| Unicode engine | 6 native groups passed |
| ICU adapter | 5 native groups passed, including 17,086 direct decomposition mappings |
| Prepared search | 7 native groups passed |
| Legacy adapter | 3 native groups passed |
| Python protocol, importer and source contracts | 25 tests passed |
| Relationship candidate compatibility | 1,112,064 valid scalar values matched |
| Declaration documentation | 1,635 of 1,635 detected declarations documented |
| Generated, vendored and original Unicode data | 37 files byte-identical |
| XML, JSON and Python syntax; Debian changelog | Passed |
| Official GLib development-header compilation | Not run |
| Vala and mixed-language tests | Not run |
| Full Meson/GTK build, font backend, package, performance qualification | Not run |

All native Unicode modules were rebuilt with GCC, `-Wall -Wextra -Werror`,
ICU 76.1 development headers and temporary minimal GLib API declarations linked
against the installed GLib runtime. The real GLib development headers, Vala
compiler, Meson and complete GTK development toolchain are unavailable here.
Temporary verification declarations and executables are not distributed.

A temporary native compatibility harness implemented the same scalar candidate
rules as the new Vala collector using the new primitives. It compared the
ordered candidate vectors (before exact-text deduplication and sorting) against
1.0.16 for every valid Unicode scalar, including unassigned values and NUL.
The two complete binary streams had matching SHA-256 hashes:
`852f5c509cb81ab8bc901a5324ffe9f59762043eddf7a8cfe9162df5263ad81d`.
This is evidence for the native primitives and equivalent rules, not execution
of the Vala implementation. The Vala binding test now includes exact ordered
sets from the baseline for recursive Latin decomposition, Japanese voiced
variants, Hangul, titlecase, new case pairs and empty sets. These tests must
still be compiled and executed on the target. No performance parity claim is
made.

Handwritten production physical lines changed from 7,426 Vala / 3,962 C to
7,531 Vala / 3,862 C, excluding headers, generated data, bindings and tests.
The native catalog no longer owns relationship construction.

## Follow-up

Priority 4 is emoji annotation policy: prepare locale fallback chains and folded
search terms in Vala, retaining immutable native annotation lookup and the
compact scanner. Complete a target compiler baseline before migrating further
performance-sensitive code.
