# Stage 8 qualification

Version: 0.8.0. Target: Debian 13 / Majoris OS.

## Executed checks

| Check | Result |
| --- | --- |
| Native Unicode engine | 5 groups passed |
| Native character search | 7 groups passed |
| Native character inspector | 5 groups passed |
| Native legacy encoding | 4 groups passed |
| Native emoji catalog | 4 groups passed |
| Native composition | 4 groups passed |
| GNU11 compilation with `-Wall -Wextra -Werror` | Passed for all six ICU-based native suites |
| Strict GLib schema compilation | Passed |
| GLib keyfile backend schema smoke test | Passed across two separate processes |
| Declaration documentation checker | 957/957 detected declarations documented |
| GtkTemplate field/ID/type static matching | 123/123 matched |
| XML, Python syntax, GResource paths, schema key references and POTFILES | Passed |
| Debian changelog version and rules executable bit | Passed |

The native regressions were compiled with GCC and ICU 76.1 (uc and i18n),
using pthreads and optimization. No native implementation changes were made in
Stage 8. The schema smoke test called the installed GLib/GIO runtime directly
with a temporary keyfile configuration directory, seeded exact Unicode text
(including canonically equivalent but distinct forms, a composite emoji, and
colon-containing records), set preferences, and verified all values in a
second process. This validates the schema and storage backend only; it is not
execution of the Vala LibraryStore implementation. The documentation checker
and template ID/type check are static checks, not compiler verification.

## Checks added but not run

The environment lacks valac, Meson, GTK4/libadwaita and GLib development
headers, and Fontconfig/Pango development headers. The full application and
new Vala tests were not compiled or executed. The native Fontconfig tests,
GTK UI tests, actual application persistence, Debian binary build, and
Wayland/X11 integration also remain unverified. Sanitizers were not rerun in
this stage because native sources are unchanged; historical qualification
reports remain available. Earlier Unicode metadata and emoji annotation data
coverage limitations remain unchanged.

New tests are registered with Meson:

- `Saved collection ordering and privacy`: exact identities, independent
  favorite ordering, deduplication, recency/frequency ordering, history
  removal, limit changes, opt-out, malformed data and count saturation.
- `Saved collections across process restarts`: the real Vala repository is
  seeded and reopened in separate processes using an isolated keyfile backend.
- `Character browser interaction`: extends the existing GTK test with favorite
  saving, history recording and removal, collection filters, and live privacy,
  size, and category preference changes.

## Remaining qualification on Debian 13

Install the build dependencies in `debian/control` and run:

```sh
meson setup build --prefix=/usr
meson compile -C build
meson test -C build --print-errorlogs
meson test -C build --suite integration --print-errorlogs
dpkg-buildpackage -us -uc -b
```

Manually check all copy/add/favorite entry points, favorites reorder and
retention after restart, equal-count frequency tie breaking, history disable
and clear with writable settings, and behavior with administrative lockdown.
Verify live category visibility and font/grid size changes, persistence after
closing and reopening, adaptive layout, light/dark theme, keyboard access and
screen-reader names. Ensure new schema keys retain old geometry keys on
package upgrade. Stage 9 desktop integration and Stage 10 final qualification
are not implemented here. No public library API/ABI or installed application
identity was changed; no new build or runtime dependency was introduced.
