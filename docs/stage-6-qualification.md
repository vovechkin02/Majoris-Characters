# Stage 6 qualification

Version: 0.6.0. Target: Debian 13 / Majoris OS.

## Executed checks

GCC compiled the five ICU-based native test executables with GNU11 and
`-Wall -Wextra -Werror`, linked against ICU 76.1. All passed:

| Test executable | Result |
| --- | --- |
| test-unicode-engine | 5 groups passed |
| test-character-search | 7 groups passed |
| test-character-inspector | 5 groups passed |
| test-legacy-encoding | 4 groups passed |
| test-emoji-catalog | 4 groups passed |

All five also passed AddressSanitizer and UndefinedBehaviorSanitizer with
leak detection enabled. Stage 6 covers exact variation identity and duplicate
suppression, skin tones, mixed-tone handshakes, profession/gender/hair/direction,
family and relationship alternatives, flags, keycaps, supported presentation
pairs, invalid input, locale fallback, Unicode case folding, and exact numeric
search. Existing catalog and inspector regression tests retain their full emoji
and named-sequence round trips. The annotation export contains 45,960 source
records across 24 languages. Regeneration from the compressed export produced
byte-identical C data. This does not prove completeness against CLDR 48.

The declaration checker detected 764/764 documented declarations. Python and
XML parsing, GResource source paths, all 70 GTK template field references,
Debian changelog version, and executable packaging rules passed source checks.
The declaration checker is lexical evidence, not a substitute for Valadoc or
compiler validation.

## Checks not run

The environment has GCC and ICU headers, but lacks the Vala/Meson,
GTK4/libadwaita and Fontconfig/Pango development toolchain and GLib schema
compiler. DNS resolution for external source retrieval failed. Full Meson
compilation, Vala bindings, strict schema compilation, native Fontconfig tests,
GUI runtime tests, GTK color glyph/shaping verification and Debian binary
package building were not run. Direct CLDR import of a complete release was not
qualified; the bundled export is not a complete Unicode 17 annotation dataset.
The full official standardized emoji presentation sequence data file was not
available; only catalog-backed presentation pairs are exposed.

## Remaining qualification

On a Debian 13 development host, install the build dependencies from
`debian/control` and Noto Color Emoji, then run:

```sh
meson setup build --prefix=/usr
meson compile -C build
meson test -C build --print-errorlogs
meson test -C build --suite integration --print-errorlogs
dpkg-buildpackage -us -uc -b
```

The Vala binding test and GTK browser interaction test exercise variation
selection and exact clipboard text. Manually confirm actual color shaping for
representative skin-tone, mixed-tone, ZWJ, flag, keycap and text-presentation
sequences on Majoris under Wayland. Check localized annotation searching with
multiple installed locales and verify that unsupported glyphs or separate
component rendering do not alter clipboard identity. Validate fonts and legacy
encodings from the previous stage as part of the full build. See
`emoji-functionality.md` for scope boundaries.
