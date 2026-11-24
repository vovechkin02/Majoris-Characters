# Stage 1 qualification — 0.1.0

## Implementation status

The application foundation, native window and actions, resource manifest,
settings schema, theme integration, localization inputs, desktop and D-Bus
metadata, Debian packaging, documentation and automated test sources are
implemented. Unicode engine and subsequent stages have not been started.

## Verification performed

Environment: Debian 13.3 (Trixie). Validation below was performed on the source
archive contents before packaging. The development compiler, GTK development
libraries, Meson, debhelper and other required build tools were unavailable.
The attempted dependency installation failed because `deb.debian.org` could not
be resolved. No alternate toolchain or network dependency was added to the
project to bypass this restriction.

| Check | Result |
| --- | --- |
| Python syntax compilation of test/checker sources | Passed |
| Declaration documentation scanner | Passed — 32/32 detected declarations documented |
| XML syntax (window, schema, resource manifest, icon, AppStream) | Passed |
| Referenced resource files present | Passed |
| Strict GLib schema compilation dry-run | Passed |
| Desktop identity and executable name inspection | Passed |
| Debian changelog parse (0.1.0-1) | Passed |
| Executable mode on `debian/rules` | Passed |
| Debian build dependency preflight | Blocked — dependencies missing |
| Meson configure and compile | Not run — toolchain unavailable |
| GTKBuilder template instantiation and GUI lifecycle | Not run |
| CLI integration test | Not run |
| Installed application/D-Bus activation and settings restoration | Not run |
| Actual Majoris theme and dark/light appearance | Not run |
| Valadoc toolchain validation | Not run |
| Desktop-file/AppStream official validators | Not run |
| Debian binary build, Lintian, install/remove/reinstall | Not run |
| Native Wayland/compositor behavior | Not run |

The documentation checker performs a scoped lexical declaration scan for the
Vala syntax used in Stage 1 and AST-based checking for Python. It is not a full
Vala parser or substitute for Valadoc and compiler validation. Its reported
coverage is not a semantic proof that documentation is complete or correct.

## Remaining qualification on Debian 13

Follow the build commands in [README](../README.md). After installing the
required dependencies, run the full Meson suite and confirm the main window
opens without GTK warnings. Manually inspect the About dialog, `Ctrl+Q`, theme
appearance, window resizing and maximization/restoration on a real compositor.
Run the installed executable tests after building and installing the Debian
package. Validate desktop metadata with `desktop-file-validate` and
`appstreamcli validate`, run Lintian on the generated binary package, and
inspect installed resources. Do not mark Stage 1 release-qualified until these
checks succeed. No test count or successful compilation is inferred from source.
