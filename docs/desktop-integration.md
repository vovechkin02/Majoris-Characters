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
Emoji (default), Symbols, Favorites or Recently Used, and search within that
collection. The catalog search is asynchronous and canceled on replacement.
The view shows up to 80 exact identities; refine the query for additional
results. Click a character or press Enter from the search field to copy it.
Keyboard navigation through the result buttons is provided by GTK. Escape
closes the picker; the full browser remains independent. The header's expand
button transfers the query to the full catalog. The picker stays open after a
copy. The application retains its local clipboard ownership while needed,
including after the picker closes. Normal clipboard limitations still apply;
explicit Quit ends the process. NUL cannot be represented in a GTK text clipboard.

The compact picker is a clipboard interface, not a privileged cross-application
text injection interface. Paste into the target application normally.

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

The engine executes `majoris-characters --input-search=QUERY` as a local child
process. That headless option emits at most 20 lines, each containing base64
UTF-8 identity and name separated by a tab. No shell is involved. The query is
visible to processes permitted to inspect its command line; no network or
logging of search contents is used by the engine. The Python decoder validates
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
