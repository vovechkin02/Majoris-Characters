# Contributing

Follow Majoris Cross-Project Engineering Standards v1.8. Keep project-owned
identifiers and documentation in American English, and document every
handwritten type, constructor, member, method, and function using Valadoc or
PEP 257 as appropriate. Use inferred Vala local types where their exact type and
ownership are clear; retain explicit types at contracts and numeric boundaries.

Use XML UI templates and native GTK/libadwaita widgets. Do not override the
shared theme without a documented semantic need. Keep the entry point thin,
manage GObject ownership explicitly, and avoid speculative classes or patterns.
Use multiline executable braces, four-space Vala/Python indentation and two-space
Meson/XML indentation. Preserve existing identifiers, settings and install paths.

Before submitting, run the commands in the README and report any unavailable
checks as not run. Review documentation coverage and meaning, and keep package
rules executable. Do not add generated build files, caches or binary packages to
source archives. If adopting upstream code later, retain its exact license and
copyright notices and record the source revision and modifications.

For localization changes, preserve normative Unicode identifiers and exact copied
values. Mark all new user-visible messages for gettext extraction and update
`po/POTFILES.in` when adding resources. Do not translate search syntax, format
placeholders, application IDs or schema keys. Use the native accessibility
properties of interactive GTK controls, including clearing recycled item labels
on unbind. Record incomplete translation and screen-reader qualification honestly.
