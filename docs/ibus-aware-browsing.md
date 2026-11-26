# IBus-aware international browsing — 1.0.24

## Scope

Revised Priority 4 extends the existing Languages → Configured Keyboard
Languages collection. It reads the optional GNOME `sources` setting of type
`a(ss)` and supports both `xkb` and `ibus` source kinds. XKB continues to use the
exact bundled layout/variant registry. IBus uses the exact configured engine
name and the running daemon's registered `EngineDesc.language` property.
The active global engine, physical layout of an IME, and guessed substrings of
engine names are not used to infer a writing system. No settings are changed,
no engines are started or selected, and the existing optional Characters IBus
insertion engine is unchanged.

`InputSourcePolicy` validates and normalizes engine locale tags (ISO language,
script, region, and optional UTF-8 encoding suffix), rejects language-neutral
or malformed descriptors, and deduplicates resolved operands. Source capture
is bounded to 256 tuples, IDs to 255 bytes, locale tags to 63 bytes, and the
worker snapshot to 1,023 bytes. No truncated operand is published. Legacy raw
XKB IDs remain accepted; `ibus:<normalized locale>` identifies a resolved
language operand. Neither registry objects nor desktop IPC enter the worker.
The `LanguageSelection` service combines the existing ICU-derived snapshot
script masks, preserving primary Script and Script_Extensions matching.
The underlying native Unicode data and scanner are unchanged.

`KeyboardSourceMonitor` is a GSettings/IBus Adapter, using native GObject
Observer signals. Discovery is lazy, asynchronous and bounded to 1,500 ms per
registry call. Configuration changes, disconnection, filter exit and shutdown
cancel obsolete calls. Callback identity checking prevents late responses
from replacing the current configuration. Re-entering the collection and
reconnection trigger fresh discovery. XKB-only results are available while
IBus is absent or discovery is pending. Only actual snapshot changes notify
an active collection. Missing schemas, unsupported sources, unavailable
engines and neutral descriptions contribute no scripts; they do not silently
fall back to English. The query remains a writing-system union, not an exact
IME vocabulary or a list of characters the keyboard can type.

## Dependency and API boundary

The native IBus client library is now a build and runtime library dependency
(`ibus-1.0` >= 1.5.3, Debian `libibus-1.0-dev`, automatic shared-library runtime
dependency). The IBus daemon, installed input engines and the existing Python
insertion integration remain optional runtime services. The native headless
search implementation still has no IBus dependency. `IBus.Bus.async()` and
`list_engines_async()` are the public client API. The narrow local VAPI
corrects the historical list finish ownership annotation: IBus creates
referenced engine descriptors, so the adapter owns and releases both the list
and its elements, including stale callbacks. References:

- https://ibus.github.io/docs/ibus-1.5/IBusBus.html
- https://valadoc.org/ibus-1.0/IBus.EngineDesc.html
- https://sources.debian.org/src/gnome-initial-setup/51.1-2/gnome-initial-setup/pages/keyboard/cc-input-chooser.c

## Expected script masks

| Configured engine declares | Associated script codes |
| --- | --- |
| `ja` or `jpn` | Hira, Kana, Hani |
| `ko` or `ko_KR` | Hang, Hani |
| `zh_Hans` | Hani |
| `zh_Hant` or `zh_TW` | Hani, Bopo |
| `hi` | Deva |
| `sr_Latn` | Latn |
| `ru` | Cyrl |
| `other` or `und` | None (policy rejection) |

These are ICU locale-to-script mappings, not hard-coded input engine IDs.
This scope uses GNOME's configured sources, not standalone IBus preload lists
or Fcitx configuration. Registry-only changes are refreshed on re-entry,
configuration change or daemon reconnect; the application does not poll.

## Qualification

A new Meson target, `test-input-source-policy`, exercises exact source lookup,
malformed and neutral data, bounded snapshots, mixed XKB/IBus masks, actual
worker search membership, and settings-monitor activation and shutdown. It
uses a private test schema and the GSettings memory backend. It does not
require a live IBus daemon or alter real user settings. The source-contract
suite verifies registry wiring, cancellation and the absence of settings writes.

Local checks on Debian 13:

- 18 release source-contract tests, three Unicode algorithm snapshot tests,
  five emoji data tests, three input protocol tests, three Unihan importer
  tests, and three isolated official archive import tests passed (35 Python
  tests). The full bundled official-data regeneration test exceeded the local
  execution timeout; the installed official-data manifest verification passed.
- A targeted temporary native harness compiled the unchanged language and
  scalar-property modules using GCC `-Wall -Wextra -Werror`, ICU 76.1 headers
  and temporary minimal GLib declarations linked to the installed GLib runtime.
  It passed 14 locale mask, three XKB registry and 12 scalar membership checks.
  It did not execute the new Vala source or the live IBus adapter.
- All 41 Unicode source, generated and upstream files and the native C
  implementation are byte-identical to 1.0.23. The original official data
  manifest verifies successfully. The new tooltip strings have French,
  German and Spanish translations; all three catalogs parse with Babel.
- Official GLib and IBus development headers, Vala, Meson, msgfmt and the
  full desktop toolchain were unavailable. The new Vala target, real daemon
  discovery, GUI, Debian binary packaging and performance qualification have
  not been run. Temporary verification declarations and executables are not
  distributed.

Run the full existing gate on Debian 13 with the new build dependency:

```sh
sudo apt-get install libibus-1.0-dev
python3 tools/qualification/qualify.py build-release --package
```

For live qualification, use a test GNOME session with configured XKB and IBus
engines for Japanese, Korean and Chinese. Select Languages → Configured
Keyboard Languages, confirm the corresponding scripts appear, then remove or
add an engine using GNOME Settings and check that results update. Stop and
restart the test IBus daemon while the filter is active and confirm XKB
remains usable and IBus scripts return after reconnection. Leave the filter
or close the window during discovery; verify no stale result publication.
Do not change the user's real configuration during automated qualification.

## Follow-up

Revised Priority 5 covers cross-process favorites/history coordination and
exact font comparison coverage. Full compiler and desktop qualification
remains a release blocker.
