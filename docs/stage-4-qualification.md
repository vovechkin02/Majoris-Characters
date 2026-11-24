# Stage 4 qualification — 0.4.0

Stage 4 adds the immutable native inspection snapshot, exact sequence encoding
representations, length-aware normalization, character identification, a
sequence-aware adaptive inspector dialog, metadata copying and related navigation.
The implementation uses the existing search and Unicode data APIs, preserving
all earlier database versions and browsing behavior. See
[character inspector](character-inspector.md) for supported behavior and limits.

## Verification performed

| Check | Result |
| --- | --- |
| Native Unicode engine, GCC GNU11 `-Wall -Wextra -Werror` | Passed, 5 contract groups |
| Native character search regression | Passed, 7 contract groups |
| Native character inspector | Passed, 5 contract groups |
| Encoding formats including supplementary scalars, NUL, controls and combining marks | Passed |
| All 3,944 emoji and 461 named-sequence text/code round trips | Passed |
| NFC, NFD, NFKC, NFKD, expanded output, embedded NUL and unavailable data | Passed |
| Snapshot independence from search request, related emoji components, invalid and oversized input | Passed |
| AddressSanitizer, UndefinedBehaviorSanitizer and leak detection, all three native suites | Passed |
| Source declaration checker | Passed, 443/443 detected declarations |
| XML/SVG, Python syntax, resource paths and GtkChild IDs | Passed |
| Debian changelog/version metadata and executable rules permissions | Passed |
| Vala inspector and existing Vala test compilation/execution | Not run |
| GTK dialog instantiation, browser/inspector integration, clipboard and visual/accessibility validation | Not run |
| Full Meson build, strict schema/resource compilation, Debian package and installed-consumer tests | Not run |

The delivery environment has Debian 13, GCC, and ICU 76.1 (Unicode 16.0).
`dpkg-checkbuilddeps` confirms that Vala, Meson, GTK4/libadwaita, GLib development
and resource tools, gettext and Debian build dependencies are not installed.
The native tests have no dependency on those tools. GUI and binding tests are
registered in Meson but not executed here. Declaration coverage is the existing
conservative source scan, not a Valadoc compiler audit or proof of semantics.

## Reproduce native tests

From the source root with ICU development files, pkg-config, and GCC:

```sh
for test in test-unicode-engine test-character-search test-character-inspector; do
  gcc -std=gnu11 -O2 -Wall -Wextra -Werror -UNDEBUG -I native/unicode \
    "tests/unit/$test.c" native/unicode/character-inspector.c \
    native/unicode/character-search.c native/unicode/unicode-engine.c \
    $(pkg-config --cflags --libs icu-uc) -pthread -o "/tmp/$test"
  "/tmp/$test" || exit 1
done
python3 tools/check_documentation.py .
```

For sanitizer qualification replace `-O2` with `-g -O1` and add
`-fsanitize=address,undefined -fno-omit-frame-pointer`. Run with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1`.
On the target Debian toolchain, run the README's Meson setup/compile/test
commands and the Debian binary package build. Confirm the inspector opens in
800×600 and narrow windows, the dropdown selects component properties, Back
restores the sequence, raw clipboard data are exact, invalid inputs retain the
snapshot, and all controls work using keyboard navigation and AT-SPI. Verify
copy-information output and normalized text in an external application.
