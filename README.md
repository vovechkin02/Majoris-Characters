# Majoris Characters

Characters is a native GTK4/libadwaita character utility for the Majoris desktop.
This development release adds exact emoji variation selection and multilingual
annotation search to the existing Unicode, font, and legacy character browsers.

| Project metadata | Value |
| --- | --- |
| Version | 0.6.0 |
| Platform target | Majoris OS / Debian 13 (Trixie) |
| Development status | Stage 6 — emoji functionality |
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
- Gettext catalog infrastructure; English is currently the only supplied UI language.
- Native Unicode names, aliases, scalar properties, scripts, blocks, named
  sequences, emoji catalogs, normalization, and related-character queries.
- Independently versioned data and local database generation tools.
- Virtualized character grid with bounded pages and total result counts.
- All Characters, Emoji, Symbols, Scripts, Unicode Blocks, and Named Sequences
  browsing scopes, with collection and general-category filters.
- Cancellable asynchronous search by English names, aliases, metadata keywords,
  literal catalog sequences, and hexadecimal or explicitly decimal code points.
- Character selection with complete code point notation, including composite emoji.
- Loading, empty, invalid-query and failure states; reset search and filters.
- Collapsible category sidebar, native keyboard navigation, and Ctrl+F search focus.
- Adaptive character inspector with enlarged glyph previews, explicit missing-glyph
  or nonprinting-preview notes, and a selector for each constituent scalar.
- Canonical names, aliases, assignment, blocks, planes, scripts, general category,
  age, combining and bidirectional properties, decomposition type, width, and
  emoji properties, subject to the installed data versions.
- Exact hexadecimal and decimal code points, UTF-8 bytes, UTF-16/32 code units,
  HTML numeric references, Unicode escape notation and CSS escape notation.
- NFC, NFD, NFKC, and NFKD code-point results, with optional normalized-text copies.
- Related-character navigation with bounded Back history; literal multiline text
  and explicit numeric identification of up to 32 scalars, including unassigned
  code points and NUL through numeric notation.
- Copy original text, individual metadata/encoding values, normalized text, or
  a report containing all sequence components. NUL-containing text cannot be
  copied through the GTK string clipboard; its numeric representations remain available.

- Installed font family and face browsing, mapped Unicode coverage including
  private-use scalars, font comparisons, and variable/color capability discovery.
- Explicit preview fallback control, adjustable point size, manual and periodic
  font inventory refresh, and nominal face-resolution checks.
- Thirty-five ICU legacy character sets with exact byte lookup, alternate
  byte mappings, repertoire search, and clipboard actions.
- Searchable emoji variations with exact sequence copying and inspection.
- Locale-aware annotation keywords with English fallback and Unicode case folding.

The bundled catalog is Unicode 17.0.0; ICU 76 provides Unicode 16.0 properties,
and the alias/named-sequence supplement is Unicode 15.0.0. Newer metadata not
available in those sources is explicitly unavailable rather than fabricated.
Script and general-category filters use ICU's assigned-property data.
Emoji annotations from Debian GTK 3.24.49-3 (Emojibase-derived data) are
bundled for 24 languages; they do not cover all Unicode 17 entries. Direct
CLDR XML imports are supported by the offline generator. The picker exposes
catalog-backed skin tone, gender, hair, direction, relationship, flag and
keycap choices, and presentation pairs derived from the catalog. Color glyphs
are rendered by Pango with installed emoji fonts. See [emoji functionality](docs/emoji-functionality.md)
for coverage and rendering boundaries. Multi-character composition, drag-and-drop,
favorites, persistent usage history, and the compact picker remain later stages.
The Vala/GTK and Fontconfig/Pango source is not yet compiler- or runtime-qualified
in the delivery environment.

## Requirements and dependencies

Debian 13 is the intended build and runtime environment. The build requires Meson
1.3 or newer, a Vala compiler, GCC or a compatible C compiler, Ninja, pkg-config,
GTK 4.14+, libadwaita 1.7+, GIO/GLib 2.80+, ICU 76+, Fontconfig 2.13+,
PangoFT2 1.50+, gettext, and Python 3. The integration
tests use Xvfb, xauth, a private D-Bus session and GLib command-line tools.
The full Debian build dependency list is in [debian/control](debian/control).
The application does not require unstable GNOME packages or GJS.

