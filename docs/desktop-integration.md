# Desktop integration

Stage 9 adds three independently usable entry points. The main application
remains Vala/GTK4/libadwaita on Debian 13. All new desktop hooks operate only
within the current user's desktop/input session.

## Compact picker

- CLI: `majoris-characters --picker`.
- Desktop action: `picker` in `com.majoris.characters.desktop`.
- Application action: `app.picker` (Ctrl+Shift+Period within Characters).
- D-Bus action: `gapplication action com.majoris.characters picker`.

The picker is an independent 400 × 480 logical-pixel default window. Opening it
again presents the existing picker, not a new copy. Select All Characters,
Emoji (default), Symbols, Favorites, Recently Used or Frequently Used. Emoji
and Symbols expose the same nine and 27 category filters as the full browser.
Catalog queries are asynchronous and canceled on replacement. The view honors
the saved grid preview size. Previous/Next page through the complete result
set with at most 80 constructed cells; saved collections use the same filter
and ordering policies as their full dialogs.

Select a character to inspect its name and code points. Copy Character or
activate a cell (double-click or keyboard activation) to copy. Enter in the
search field uses the selection or the first result. Ctrl+F focuses search;
Escape closes the picker. Variations opens the existing catalog-backed emoji
family selector, including skin tones. Its explicit copy uses the picker's
clipboard and usage policy. Add to Favorites/Remove Favorite changes the
independent saved collection; Inspect Character opens the shared inspector.
NUL may be inspected but cannot be copied or returned as GTK text.

The expand button transfers the collection, native category, query, absolute
result position and selected identity into the full application. Catalog
handoff re-runs the same asynchronous request and locates the exact catalog
identity before selecting it on the full browser's 512-result page. Saved
collections open their full dialog with the filter and selected text restored.
When the relevant sidebar row is hidden by preferences, the handoff still
retains its scope without overriding the user's visibility setting.

The picker stays open after a copy. The application retains local clipboard
ownership while needed, including after the picker closes. Normal clipboard
limitations still apply; explicit Quit ends the process.

## Private selection for cooperating applications

Launch `majoris-characters --pick-text` as a child process using an argument
vector (no shell), with stdout captured as bytes and stderr separate. This
option must be used alone. It starts a non-unique application instance and
never exports or takes over the primary desktop search provider. The normal
picker and global shortcut remain independent.

| Child exit status | Stdout contract |
| --- | --- |
| 0 | One exact UTF-8 identity, 1–32 scalars, at most 128 bytes, no terminator |
| 1 | Canceled by closing the picker; no output |
| 2 | Invalid invocation or output error; do not consume stdout |
| Other nonzero | Startup or process failure; do not consume stdout |

The private picker offers the same catalog, categories, pages, saved collections
and variation selection. Insert Character (or cell activation) returns the
exact text and closes the picker. It does not touch the clipboard, inject
keystrokes, use a global input context, or record speculative usage. Expansion
and the ordinary inspector are hidden in private mode to prevent alternate
clipboard workflows. Favorites remain explicitly editable.

The caller should launch asynchronously (for example with `Gio.Subprocess`
and `communicate_utf8_async`), own the target editor or text buffer, retain only
a valid insertion target, and cancel/terminate its child on document closure.
After completion, accept status 0 only, validate nonempty UTF-8, reject embedded
NUL and oversized output, and insert the unmodified sequence through the
caller's own editor API. Do not strip whitespace, normalize, interpret as a
command, or send it to another application. A caller may use the existing
`--record-insertion=TEXT` command after confirmed insertion if appropriate for
its privacy policy. Avoid invoking the private picker in sensitive inputs.
There is no automatic insertion into unrelated Wayland clients; the ordinary
clipboard picker and optional IBus path serve those separate use cases.

## Home menu and GNOME Shell search

The package installs the GNOME-compatible provider INI in
`/usr/share/gnome-shell/search-providers/` and the existing session D-Bus service.
The provider is enabled by default in its metadata; desktop search settings may
still disable it. It can be activated through the application's D-Bus name
without opening its main window. The provider contract is:

- Bus name: `com.majoris.characters`
- Object path: `/com/majoris/characters/SearchProvider`
- Interface: `org.gnome.Shell.SearchProvider2`

