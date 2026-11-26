# Majoris Characters

Characters is a native GTK4/libadwaita character utility for the Majoris desktop.
This qualification candidate includes the ten development stages and subsequent
Unicode, international browsing, emoji data, and exact font inspection improvements.

| Project metadata | Value |
| --- | --- |
| Version | 1.0.11 |
| Platform target | Majoris OS / Debian 13 (Trixie) |
| Development status | Vala migration pass 3 — qualification candidate (not a verified stable release) |
| Application ID | `com.majoris.characters` |
| Debian package | `majoris-characters` |
| License | GPL-3.0-or-later |

## Contents

- [Features](#features)
- [Requirements and dependencies](#requirements-and-dependencies)
- [Building and installing](#building-and-installing)
- [Quick start and usage](#quick-start-and-usage)
- [Architecture and source layout](#architecture-and-source-layout)
- [Security, resources and compatibility](#security-resources-and-compatibility)
- [Testing and release qualification](#testing-and-release-qualification)
- [Documentation and contribution](#documentation-and-contribution)
- [License](#license)

## Features

- Native Vala application with a declarative GTKBuilder XML window.
- Single-instance application activation through GApplication and D-Bus.
- Native header bar, menu, About dialog, and Quit action (`Ctrl+Q`).
- Persistent normal window size and maximized state using GSettings.
- Desktop launcher, application icon, AppStream metadata and D-Bus service.
- Shared system theme and light/dark appearance through libadwaita.
- Gettext catalogs: English source strings, expanded French, and German and Spanish
  interface translations, with English fallback for untranslated messages.
- Native Unicode names, aliases, scalar properties, scripts, blocks, named
  sequences, emoji catalogs, normalization, and related-character queries.
- Independently versioned data and local database generation tools.
- Virtualized character grid with bounded pages and total result counts.
- All Characters, Emoji, Symbols, Scripts, Unicode Blocks, and Named Sequences
  browsing scopes, with collection and general-category filters.
- All 17 Unicode planes; a separate reserved/noncharacter scope with plane filters.
- Twenty-seven overlapping semantic collections, including superscripts, subscripts,
  fractions, whitespace, quotations, accessibility and variation selectors.
- Language collections, current-locale and configured XKB keyboard collections,
  script-extension matching, and CJK radical and stroke browsing.
- Han reading search and inspection with explicit fallback provenance; an offline
  importer supports formal Unihan readings, definitions and classification data.
- Cancellable asynchronous search by English names, aliases, metadata keywords,
  literal catalog sequences, and hexadecimal or explicitly decimal code points.
- Character selection with complete code point notation, including composite emoji.
- Loading, empty, invalid-query and failure states; reset search and filters.
- Collapsible category sidebar, native keyboard navigation, and Ctrl+F search focus.
- Keyboard Shortcuts reference in the main menu and compact picker (Ctrl+?).
- Accessible names on recycled grid/list items, explicit paging announcements, and
  human-readable Unicode property and script labels that retain normative codes.
- Adaptive character inspector with enlarged glyph previews, explicit missing-glyph
  or nonprinting-preview notes, and a selector for each constituent scalar.
- Canonical names, aliases, assignment, blocks, planes, scripts, general category,
  age, combining and bidirectional properties, decomposition type, width, and
  emoji properties, subject to the installed data versions.
- Exact hexadecimal and decimal code points, UTF-8 bytes, UTF-16/32 code units,
  HTML numeric references, Unicode escape notation and CSS escape notation.
- Direct per-scalar decomposition mappings, navigable and copyable separately from
  recursive normalization, plus Linux Unicode keyboard-entry guidance.
- NFC, NFD, NFKC, and NFKD code-point results, with optional normalized-text copies.
- Related-character navigation with bounded Back history; literal multiline text
  and explicit numeric identification of up to 32 scalars, including unassigned
  code points and NUL through numeric notation.
- Copy original text, individual metadata/encoding values, normalized text, or
  a report containing all sequence components. NUL-containing text cannot be
  copied through the GTK string clipboard; its numeric representations remain available.

- Installed font family and face browsing, mapped Unicode coverage including
  private-use scalars, font comparisons, and variable/color capability discovery.
- Exact FreeType file/face previews with fallback disabled, variable-axis adjustment,
  selected-instance reset, and separate Microsoft Symbol cmap inspection.
- Explicit preview fallback control, adjustable point size, manual and periodic
  font inventory refresh, and nominal face-resolution checks.
- Seventy-seven ICU legacy character sets, including GB18030-2022, EUC and
  contextual ISO-2022, HZ and ISCII. Browse scalar repertoires, inspect exact
  byte alternatives or canonical bytes, and decode or encode multi-scalar sequences.
- Searchable emoji variations with exact sequence copying and inspection.
- Locale-aware annotation keywords with English fallback and Unicode case folding.
- Collapsible character composer shared across browser, inspector, font and emoji dialogs.
- Native text editing, selection, cut/copy/paste, undo/redo, and copy-only character tile drags.
- Direct raw text copying, nine encoding representations, explicit four-form normalization,
  scalar and UTF-8 byte counts, and ICU extended grapheme counts.
- Inspect selected composed text (up to 32 scalars) without modifying the editor.
- Persistent, manually ordered favorites for exact character and sequence identities.
- Recently and frequently used collections, per-entry removal, and history clearing.
- Configurable history retention and recording opt-out; favorites remain independent.
- Searchable saved collections with copy, inspect, and add-to-composer actions.
- Persistent sidebar category visibility, grid and font preview sizes, and font fallback.
- Compact picker with emoji/symbol category filters, full pagination, favorites,
  recent and frequent collections, variation selection and inspection.
- GNOME Shell SearchProvider2, desktop action, and single-instance CLI activation.
- Configurable opt-in GNOME Shell 48 shortcut and native IBus candidate insertion.

The bundled catalog and independent scalar property snapshot target Unicode
17.0.0. The snapshot covers 297,334 assigned scalars, including private use.
Names combine the GNOME catalog, stable ICU names, and Unicode 17 additions.
The alias supplement includes the eight documented additions since Unicode 15;
the 461 named-sequence records are retained pending full official-file comparison.
Normalization uses ICU 76 with the Unicode 17 combining-class and compatibility
delta. See [Unicode engine](docs/unicode-engine.md) for provenance and limits.
See [international browsing](docs/international-browsing.md) for CJK query syntax,
source coverage and official Unihan import instructions. The bundled fallback is
not the full official Unihan database.
Emoji annotations from Debian GTK 3.24.49-3 (Emojibase-derived data) are
bundled for 24 languages, plus CLDR 48 English annotations for the eight new
Emoji 17 bases. Full multilingual coverage remains outstanding. Direct CLDR
XML imports and coverage reporting are supported by the offline generator. The
picker exposes catalog-backed skin tone, gender, hair, direction, relationship,
flag and keycap choices, all 742 standardized presentation sequences, and
selector-omission forms. Exact search retains supplementary identities without
adding duplicates to ordinary browsing. Color glyphs
are rendered by Pango with installed emoji fonts. See [emoji functionality](docs/emoji-functionality.md)
for coverage and rendering boundaries. Desktop integration is provided through the
compact picker, SearchProvider2, an opt-in Shell shortcut, and an optional IBus input
source. See [desktop integration](docs/desktop-integration.md) for setup and limits.
The Vala/GTK and Fontconfig/Pango source is not yet compiler- or runtime-qualified
in the delivery environment.

## Requirements and dependencies

Debian 13 is the intended build and runtime environment. The build requires Meson
1.3 or newer, a Vala compiler, GCC or a compatible C compiler, Ninja, pkg-config,
GTK 4.14+, libadwaita 1.7+, GIO/GLib 2.80+, ICU 76+, Fontconfig 2.13+,
FreeType 2.10+, PangoFT2 1.50+, gettext, and Python 3. The integration
tests use Xvfb, xauth, a private D-Bus session and GLib command-line tools.
The full Debian build dependency list is in [debian/control](debian/control).
The application does not require unstable GNOME packages or GJS. The optional
GNOME Shell 48 extension runs inside the Shell; the optional IBus source needs
`ibus`, `gir1.2-ibus-1.0`, and `python3-gi` (Debian recommended packages).

## Building and installing

Install the build tools on Debian 13:

```sh
sudo apt-get install build-essential valac meson ninja-build pkgconf \
  libgtk-4-dev libadwaita-1-dev libglib2.0-dev libglib2.0-bin libicu-dev gettext \
  libfontconfig-dev libfreetype-dev libpango1.0-dev fonts-dejavu-core fonts-noto-color-emoji \
  python3 xvfb xauth dbus-daemon debhelper
```

Configure, compile and test (no network is used by the build):

```sh
meson setup build --prefix=/usr
meson compile -C build
meson test -C build --print-errorlogs
```

Run without installing from a graphical session:

```sh
GSETTINGS_SCHEMA_DIR="$PWD/build/data" ./build/src/majoris-characters
```

Install system-wide:

```sh
sudo meson install -C build
majoris-characters
```

For Debian binary packages (run from the repository root):

```sh
dpkg-buildpackage -us -uc -b
sudo apt install ../majoris-characters_1.0.11-1_$(dpkg --print-architecture).deb
```

The source package uses `3.0 (quilt)` and requires the corresponding upstream
orig tarball for a full source-package build. The binary-only command above does
not require it. Debian packaging uses debhelper's Meson integration and
standard installation triggers; `debian/rules` is executable.

## Quick start and usage

Open the independent compact picker with `majoris-characters --picker`, the
Character Picker desktop action, or the application menu. Ctrl+Shift+Period
opens it while Characters has focus. The optional Shell extension enables
Super+Period globally after explicit activation:

```sh
gnome-extensions enable characters-picker@majoris.com
```

The compact picker copies exact text, and cooperating applications can use the private `--pick-text` subprocess contract. For direct insertion without clipboard
replacement, select the Characters IBus input source and use Ctrl+Shift+Period
in a compatible application; type a query and choose a candidate. See the
[desktop integration guide](docs/desktop-integration.md) for setup, keybinding
configuration, and Wayland limitations.

Launch a full catalog query with `majoris-characters --search=U+20AC`.

Open **Characters** from the desktop application launcher, or execute
`majoris-characters` in a graphical session. Launching it again presents the
existing window in the same D-Bus session. Select a collection in the sidebar,
then use the collection dropdown and general-category filter where available.
Search within the selected scope, or choose All Characters for a global search.
For example, enter `euro`, `U+20AC`, `20AC`, `0x20ac`, `dec:8364`, or `€`.
Composite sequences can be searched literally or with space-separated `U+`
values, such as `U+1F469 U+200D U+1F4BB`.

Use Previous/Next Page to browse larger result sets. Click a tile or use native
keyboard navigation to select it and see its name and complete code-point list.
Choose **Inspect Character**, or activate the tile with Enter or a double-click,
to open the inspector. Use the component selector for a multi-scalar sequence,
copy a representation, or navigate to a related character and back. Choose
**Identify Characters** from the main menu to inspect pasted text or explicit
code points without changing the browser search. See [character inspector](docs/character-inspector.md)
for precise input, representation, and clipboard contracts.
Choose **Fonts** or **Character Sets** from the sidebar or main menu. Select a
family and face, filter the mapped repertoire, change preview size, or toggle
font fallback. Use **Font Variations** in the inspector to compare the selected
scalar across installed faces. Choose **Inspect Font Face and Symbol Map**
for exact glyph previews, design-axis adjustments and raw symbol codes.
For legacy character sets, enter `byte:80` or
`byte:82 A0` or `byte:94 39 FC 36` to find an exact scalar mapping in the selected
encoding. Choose **Inspect Byte Sequence** to decode stateful or multi-scalar input,
encode literal text or explicit code points, copy the original or canonical bytes,
and open the shared Unicode inspector. See [legacy encodings](docs/legacy-encodings.md)
and [font inspection](docs/font-inspection.md) for mapping and preview limitations.
Select **Emoji**, search by a name or annotation keyword in the current locale,
then select an entry and choose **Emoji Variations**. The same action is available
in the inspector for recognized sequences. Choose an alternative to see its
actual code points; Copy Emoji preserves modifiers, joiners, and selectors.
Choose **Add to Composer** from the browser or a dialog to append exact text.
Open the composer from the menu or Ctrl+M, edit text or drag a character tile
into the editor, then copy the whole string or a selected representation.
Normalization is explicit and undoable. See [composition and clipboard](docs/composition.md)
for selection semantics, analysis limits, and exact-text guarantees.
Choose **Add to Favorites** in any character browser or inspector. Open Favorites,
Recently Used, or Frequently Used from the sidebar or menu; search, copy, inspect,
remove, or add a saved identity to the composer. Favorites can be moved up or down
when no collection filter is active. Preferences controls history recording and
retention, top-level sidebar category visibility, and glyph sizes. See
[collections and preferences](docs/library-and-preferences.md) for exact identity,
privacy, ordering, and retention semantics.
Ctrl+F focuses search; the main menu resets search and filters, shows version
information, or closes the window. The window size is restored on subsequent
launches. See [character browsing and search](docs/character-search.md) for query
semantics and known limitations.

```sh
majoris-characters --help
majoris-characters --version
majoris-characters --unicode-version
```

These options do not need a graphical display. The registered D-Bus service
uses the standard GApplication activation and action interfaces and the GNOME
SearchProvider2 interface. No public library ABI or plugin API is provided.

## Architecture and source layout

- `native/unicode/`: private C catalog and property lookups, search, ICU operations, and generated
  and vendored databases.
- `src/unicode/`: Vala scalar inspection, representation formatting, and composition policy.
- `src/emoji/`: Vala exact variation collections and structural family keys.
- `native/fonts/`: Fontconfig inventory, FreeType exact glyphs and axes, and Pango fallback.
- `src/fonts/`: asynchronous font and encoding source loading.
- `vapi/`: internal Vala bindings.
- `tools/unicode/`: offline catalog generation.
- `tests/unit/`: native engine, search, inspector, Vala binding, and supersession contracts.
- `src/app/`: thin entry point and application lifecycle/actions.
- `src/search/`: bounded query parsing, semantic and Han predicates, language selection,
  asynchronous request lifecycle and native result ownership.
- `src/library/`: GSettings-backed exact-text favorites and bounded usage history.
- `src/views/`: window presentation, virtualized tiles, filtering, selection, inspector dialogs, and composer.
- `data/`: UI templates, GResources, schema, application icon and desktop metadata.
- `po/`: translation catalog inputs.
- `tests/integration/`: real executable CLI and GUI lifecycle tests, plus browser and inspector interaction.
- `tools/`: declaration documentation coverage checker.
- `debian/`: package metadata and debhelper integration.

See [architecture](docs/architecture.md) for ownership and design decisions.

## Security, resources and compatibility

The application is unprivileged and performs no networking, font installation,
font removal, or external process launching. Font inspection reads installed
font metadata through Fontconfig, which may maintain standard font caches. Search runs in a bounded, cancellable native
worker. Query, identification, and composed text are not automatically persisted or transmitted. Clipboard
access and drag-and-drop are initiated by the user through native GTK controls.
Its temporary navigation history is discarded on close. Persistent state consists
of window geometry, explicit favorite identities, opt-out-controlled character usage,
and presentation preferences in the local GSettings backend. Arbitrary clipboard
contents, typed composition text and copied metadata are not recorded. Usage is
recorded only by explicit character copy, composer addition, or IBus insertion
actions (including explicitly identified sequences). Disabling history clears retained records when
settings are writable; deletion is not a secure erase of storage or backups.
Native D-Bus activation is scoped to the user's session. Desktop searches do not
change history or the clipboard; activating a result opens its inspector.
The optional IBus source blocks input in clients declaring password, PIN or private
input and cancels search on focus loss. The input query is passed to a local subprocess via standard input, not its
command line. The explicit insertion history command still receives the selected
character as an argument.
The interface inherits system theme styling; no custom CSS or Majoris-specific runtime package is required. Font previews
use exact FreeType rasterization without fallback or explicit Pango attributes
with fallback, without changing the interface font. The app's original icon follows the
Majoris color palette. GNOME-derived data and algorithms are attributed in [third-party notices](THIRD_PARTY_NOTICES.md).

## Testing and release qualification

```sh
python3 tools/check_documentation.py .
python3 tests/release/test_source_contracts.py
meson test -C build --print-errorlogs
meson test -C build --suite integration --print-errorlogs
```

The default suite checks native Unicode, search, ICU adapter, legacy encoding and font contracts, Vala inspector,
composition and emoji variation contracts, bindings and
request supersession, declaration documentation, CLI version/help/error handling,
XML window startup, repeat activation, actions, clean close, settings persistence,
and actual GTK browser paging, search, filtering, selection, inspector navigation,
exact emoji variation copying, composer editing, normalization and clipboard copying,
saved-collection interactions, compact picker copying and desktop search in a private D-Bus
session and Xvfb display. Headless library tests and a separate-process keyfile
persistence test cover the new settings repository. The test runner isolates the home and configuration
directories and treats GTK warnings as fatal. Native search and Vala inspector contracts and the Python input response protocol are
tested without a graphical display. Native timings are not a substitute for GTK responsiveness or platform performance qualification.

After installing the Debian package, the same runtime tests can target the real
installed executable:

```sh
python3 tests/integration/test_application.py /usr/bin/majoris-characters 1.0.11 cli
xvfb-run -a dbus-run-session -- python3 \
  tests/integration/test_application.py /usr/bin/majoris-characters 1.0.11 gui
```

For a fresh, fail-closed automated build/test and optional Debian package gate:

```sh
python3 tools/qualification/qualify.py build-release --package
```

See [ISO integration and desktop qualification](docs/release-integration.md) for
installation and target runtime checks. See [Accessibility and localization](docs/accessibility-and-localization.md) for
keyboard guidance, translation coverage, and manual accessibility qualification.
See [Vala migration qualification](docs/vala-migration-pass-3.md) for current evidence,
[Priority 8 qualification](docs/priority-8-qualification.md) for earlier evidence, and
[Stage 10 qualification](docs/stage-10-qualification.md) for historical
verification results and remaining build/package checks. Historical Stage 1–9
reports are preserved under `docs/`. Source inspection is
not equivalent to compilation or runtime validation.

## Documentation and contribution

Follow the Majoris Cross-Project Engineering Standards v1.8 supplied by the
project owner. Keep XML UI separate from behavior, use American English,
meaningful Valadoc and Python docstrings, clear ownership, and only introduce
GoF patterns where a real variation warrants them. See [CONTRIBUTING](CONTRIBUTING.md)
and the [development roadmap](docs/roadmap.md).

## License

Project-owned source is licensed under GPL-3.0-or-later (see [LICENSE](LICENSE)).
AppStream metadata is CC0-1.0. The application icon is original to this project.
See [Unicode engine](docs/unicode-engine.md) for data provenance and limitations.
GNOME-derived components and Unicode data retain their upstream licenses; see
[third-party notices](THIRD_PARTY_NOTICES.md).
See [debian/copyright](debian/copyright) for packaging license information.
