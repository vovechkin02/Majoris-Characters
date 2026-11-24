# Release and ISO integration

Stage 10 is a qualification candidate, not an assertion of a verified stable
release. The package remains `majoris-characters` with the stable application ID
`com.majoris.characters` and a Debian 13 / GNOME 48 baseline.

## Automated gate

From the source repository on Debian 13, install the build dependencies from
`debian/control` (and the standard Debian build tools), then run:

```sh
python3 tools/qualification/qualify.py build-release --package
```

Use a fresh build directory. The gate builds the real application, runs every
registered Meson test, requires an OK result for each, and optionally produces
Debian binaries. It does not treat skips, omitted results or old test logs as
passing. Also validate installed desktop and AppStream files with
`desktop-file-validate` and `appstreamcli validate --no-net` when those tools are
installed. Install the generated package in a disposable target environment;
run the installed CLI and GUI tests in README. Verify that the installed schema,
French message catalog, desktop action, provider registration, D-Bus activation
service, icon, IBus component and Shell extension are present.

## Majoris ISO builder

The ISO builder repository is not included in this delivery. Add a dedicated
Characters build/install hook following that repository's existing package
workflow. Use the source from this release, install its actual build dependencies,
compile and build the Debian binary, and install it inside the image using APT
so that declared runtime dependencies are resolved. Do not copy the unbuilt
source tree into the live filesystem, and do not pull unstable GNOME or ICU
libraries into Debian 13 just for this utility. Do not replace other native
applications or remove desktop packages.

If direct input is required out of the box, explicitly install `ibus`,
`gir1.2-ibus-1.0`, and `python3-gi` in addition to the package. Preserve the
user's choice of input source. The standalone picker does not require IBus.
If the Home menu does not enumerate GNOME Shell providers, integrate the
SearchProvider2 interface documented in [desktop integration](desktop-integration.md).
Register the picker shortcut in either the Desktop Shell or the optional extension,
not both, and preserve user-assigned bindings. Do not enable the optional
extension on the lock screen. Do not enable unrelated extensions or install
additional desktop utilities as part of this hook.

After installation in the chroot, verify `dpkg-query -W majoris-characters`,
`majoris-characters --version`, `majoris-characters --unicode-version` and the
compiled GSettings schema. Boot both live and installed images and run the
following checks from the graphical user session rather than the build chroot.

## Target desktop checklist

| Area | Required verification |
| --- | --- |
| Launch | Cold D-Bus activation, desktop action, repeat activation, quit, restore geometry |
| Browser | Rapid search, paging, aliases, script/block filters, nonprinting scalars, missing glyphs |
| Inspector | Composite sequence and component metadata, related navigation, exact copy and normalization |
| Fonts | Actual face matching and substitution, variable/color flags, private-use glyphs, legacy bytes |
| Emoji | Color glyphs, supported variations and fallback, localized annotation search |
| Composer | Combining and RTL text, undo/redo, normalization, clipboard and Wayland/X11 drag-and-drop |
| Collections | Exact restart persistence, ordering, retention, history opt-out and locked settings |
| Accessibility | Orca/AT-SPI announcements, focus order, keyboard-only navigation, labels, high contrast, 200% text/scale |
| Localization | French UI and installed desktop labels, English fallback, long labels and RTL locale layout |
| Picker | Global shortcut conflict, lock-screen exclusion, live size changes, retained clipboard ownership |
| Search provider | Cold start, result metadata, superseded requests, Home menu discovery and inspector activation |
| IBus | Actual focused client insertion, sensitive fields, modifier passthrough, focus loss, input-source switch, privacy |
| Performance | Startup, typing latency, full-repertoire paging, font discovery, cancellation and repeated close/open memory |

Test real GNOME Wayland and X11 sessions where supported. Check the shared Majoris
theme in light and dark mode. Record tested hardware, software versions,
commands, outcomes and blockers. The Xvfb test suite is not evidence for
compositor-specific drag-and-drop, IBus insertion, Shell keybindings, screen
reader behavior or display scaling. Do not promote the ISO package to stable
until required checks pass and remaining data limitations are accepted.