| Method | Input | Output |
| --- | --- | --- |
| GetInitialResultSet | `as` terms | `as` identities |
| GetSubsearchResultSet | `as` previous identities, `as` terms | `as` identities |
| GetResultMetas | `as` identities | `aa{sv}` metadata |
| ActivateResult | `s` identity, `as` terms, `u` timestamp | none |
| LaunchSearch | `as` terms, `u` timestamp | none |

A result identity is canonical `U+XXXX` notation (multiple scalars separated by
spaces), stable across catalog index changes. Queries are limited to 512 UTF-8
bytes and results to 20 copyable identities. Metadata includes ID, name,
description (complete code points), and application icon. Activating a result
opens its inspector, where the user can explicitly copy or append it; merely
searching does not change the clipboard or usage history. LaunchSearch opens
the full browser with its filters reset. Refined search uses the full catalog,
not only a truncated previous result set. Obsolete requests are canceled.

Example:

```sh
gdbus call --session --dest com.majoris.characters \
  --object-path /com/majoris/characters/SearchProvider \
  --method org.gnome.Shell.SearchProvider2.GetInitialResultSet "['U+20AC']"
```

A custom Majoris Home menu that enumerates GNOME search providers can consume
this INI directly. If it does not, its own repository must integrate the above
contract. The Majoris Desktop Shell repository was not supplied for Stage 9;
no changes to it or to the ISO builder are included here.

## Global shortcut

The package installs an optional GNOME Shell 48 extension at
`/usr/share/gnome-shell/extensions/characters-picker@majoris.com`. It registers
one binding in normal desktop and overview modes, with no lock-screen binding.
After installation, log out and back in if necessary for Shell to discover it:

```sh
gnome-extensions enable characters-picker@majoris.com
gsettings set com.majoris.characters picker-shortcut "['<Super>period']"
```

Disable it with `gnome-extensions disable characters-picker@majoris.com`.
The default is Super+Period; use the GSettings key to choose an unoccupied
shortcut or an empty array to clear it. The package does not enable the extension,
overwrite existing user shortcuts, or replace any existing Home menu bindings.
A custom shell may instead bind its own shortcut to the desktop action or CLI;
do not enable both if they would conflict. Shell versions other than 48 have
not been declared compatible. On other desktop environments, bind the CLI
command using that desktop's supported keyboard settings.

## Optional native insertion with IBus

The package recommends `ibus`, `gir1.2-ibus-1.0`, and `python3-gi`. If omitted,
install them before enabling the input source:

```sh
sudo apt install ibus gir1.2-ibus-1.0 python3-gi
```

The installed IBus component registers the `majoris-characters` engine.
After a new login (or an appropriate IBus restart), add the Characters input
source from the desktop's input-source settings, under Other if applicable.
Select it as the active input source in an IBus-compatible application. It
passes ordinary keys through while idle. Ctrl+Shift+Period opens its native
preedit/candidate session. Type a character name, literal sequence or code
point. Arrow keys and Page Up/Down navigate; Enter or a candidate click
commits the exact Unicode sequence. Escape cancels. The input source does not
use the clipboard, simulate keystrokes, read the active application's text, or
require privileged compositor access. It does not replace existing language
IMEs; switch input sources to use them. The compact GTK picker remains
available independently.

The engine executes `majoris-characters --input-search-stdin` as a local child
process, writing the bounded UTF-8 query through its standard-input pipe. That
headless option emits at most 20 lines, each containing base64 UTF-8 identity
and name separated by a tab. No shell is involved. The query is not included in
process arguments; no network or logging of search contents is used by the
engine. The original `--input-search=QUERY` CLI remains available for compatibility. The Python decoder validates
the exact sequence, length, count and output budget. Focus loss, reset,
disable, or supersession cancels pending work and invalidates callbacks. The
engine declines password, PIN, and private input where the client declares
those purposes/hints. It uses IBus preedit clear-on-focus-loss mode, not commit.
The commit uses IBus's current focused input context, not a previously captured
window. Explicit insertions are recorded through the primary application
(`--record-insertion=TEXT`), subject to the existing history preference; failure
to record history does not undo a commit.

Direct insertion is available only when the Characters source is selected and
the target toolkit/application supports IBus. Wayland and X11 behavior depends
on the input-method integration of the session. There is no claim of direct
insertion from the standalone GTK picker into arbitrary applications or of
operation inside secure input fields. Full desktop, shell, and IBus runtime
qualification is still required on Majoris.
