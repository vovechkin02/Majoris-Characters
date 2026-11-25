# Priority 7 qualification — 1.0.7

## Implemented source changes

- Compact browsing retains the complete immutable native result, using 80 cells
  per page. Emoji and Symbols expose the existing nine and 27 native category
  slots; all six compact scopes support query and paging. Frequent history
  reuses the stable ordering from LibraryStore. Saved queries share one policy
  with the full collection dialog. Preference changes rebind a completed page
  rather than starting another native search.
- Selectable accessible cells expose exact name and code point metadata. Copy,
  Enter, and cell activation preserve the original identity; NUL is inspectable
  but cannot be copied as text. Favorites can be added or removed independently
  of history. The existing emoji variation selector delegates its chosen text
  to the compact picker. Inspector and nested dialog copy operations use the
  normal application clipboard hold when rooted in the compact picker.
- Expansion restores native scope, category, query, absolute result position
  and selection in the full browser, or the saved collection's filter and
  text selection in its full dialog. The existing controller cancels stale
  results. Hidden category settings are not overwritten by a handoff.
- `--pick-text` starts a separate non-unique, ID-less application instance.
  On an explicit selection it returns exact UTF-8 on stdout, no terminator,
  and exits 0; cancellation exits 1 without output. It never claims the
  primary desktop identity or replaces the clipboard. Cooperating callers
  own their native editor insertion target and cancellation. No insertion
  into unrelated clients is attempted. Private mode hides full-browser
  expansion and the ordinary copy-capable inspector. The existing IBus
  input-source mechanism and global picker remain unchanged.

## Verification actually executed

Native Unicode sources (unchanged by this pass) were rebuilt with GCC,
GNU11, `-O2 -Wall -Wextra -Werror` and ICU 76.1, then all six suites were run.

| Check | Result |
| --- | --- |
| Unicode engine | 6 groups passed |
| Character search | 12 groups passed |
| Character inspector | 6 groups passed |
| Legacy encoding | 7 groups passed |
| Emoji catalog | 7 groups passed |
| Composition | 4 groups passed |
| Input protocol | 3 Python tests passed |
| Unihan import | 3 Python tests passed |
| Emoji data | 5 Python tests passed |
| Release source contracts | 4 Python tests passed |
| Strict GSettings schema dry run | Passed |
| Declaration documentation | 1,453/1,453 detected |
| Python AST and UI XML syntax | Passed |
| Debian changelog | 1.0.7-1 parsed successfully |

The updated Vala integration driver exercises category counts, paging,
selection, preview resizing, exact copy, variants, favorites, inspector,
frequent history, and catalog/saved-collection handoff. The private picker
GUI driver and Python wrapper check exact byte output, cancellation and
unchanged clipboard/history. These are added source tests, not executed
results. The local CLI test also rejects mixed `--pick-text` arguments.

## Not run / remaining qualifications

No Vala compiler, Meson, GTK4/libadwaita or font development headers are
available in this container; therefore native font compilation, Vala compilation,
all GTK interaction tests (including the new picker and client tests), and
Debian binary package creation are unverified. Sanitizers were not rerun.
Run the existing full release gate on Debian 13. Keyboard, screen reader,
Wayland/X11, exact client insertion, clipboard retention and font rendering
must be qualified in the actual desktop. No external application, Desktop Shell
or ISO Builder repository has been modified. The cooperating-client interface
requires the caller to adopt the documented protocol; the standalone picker
still copies text, not arbitrary input injection. Official Unihan and complete
CLDR annotations remain open. This is a qualification candidate, not a stable
release certification.
