# Stage 5 qualification

Version: 0.5.0. Target: Debian 13 / Majoris OS.

## Executed checks

The four native ICU-based test executables were compiled with GCC using
`-Wall -Wextra -Werror`, linked against ICU, and executed successfully:

| Test executable | Result |
| --- | --- |
| test-unicode-engine | 5 groups passed |
| test-character-search | 7 groups passed |
| test-character-inspector | 5 groups passed |
| test-legacy-encoding | 4 groups passed |

All four executables also passed AddressSanitizer and UndefinedBehaviorSanitizer
runs with leak detection enabled. The encoding test covers the 35 available
ICU converters, invalid input, alternate byte mappings, exact-byte queries,
restricted repertoire search, and cancellation. This does not qualify the
Fontconfig backend or actual GTK rendering.

## Checks not run

The delivery environment lacks the Vala/Meson and GTK4/libadwaita development
toolchain and Fontconfig/Pango development headers. Attempted dependency
installation failed because Debian package mirror DNS resolution failed.
The native font catalog, Vala bindings, full Meson build, font-source request
supersession tests, GTK interaction tests and Debian binary package build
therefore remain unqualified. Do not interpret source implementation as a
successful compile or runtime check.

On a Debian 13 development host, install the dependencies listed in
`debian/control`, then run:

```sh
meson setup build --prefix=/usr
meson compile -C build
meson test -C build --print-errorlogs
meson test -C build --suite integration --print-errorlogs
dpkg-buildpackage -us -uc -b
```

Manually verify font enumeration, exact face coverage, substitution notes,
private-use mappings, font installation/removal refresh, font comparison,
preview fallback and size, and color/variable font behavior with representative
installed fonts. Verify byte lookup and copying through the GTK clipboard.
No actual font rendering or desktop performance measurements were made here.
