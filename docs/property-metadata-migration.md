# Property and metadata migration — 1.0.19

## Scope

Priority 5 of the revised migration plan moves application-facing property
selection and formatting to `src/unicode/property-metadata.vala`. The existing
`Unicode.get_property_value` Vala API retains its callers and exact output.
Plane is available for every valid scalar, including unassigned values and
noncharacters. Other scalar fields require assignment in the Unicode 17
snapshot. Unknown keys, invalid scalar values, and absent Han metadata return
null rather than a fabricated value.

The scalar adapter exposes a typed selector for six immutable text properties,
a raw age encoded in tenths, the existing numeric combining class, and four
Boolean property bits. ICU alias resolution remains native, restricted to the
same properties as before. Vala formats decimal values, age and Y/N values.
Canonical and algorithmic character names remain native because the search
scanner uses them in its hot path. The inspector's translated descriptions
remain separate from the standardized values used for copying and searching.

The Han adapter exposes twelve typed immutable fields and numeric collation
fallback groups. Vala chooses exact official or fallback keys, never silently
substituting one source for another. Zero collation groups are absent, not
reported as stroke counts. The generated group labels and formal-versus-
fallback scan predicates remain native. No database is regenerated, no new
runtime dependency is introduced, and the existing GLib ownership conventions
are maintained. The old string-returning property dispatchers are removed.

## Verification

| Check | Result |
| --- | --- |
| Native Unicode engine | 6 groups passed |
| Native ICU adapter | 5 groups passed; 17,086 direct mappings checked |
| Native prepared search | 8 groups passed |
| Native legacy adapter | 3 groups passed |
| Python protocol, source and importer tests | 27 passed |
| Native metadata compatibility | 37 property/alias comparison hashes matched |
| Declaration documentation | 1,696 of 1,696 detected declarations documented |
| Unicode source, generated and vendored data | Unchanged |
| XML, UI, JSON, Python syntax and Debian changelog | Passed |
| Official GLib development-header compilation | Not run |
| Vala and mixed-language contracts | Not run |
| Full Meson/GTK build, native fonts, package and performance qualification | Not run |

The native Unicode modules were rebuilt with GCC, `-Wall -Wextra -Werror`,
ICU 76.1 headers, and temporary minimal GLib API declarations linked against
the installed GLib runtime. Official GLib development headers, Meson, Vala,
and the full desktop development toolchain are unavailable. Temporary test
headers, executables and comparison harnesses are not distributed.

The native compatibility probe compared 1.0.18 with the new raw accessors
and a temporary C implementation of the equivalent Vala formatting policy.
For each of 37 keys (27 metadata fields plus ten supported, unsupported or
invalid aliases), the probe traversed U+0000 through the invalid boundary
U+110000 inclusive. Counts of present values and ordered FNV-1a comparison
hashes matched for every key. This represents 41,222,181 property decisions
per build, including surrogate and invalid boundary inputs. The probe is not
execution of the Vala implementation or a benchmark. The expanded Vala
binding regression covers age, numeric and Boolean formatting, ICU aliases,
invalid points, and the independent Han sources.

Handwritten production source changed from 7,664 Vala / 3,869 C lines to
7,756 Vala / 3,840 C lines, excluding headers, generated data, bindings and
tests. The Vala share is approximately 66.9%.

## Follow-up

Establish a full target compilation and benchmark baseline before evaluating
further migration of the native scanner. Official Unihan and CLDR dataset
coverage, external integration and release qualification remain outstanding.
