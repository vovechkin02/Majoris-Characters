# Priority 8 qualification — 1.0.8

## Source changes

- Native libadwaita Keyboard Shortcuts dialog with main menu and compact picker
  entry points, including private picker mode. Registered application bindings
  are rendered via GTK's accelerator parser; configured global keybindings are
  read without changing or enabling them.
- Accessible labels on GTK browser/font/saved list items with unbind cleanup,
  inspector preview and related-character names, explicit page navigation and
  compact clipboard announcements. The picker search shortcut now checks the
  GTK default modifier mask instead of accepting additional modifiers.
- Presentation-only localized Unicode property descriptions, including all
  existing enumerated general categories, bidi classes, decomposition types and
  East Asian width values, flags, plane names, and script names/extensions.
  Raw property values, names, identifiers and clipboard/report semantics remain
  unchanged. The generated script names and emoji group labels are marked for
  gettext extraction. The native script name query shares the same snapshot.
- French, German and Spanish PO catalogs and matching desktop name/summary
  metadata. Counts of nonempty translated messages (excluding the header) are
  613 French, 595 German and 541 Spanish, against 690 statically extracted
  messages in the updated source inventory. These counts are not claims of
  complete UI or data translation. CLDR script names are attributed; no new
  runtime or build dependency was added. Additional locale names, some block
  labels, error messages and other untranslated text retain English fallback.

## Executed verification

All native Unicode modules were compiled with GCC GNU11, `-O2 -Wall -Wextra
-Werror`, ICU 76.1. The Unicode engine suite was rerun after adding script-name
lookup assertions (Latin, Hiragana, Beria Erfe, invalid and null inputs).

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
| Release source contracts | 5 Python tests passed |
| Declaration documentation | 1,481/1,481 detected |
| Unicode property regeneration | Byte-identical output |
| Strict GSettings schema dry run | Passed |
| UI/XML and Python AST validation | Passed |
| Three PO catalogs | Babel format checks and MO generation; Python gettext lookup and fallback passed |
| Debian changelog | 1.0.8-1 parsed successfully |

The updated integration driver exercises the native dialog, registered
accelerator label, property descriptions and fallback. Source contracts check
entry points, template mappings, accessible list items and translation keys.
These checks are not substitutes for actual GTK interaction or Orca testing.

## Remaining qualification

Meson, valac, GNU msgfmt, GTK4/libadwaita and font development packages are not
available in this environment. Full Vala and native font compilation, GTK
interaction, all three GNU msgfmt tests, desktop shortcut/insertion behavior,
real screen-reader testing, native-speaker review and Debian packaging have
not been run. Sanitizers were not rerun. Run the existing full release gate
on Debian 13 and follow `accessibility-and-localization.md`. Official Unihan,
complete emoji CLDR annotations and prior data conformance remain outstanding.
No external Desktop Shell, application or ISO Builder source has been changed.
