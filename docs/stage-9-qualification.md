# Stage 9 qualification

Version: 0.9.0. Target: Debian 13 / Majoris OS.

## Executed checks

- All six unchanged ICU-based native suites: 29 test groups passed. Compiled
  using GCC GNU11, `-O2 -Wall -Wextra -Werror`, pthreads, ICU 76.1.
- Python input-source protocol: three tests passed (exact Unicode preservation,
  malformed records and bounds). These tests do not exercise IBus itself.
- Strict GLib GSettings schema dry-run passed.
- Declaration documentation checker: 1,056/1,056 detected declarations documented.
  Lexical detection is not compiler or Valadoc qualification.
- XML/JSON/Python syntax, resource references, POTFILES, all 128 GtkTemplate
  field IDs/types, executable bits, and Debian changelog version passed.
  Installed D-Bus signatures, desktop launcher behavior, and AppStream validation
  remain unverified.

## Added tests awaiting target qualification

- `Application command-line options`: headless input search, exact composite
  output, NUL exclusion, and invalid empty query.
- `Application window lifecycle`: compact picker CLI activation, D-Bus export,
  initial and refined searches, metadata, result activation, insertion history,
  and normal process shutdown.
- `Character browser interaction`: actual compact picker model, superseded query,
  exact clipboard copy, and closing the picker without closing the full window.

The delivery environment has ICU and GCC, but lacks valac, Meson, GTK4,
libadwaita, GLib development headers, Fontconfig/Pango development headers,
GJS, IBus and Python GObject introspection. Package mirror DNS resolution did
not succeed. The new Vala application, Shell extension, and IBus engine have
not been compiled or runtime-qualified. Native Fontconfig tests, full Meson
suite, sanitizers in this stage, Debian binary package build, Wayland/X11,
actual Home menu discovery, and real input insertion remain not run. Earlier
Unicode property and annotation coverage gaps are unchanged.

## Required target tests

```sh
meson setup build --prefix=/usr
meson compile -C build
meson test -C build --print-errorlogs
dpkg-buildpackage -us -uc -b
```

Install the binary package in a Debian 13 GNOME 48 session and follow
[desktop integration](desktop-integration.md). Verify activation through the
installed D-Bus service from a cold process, exact introspection signatures,
rapid supersession, search provider discovery in the Home menu, picker focus
and keyboard access, clipboard retention after picker close, and single-instance
behavior. Enable/disable the extension, test an alternate shortcut and a
conflict, and confirm the binding is not active at the lock screen. Add the IBus
source and test ordinary key passthrough, exact combining/emoji insertion in
GTK and other IBus-compatible clients, candidate paging/clicks, focus changes,
cancel/reset/disable, input-source switching, declared sensitive fields,
malformed output, and history opt-out. Verify with both Wayland and X11 where
available. No separate Desktop Shell or ISO builder changes were made.
