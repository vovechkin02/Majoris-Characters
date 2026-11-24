# Composition and clipboard

The composer is a collapsible panel below the main browser. Use the main menu
or Ctrl+M to open it, or choose Add to Composer in the browser, inspector, font
or legacy browser, or emoji variation dialog. Adding a character appends the
exact UTF-8 sequence at the end without replacing the editor's selection. The
panel opens automatically and the current browser search remains unchanged.

The native GTK text view supports multiline and bidirectional text, selection,
keyboard editing, cut/copy/paste, undo/redo, and text drag-and-drop. Character
and font tiles offer copy-only drag content based on their bound identity,
not their rendered glyph. Drop text into the expanded editor or another
compatible text destination. Within the editor, GTK owns standard text drag
behavior and keyboard editing. No synthetic keystrokes or clipboard polling
are used. Undo history is limited to 100 native levels. Composer contents and
history are not persisted or included in a settings schema.

Copy Text always copies the complete buffer. The native Copy Selection action
copies only the selected range. Copy Format, Normalize, and Inspect Text use
the selection when present, otherwise the entire buffer. Copy Format supports
Unicode hexadecimal and decimal notation, UTF-8 bytes, UTF-16 and UTF-32 code
units, HTML decimal and hexadecimal numeric references, fixed-width Unicode
escapes, and CSS escapes. These are textual representations; UTF-16/32 are
not byte-order-specific binary streams. HTML and CSS parsers can reinterpret
some values. No conversion is applied to ordinary clipboard copies.

Normalization is never implicit. Select NFC, NFD, NFKC, or NFKD and choose
Normalize. The operation replaces the target in one undoable action; selected
ranges remain selected after replacement. Normalizing a selection does not
normalize across its boundary with adjacent unselected text. Unavailable ICU
properties or oversized normalized output produce a message and leave the
original unchanged.

The status line reports Unicode scalar count, UTF-8 byte count, and extended
grapheme clusters according to the installed ICU segmentation data. Grapheme
clusters are not the same as rendered glyphs, and Unicode version differences
may change segmentation of newer sequences. Counts and technical operations
are limited to 16,384 scalars; larger text can still be edited and copied using
GTK, and a smaller selection can be converted. The inspector accepts up to 32
scalars: select a smaller range to inspect longer compositions. Embedded NUL
cannot be carried in ordinary GTK UTF-8 text; its numeric notation remains
available in the existing inspector. Copy and Add are disabled for identities
without representable text. The application never substitutes a preview glyph
or silently strips combining marks or presentation selectors.

The native contracts are exercised by `test-composition`; the real GTK browser
integration test additionally covers append, exact clipboard data, grouped
undo/redo, normalization, format copying, and inspector navigation. Manual
Wayland and X11 checks are still required for actual drag-and-drop, clipboard
ownership across applications, native keyboard shortcuts, and bidi layout.