## Building and installing

Install the build tools on Debian 13:

```sh
sudo apt-get install build-essential valac meson ninja-build pkgconf \
  libgtk-4-dev libadwaita-1-dev libglib2.0-dev libglib2.0-bin libicu-dev gettext \
  libfontconfig-dev libpango1.0-dev fonts-dejavu-core fonts-noto-color-emoji \
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
sudo apt install ../majoris-characters_0.6.0-1_$(dpkg --print-architecture).deb
```

The source package uses `3.0 (quilt)` and requires the corresponding upstream
orig tarball for a full source-package build. The binary-only command above does
not require it. Debian packaging uses debhelper's Meson integration and
standard installation triggers; `debian/rules` is executable.

## Quick start and usage

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
scalar across installed faces. For legacy character sets, enter `byte:80` or
`byte:82 A0` to find an exact mapping in the selected encoding. See
[font inspection](docs/font-inspection.md) for mapping and preview limitations.
Select **Emoji**, search by a name or annotation keyword in the current locale,
then select an entry and choose **Emoji Variations**. The same action is available
in the inspector for recognized sequences. Choose an alternative to see its
actual code points; Copy Emoji preserves modifiers, joiners, and selectors.
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
uses the standard GApplication activation and action interfaces; no custom public
D-Bus API, library ABI, or plugin API is provided.

## Architecture and source layout

- `native/unicode/`: private C engine, search and inspector implementations, vendored catalog
  and generated supplement.
- `native/fonts/`: Fontconfig face inventory, coverage, and Pango resolution checks.
- `src/fonts/`: asynchronous font and encoding source loading.
- `vapi/`: internal Vala bindings.
- `tools/unicode/`: offline catalog generation.
- `tests/unit/`: native engine, search, inspector, Vala binding, and supersession contracts.
- `src/app/`: thin entry point and application lifecycle/actions.
- `src/search/`: bounded asynchronous request lifecycle and native result ownership.
- `src/views/`: window presentation, virtualized tiles, filtering, selection, and the inspector dialog.
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
worker. Query and identification text are not persisted or transmitted. The
inspector only accesses the system clipboard after an explicit copy action.
Its temporary navigation history is discarded on close. Its only persistent application
state is the three GSettings window keys. Native D-Bus activation is scoped to the user's session.
The interface inherits system theme styling; no custom CSS or Majoris-specific runtime package is required. Font previews
use explicit Pango attributes without changing the interface font. The app's original icon follows the
Majoris color palette. GNOME-derived data and algorithms are attributed in [third-party notices](THIRD_PARTY_NOTICES.md).

## Testing and release qualification

```sh
python3 tools/check_documentation.py .
meson test -C build --print-errorlogs
meson test -C build --suite integration --print-errorlogs
```

The default suite checks native Unicode, search, inspector, legacy encoding and font contracts, Vala bindings and
request supersession, declaration documentation, CLI version/help/error handling,
XML window startup, repeat activation, actions, clean close, settings persistence,
and actual GTK browser paging, search, filtering, selection, inspector navigation,
and exact emoji variation clipboard copying in a private D-Bus
session and Xvfb display. The test runner isolates the home and configuration
directories and treats GTK warnings as fatal. Native search and inspector contracts are tested without a
graphical display. Native timings are not a substitute for GTK responsiveness or platform performance qualification.

After installing the Debian package, the same runtime tests can target the real
installed executable:

```sh
python3 tests/integration/test_application.py /usr/bin/majoris-characters 0.6.0 cli
xvfb-run -a dbus-run-session -- python3 \
  tests/integration/test_application.py /usr/bin/majoris-characters 0.6.0 gui
```

See [Stage 6 qualification](docs/stage-6-qualification.md) for actual
verification results and remaining build/package checks. Historical Stage 1–5
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
