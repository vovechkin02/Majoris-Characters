# Stage 2 qualification — 0.2.0

Native engine qualification was performed on Debian 13.3 using GCC and ICU 76.1.
The Vala compiler, GTK/libadwaita development libraries, Meson and GLib schema
compiler were not available in the container; DNS resolution of the Unicode
website failed. No assertion of full application compilation is made.

| Check | Result |
| --- | --- |
| Native C compilation (`-Wall -Wextra -Werror`) | Passed |
| Native unit suite, five groups | Passed |
| All 346 block range endpoints and ordering | Passed |
| All 3,944 emoji entries, group membership and round-trip | Passed |
| All 461 named sequences and round-trip | Passed |
| Scalar validity, metadata, aliases and relationship samples | Passed |
| UTF-8, four normalization forms, invalid inputs | Passed |
| AddressSanitizer and UndefinedBehaviorSanitizer, leak detection | Passed |
| Supplement regeneration, byte-for-byte comparison | Passed |
| Documentation lexical/AST check | Passed — 156/156 detected declarations |
| Python syntax and XML syntax | Passed |
| Debian changelog version and rules permissions | Passed |
| Full GNOME database regeneration from official sources | Not run — inputs unavailable |
| Vala bindings compilation and native/Vala test | Not run — toolchain unavailable |
| Meson application build, CLI and GTK integration | Not run |
| GSettings strict schema compiler | Not run in this environment |
| Debian package build and installed application checks | Not run |

Native test command (independent of GTK):

```sh
cc -std=gnu11 -Wall -Wextra -Werror -UNDEBUG -I native/unicode \
  $(pkg-config --cflags icu-uc) native/unicode/unicode-engine.c \
  tests/unit/test-unicode-engine.c $(pkg-config --libs icu-uc) \
  -o /tmp/test-unicode-engine
/tmp/test-unicode-engine
```

Full application qualification remains necessary on the intended development
host. Install the build dependencies from `debian/control`, configure Meson,
compile, and run the full suite. The Vala binding test covers array-length and
ownership contracts and the CLI test covers the actual application linkage.
The native results do not substitute for those tests. Full Unicode 17 metadata
coverage is separately limited by available ICU and data versions, as described
in [Unicode engine](unicode-engine.md).
