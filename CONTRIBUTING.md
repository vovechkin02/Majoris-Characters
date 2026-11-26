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

## Native GLib contracts

Project-owned C interfaces use GLib types (`gboolean`, `gchar`, `gint`,
`guint`, `gsize`, fixed-width GLib integers, and `gunichar` for scalar values).
Keep opaque catalog identities and raw symbol codes distinct from Unicode
scalars. Preserve the types mandated by ICU, FreeType, Fontconfig, and Pango,
and retain generated and vendored declarations unchanged. Native modules
must declare GLib directly in their Meson dependencies and include the
required headers explicitly.

Return GLib-owned strings and arrays to Vala and release them with `g_free()`.
Use checked `g_try_*` allocation where failure is recoverable, including the
search scanner, normalization, encoding enumeration, and native catalog
lookups. The private `characters_try_strdup()` helper preserves the nullable
string allocation contract; ordinary `g_strdup()` is appropriate only when
fatal allocation behavior is acceptable. Keep compact typed vectors and
C11 atomic cancellation; do not introduce per-character GObjects or replace
bounded algorithms solely to use a GLib container. Use GLib cleanup helpers
where they simplify ownership without changing failure or lifetime semantics.
Keep VAPI length, type, and transfer annotations synchronized with native
headers and mixed-language tests.


Collection labels, group policies, and scalar presentation belong in the Vala
domain layer. Native search may consume copied, untranslated metadata vocabulary
and validated numeric predicates; do not reintroduce native UI formatters or
locale-dependent matching. Keep the exact text and visible glyph representation
separate, and preserve explicit scalar arrays for embedded NUL values.

Unicode relationship selection belongs in Vala, including version-specific case
rules, canonical component assembly, and reverse accent ranges. Keep raw ICU
simple mappings and recursive canonical decomposition separate from direct
compatibility decomposition. Native confusable access exposes immutable class
membership only; the Vala collector owns validation, deduplication and ordering.

Emoji annotation locale selection and fallback ordering belong in Vala. Fold
query operands once before scanning using the bounded ICU adapter. Native
lookup accepts resolved indexes and copied folded terms; do not reintroduce
per-candidate locale resolution or term folding. Exact presentation aliases
and their labels remain distinct from the immutable annotation keys.

Prepare search traversal phases and bounds in Vala, committing the plan only
after successful query and collection preparation. Native traversal must retain
compact typed identities, ordered results, explicit repertoire ownership,
recoverable allocation, and bounded cancellation. Prefer immutable property
spans and borrowed catalog scalars over per-candidate conversion. Any further
migration of the hot scan loops requires a compiled behavioral and performance
baseline; language statistics alone are not a justification.


Official data updates use the pinned maintainer import workflow in
`docs/official-data-import.md`. Keep formal Unihan properties and fallback
collation metadata distinct. Verify the source archive digests, stage generated
outputs before replacement, preserve Unicode notices, and report actual locale
coverage without treating English search fallback as a translation. Do not
mark a dataset as imported until its original release inputs were processed.

Keep grapheme classes, Indic conjunct properties and case mappings in the
pinned Unicode algorithm snapshot. Both offline search-index generation and
runtime queries must use the same folding semantics. Keep exact text and
metadata unchanged; only derived search operands may be folded. Official
Unicode test corpus acquisition and the optional conformance driver are
documented in `docs/unicode-algorithm-consistency.md`.

Keep desktop input-source discovery in the main-context Adapter. Resolve only
configured, exact IBus engine names using declared registry languages, never
by guessing from engine names or physical keyboard layout. Use cancellable
bounded IPC, immutable worker snapshots, and read-only desktop settings.
Unavailable or neutral engines contribute no scripts. Keep IBus library
ownership annotations accurate; do not pass live descriptors to search workers.

Keep saved collection and history-policy read-modify-write operations inside
`LibraryStore`'s coordinated transaction. Do not write the four collection
keys directly from views or private pickers. The native adapter owns the
advisory descriptor and backend snapshots, not collection policy. Font
comparison verification must open the exact file and face on the source
worker, never trust cached Fontconfig charset membership as proof of a glyph
or perform installed-font I/O on the GTK main context.
