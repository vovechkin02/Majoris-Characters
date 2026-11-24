# Stage 3 qualification — 0.3.0

## Scope and delivered source

Stage 3 extends the Stage 2 source archive with an assigned-character and
sequence browser, native search, a GTK GridView with recyclable XML tiles,
collection and general-category filters, selection, pagination, loading/empty/
error states, reset, and cancellable latest-request-only asynchronous scheduling.
Existing application identity, geometry schema, application actions, licenses,
Unicode data versions, and native Unicode functionality are retained.

The native database reports Unicode 17.0.0 catalog data, ICU 76.1 / Unicode 16.0
properties, and the Unicode 15.0.0 alias/named-sequence supplement. It recognizes
292,988 assigned scalars (including private-use scalars) and 296,197 distinct
browseable identities, including composite emoji and named sequences. These
counts describe this bundled catalog and runtime, not a complete Unicode 17
property database. All 346 blocks and nine emoji groups remain available.

## Verification performed

| Check | Result |
| --- | --- |
| Native engine regression, GCC GNU11 `-Wall -Wextra -Werror` | Passed, five existing test groups |
| Native search, GCC GNU11 `-Wall -Wextra -Werror` | Passed, seven test groups |
| Assigned scalar enumeration and ordering | 292,988 verified against engine |
| Composite identity merging and exact code-point queries | Passed, including duplicate keycap identities |
| All nine emoji group sequences | Passed, exact text and group counts |
| Named sequence catalog enumeration and lookup | Passed |
| Block/script/category filtering and name/alias keyword search | Passed |
| Invalid input, bounds, invisible controls, combining marks, cancellation | Passed |
| AddressSanitizer and UndefinedBehaviorSanitizer, both native test suites | Passed |
| Leak detection, both native test suites | Passed |
| Source declaration checker | 333/333 detected declarations documented |
| Python syntax and XML/SVG syntax | Passed |
| GResource file references and GtkChild IDs | Passed, 18/18 fields found in templates |
| Meson/AppStream/Debian version agreement and Debian rules executable bit | Passed |
| Debian changelog parser | Passed, 0.3.0-1 |
| Vala binding and search-controller test execution | Not run |
| Meson setup/full application compilation | Not run |
| GtkBuilder runtime instantiation, GUI browser and lifecycle tests | Not run |
| Strict GSettings schema compilation and GResource compilation | Not run; unchanged schema, XML parsed and resource paths checked |
| Debian package build, Lintian, installed-consumer tests | Not run |
| Physical rendering, accessibility and GTK runtime performance | Not run |

The delivery environment is Debian 13 with GCC and ICU 76.1. `valac`, Meson,
GTK4/libadwaita/GIO development packages, and GLib resource/schema tools are
unavailable. An attempted package metadata refresh failed with DNS resolution
errors for `deb.debian.org`. `dpkg-checkbuilddeps` identifies unmet build
dependencies. The new Vala/UI tests are registered and must be executed on the
target toolchain before release; this archive is a source implementation, not
a qualified binary package. The documentation checker is a conservative
lexical/AST coverage check, not Valadoc compiler verification.

## Reproduce native qualification without GTK development packages

From the project root with GCC, pkg-config, and ICU 76 development files:

```sh
for test in test-unicode-engine test-character-search; do
  gcc -std=gnu11 -O2 -Wall -Wextra -Werror -UNDEBUG \
    -I native/unicode "tests/unit/$test.c" \
    native/unicode/character-search.c native/unicode/unicode-engine.c \
    $(pkg-config --cflags --libs icu-uc) -pthread -o "/tmp/$test"
  "/tmp/$test" || exit 1
done
python3 tools/check_documentation.py .
```

For sanitizer runs, replace `-O2` with `-g -O1`, add
`-fsanitize=address,undefined -fno-omit-frame-pointer`, and run with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`.

On the complete Debian 13 toolchain, follow the README's Meson and Debian
package build instructions. Run the full test suite, including
`Search bindings and request supersession` and `Character browser interaction`,
then inspect the window at the normal 800×600 test geometry and at narrow
widths. Confirm keyboard navigation, rendered glyphs, selection and window
closure while a search is outstanding. Stage 4 is the full character inspector;
localized CLDR keywords, font coverage, composer, clipboard actions, favorites,
and desktop picker integration remain assigned to later stages.
