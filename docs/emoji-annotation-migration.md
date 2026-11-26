# Emoji annotation policy migration — 1.0.18

## Scope

Priority 4 of the revised migration plan moves locale normalization, parent
locale selection, Chinese regional script preference, English fallback and
annotation operand preparation into the Vala emoji domain. Locale resolution
uses the bundled table registry, so unavailable script-specific locales fall
back through the same parents as before. The supplied database currently has
24 locale tables; no new annotation translations are claimed.

The search parser resolves the locale chain once before execution. Each AND
term, or the single literal-letter query used in emoji browsing, is folded
once by the existing ICU NFKC case-folding algorithm. A null or empty fold is
represented by an empty string that cannot match an annotation. Exact numeric
queries, literal sequences, catalog order and the English name/category
matching rules are preserved. The native job validates and owns copies of
all prepared annotation operands, freeing them with the query on every exit.
A catalog metadata search with missing annotation preparation fails closed.

The native matcher now receives decoded scalars from the scanner, resolved
locale indexes and a folded term. It builds bounded UTF-8 lookup keys on the
stack and searches immutable annotation tables. Exact selector-free lookup
precedes skin-tone-base lookup within each locale. No locale interpretation,
term folding or heap allocation is performed by this matcher. The same keys
and substring semantics are used as before. The scan itself remains in C.

Presentation labels and alias-name construction now belong to the Vala emoji
service. The shared labels are used by the variant picker; canonical scalar,
named-sequence and fully qualified emoji naming keeps precedence in the
inspector. The native catalog only supplies raw catalog names and exact alias
identities. No new runtime dependency, UI control or generated data change is
introduced. No new design-pattern hierarchy is necessary for these stateless
policies.

## Verification

| Check | Result |
| --- | --- |
| Native Unicode engine | 6 groups passed |
| Native ICU adapter | 5 groups passed; 17,086 direct mappings checked |
| Native prepared search | 8 groups passed, including copied annotation operands |
| Native legacy adapter | 3 groups passed |
| Python protocol, source and importer tests | 26 tests passed |
| Native annotation compatibility | 2,555,712 matching decisions |
| Declaration documentation | 1,659 of 1,659 detected declarations documented |
| Generated, vendored and original Unicode data | 37 files byte-identical |
| XML, JSON and Python syntax; Debian changelog; GSettings | Passed |
| Official GLib header compilation | Not run |
| Vala and mixed-language contracts | Not run |
| Full Meson/GTK build, native fonts, package and performance qualification | Not run |

Native Unicode sources were rebuilt with GCC, `-Wall -Wextra -Werror`, ICU
76.1 headers and temporary minimal GLib API declarations linked against the
installed GLib runtime. Official GLib development headers, Vala, Meson and
complete desktop development headers are unavailable here. Temporary test
headers, executables and comparison harnesses are not distributed.

The baseline comparison used the original 1.0.17 native matcher and the new
matcher with a temporary C implementation of the same prepared locale rules.
For all 3,944 catalog entries, 36 locale inputs (including all 24 bundled
languages, regional, mixed-case, unknown, empty and oversized tags), and 18
query terms (including invalid, empty, compatibility and non-ASCII inputs),
the two Boolean decision streams matched exactly. Both SHA-256 digests were
`91ae26943eb5d4e5a1bd8eddf7dca9372aa2e14d14cc3ac7b63480a09fe6463e`.
This is evidence for the prepared native lookup and equivalent locale rules;
it is not execution of the Vala policy or a performance benchmark. The Vala
contract includes locale order, folding, exact alias naming and invalid
identities. Existing mixed-language tests now use the localized Vala factory.

Handwritten production lines changed from 7,531 Vala / 3,862 C to 7,664 Vala /
3,869 C, excluding headers, generated data, bindings and tests. The new native
validation and operand ownership add a few physical lines while removing
locale and presentation policy from C. The Vala share is approximately 66.5%.

## Follow-up

Priority 5 is property and metadata migration: move non-hot-path formatting
and property routing into Vala while retaining typed native property lookup.
Complete a target compiler baseline before further performance-sensitive work.
