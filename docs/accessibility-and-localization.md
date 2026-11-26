# Accessibility and localization

## Keyboard Shortcuts

Use the main application menu, the compact picker's keyboard icon, or the
registered Ctrl+? accelerator to open the reference. Application accelerator
labels are read from the current `Gtk.Application` when the dialog is opened.
The global shortcut label reflects `picker-shortcut` from GSettings; this is
not evidence that the optional GNOME Shell extension is enabled or that the
binding is free. The private `--pick-text` picker hides full-application,
browser and composer instructions that are unavailable in that process.

The reference includes search focus, composer and picker activation, Quit,
Tab/Shift+Tab navigation, character grid movement, context-dependent Enter and
Escape, native composer editing, and the optional IBus candidate navigation.
The global shortcut and IBus sections describe their requirements explicitly.

## Accessibility behavior

- Recycled browser and font grid items expose their character name and complete
  code point notation on the GTK list item, including face/encoding details
  where applicable. Unbind clears the accessible label and old tile content.
- Saved collection rows expose character name, code points, and usage count
  independently of the visible glyph, and clear the values on recycling.
- Inspector preview and related-character controls expose descriptive names
  and complete code points. Existing accessible copy buttons are retained.
- Explicit page changes announce the visible range; compact-picker clipboard
  confirmation announces the result without exposing unrelated clipboard data.
  Search keystrokes are not individually announced by custom code.
- Controls and layout use native GTK4/libadwaita widgets and shared theme;
  the application does not implement a custom screen-reader or keyboard stack.

## Translations

Gettext catalogs are selected using the process locale and installed under the
`majoris-characters` domain. French, German, and Spanish cover all 950 active
literal messages in the checked-in template. English source messages remain
available as fallback. Script and named language collection labels use CLDR
48 and Babel 2.18.0 display names where available. Proper names without a
localized CLDR equivalent use their established name or a readable spelling.
Presentation-selector labels and descriptive fallback names are localized in
the UI without modifying the search vocabulary or raw metadata.

The inspector displays human-readable values for general category, bidi class,
decomposition type, East Asian width, boolean emoji/mirroring properties,
primary script and script extensions, and Unicode plane. Codes remain visible
where appropriate. Copying an individual property or a complete report retains
raw property values. Canonical names, aliases, Unicode block identifiers, exact
code points, bytes, font names, raw Unihan readings, query grammar and stored
identities are not translated. The block registry continues to use standard
English Unicode block names. AppStream descriptions are localized separately.

Babel is not required at build or runtime: the standard PO files are the
translator-maintained source. See [translation completion](translation-completion.md)
for extraction, catalog maintenance, and verification commands. All catalogs
require linguistic review and live GTK qualification.

## Manual target qualification

On a Debian 13/Majoris desktop, test with English, French, German and Spanish
locales. Open the full browser and picker, use the menu and shortcut reference,
inspect Latin, Arabic, and Japanese shared-script characters, and confirm
numeric property copies are identical to the English locale. Test large font
sizes, narrow windows, long translations, and right-to-left character content.
With Orca and AT-SPI enabled, traverse all controls with Tab and arrow keys,
check result names/selection, navigate pages, confirm announcements and verify
that recycled rows do not retain stale names. Test picker cancellation and
client-mode selection without modifying the clipboard. This requires the target
GTK runtime and real assistive technology; static label checks are not WCAG or
screen-reader certification.
