# Translation completion — 1.0.26

## Scope

Revised Priority 6 completes the active French, German, and Spanish gettext
message inventories. English is the source language. The translation boundary
remains presentation-only: canonical Unicode names, identifiers, query grammar,
code-point values, clipboard content, and serialized settings are unchanged.
The block registry retains standardized English block names. This release does
not add interface languages or translate the full Unicode character database.

The 1.0.25 inventory contained 695 marked messages, with 617 French, 599 German,
and 545 Spanish translations. The new inventory contains 950 unique messages,
including named language collection labels and presentation selector labels.
Existing translated messages are retained; missing messages receive translations
or an established proper name where no distinct localized name is available.
The language registry generator emits identity `N_` markers; the C adapter still
returns the same English vocabulary and locale tags. Vala translates labels at
the existing display boundary. Fallback character names and emoji presentation
labels are translated without altering canonical identity or search metadata.
The AppStream description is independently translated into the three languages.

| Locale | Previous messages | New messages | Total | Empty/fuzzy |
| --- | ---: | ---: | ---: | ---: |
| French | 617 | 333 | 950 | 0 |
| German | 599 | 351 | 950 | 0 |
| Spanish | 545 | 405 | 950 | 0 |

CLDR language and script names use the supplied CLDR 48 archive and Babel
2.18.0 locale data, with established spellings for proper names lacking a
localized equivalent. These names retain Unicode-3.0 attribution; see
`THIRD_PARTY_NOTICES.md`. The new prose translations require native-speaker
review. A nonempty translation that legitimately equals the source is not
considered missing. Catalog coverage is not a guarantee of linguistic quality
or that every user-visible upstream proper name is localized.

## Maintenance

The standard PO files remain the translator-maintained source. No additional
runtime or build dependency is required. The deterministic inventory helper
reads the repository's current literal gettext conventions from
`po/POTFILES.in`: C/Vala `_` and `N_` calls (including concatenated literals),
Python literal `_` calls, translatable GTK UI elements, and desktop Name,
Comment, and Keywords fields. Dynamic lookups use the corresponding marked
keys; the helper does not attempt to be a general-purpose gettext parser.
If new plural/context syntax is introduced, extend the inventory and tests
before relying on this helper for those messages.

Regenerate the template after source changes, then merge and translate:

```sh
python3 tools/localization/catalogs.py
for lang in de es fr; do
    msgmerge --update --backup=none po/$lang.po po/majoris-characters.pot
done
```

Review every new or fuzzy entry before publication. The normal Meson gettext
update target remains available; after using it, regenerate the deterministic
template with the helper. Verify the catalog against the source inventory:

```sh
python3 tools/localization/catalogs.py --check
python3 tests/unit/test_translation_catalogs.py
```

The Meson source suite runs the same regression. GNU `msgfmt --check
--check-format` validates each PO and produces temporary MO files. Python's
standard gettext implementation checks every compiled message and every
printf operand, and rejects stale or missing messages. Existing standalone
GNU format tests remain available. No compiled MO or temporary fixture is
included in the source archive.

## Qualification

| Check | Result |
| --- | --- |
| Active messages per catalog | 950/950 |
| Empty or fuzzy translations | 0 |
| Obsolete messages | 0 |
| Compiled MO lookup with Babel and Python gettext | 2,850/2,850 |
| Printf signatures | 23 per catalog, all matched |
| Python source, importer, and extraction tests | 38 passed |
| Declaration documentation | 1,913/1,913 detected declarations |
| Language registry regeneration | Byte-identical to marked header |
| Original Unicode data | 40/41 files byte-identical; one extraction-only header changed |
| Official dataset manifest verification | Passed |
| XML, UI, JSON, Python syntax and Debian version | Passed |
| GNU gettext extraction and compiled-catalog test | Not run; GNU tools unavailable |
| Full Han dataset regeneration | Not run |
| Vala/GTK compilation and live localized UI | Not run |
| Native-speaker and assistive-technology review | Outstanding |

Babel 2.18.0 was available offline for independent PO parsing and MO generation;
GNU xgettext, msgmerge, msgfmt, Meson and valac were unavailable. The compiled
lookup and format checks in this environment used Babel-generated MOs, not
GNU gettext binaries. The target GNU test is supplied for the complete build.
Manual target qualification should cover French, German and Spanish browser,
picker, font dialog, composer, inspector, desktop launcher and application
metadata, including layout at narrow widths and large text sizes. Verify that
raw copied values and search results are independent of display language.

The next functional priority is external Desktop Shell and ISO integration.
Full application compilation remains a separate outstanding prerequisite to
release qualification.
