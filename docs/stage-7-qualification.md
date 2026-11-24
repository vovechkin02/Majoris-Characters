# Stage 7 qualification

Version: 0.7.0. Target: Debian 13 / Majoris OS.

## Executed checks

GCC compiled all six ICU-based native suites using GNU11, `-Wall -Wextra
-Werror`, ICU 76.1 (uc and i18n), and pthreads where required. Results:

| Test executable | Result |
| --- | --- |
| test-unicode-engine | 5 groups passed |
| test-character-search | 7 groups passed |
| test-character-inspector | 5 groups passed |
| test-legacy-encoding | 4 groups passed |
| test-emoji-catalog | 4 groups passed |
| test-composition | 4 groups passed |

All six also passed AddressSanitizer, UndefinedBehaviorSanitizer, and leak
detection. The new native suite verifies empty and composite graphemes,
combining marks, flags, keycaps, mixed-script text, all nine exact output
representations, all four normalization forms, full-range processing beyond
32 scalars, malformed UTF-8, unassigned properties, invalid formats, invalid
scalars, maximum input size, and output expansion rejection. Existing native
regressions retain full emoji and named-sequence round trips.

The declaration checker detected 838/838 documented declarations. XML parsing,
GResource source paths (7), all 93 GTK template field references, Python syntax,
translation source list, README links, Debian changelog version, and executable
packaging rules passed source checks. The documentation checker is lexical
rather than a Valadoc/compiler verification tool.

## Checks not run

The environment has GCC and ICU headers, but does not have valac, Meson,
GTK4/libadwaita, Fontconfig/Pango development headers, or the GLib schema
compiler. Full application compilation, Vala bindings, Fontconfig tests,
strict schema compilation, GTK runtime tests, desktop clipboard interoperation,
drag-and-drop, visual and accessibility qualification, and Debian binary
package building were not run. Stage 2, 5 and 6 limitations remain unchanged.

## Remaining qualification

On the Debian 13 development host, install the dependencies in `debian/control`
and run:

```sh
meson setup build --prefix=/usr
meson compile -C build
meson test -C build --print-errorlogs
meson test -C build --suite integration --print-errorlogs
dpkg-buildpackage -us -uc -b
```

The browser integration test includes the composer, native undo/redo and grouped
normalization, cut/paste, exact representation copying, raw copying, inspector
navigation, and append from the inspector, legacy/font browser and emoji picker.
Manually verify native keyboard editing shortcuts, bidirectional editing, text
selection and clipboard interoperability under Wayland and X11; drag ordinary,
combining, private-use, and composite emoji tiles into the editor and another
application. Confirm stale tile recycling cannot change an in-flight drag,
NUL cannot be copied or appended, and long text remains editable when technical
analysis is unavailable. Check compact window layout in light and dark mode.
