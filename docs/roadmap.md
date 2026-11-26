# Development roadmap

| Stage | Scope | Status |
| --- | --- | --- |
| 1 | Vala app, XML UI, Meson, resources, GSettings, theme, Debian packaging | Source implemented; build/runtime qualification pending |
| 2 | Unicode engine, GNOME C reuse and Vala bindings, database and metadata | Core implemented; native tests passed; Unicode 17 scalar snapshot and normalization delta implemented; official data conformance and Vala qualification pending |
| 3 | Virtualized character browsing, scripts, blocks, search and filters | Source implemented; native search tests passed; Vala and GTK qualification pending |
| 4 | Inspector, relationships, encodings, normalization, and identification | Source implemented; native tests passed; Vala and GTK qualification pending |
| 5 | Installed fonts, coverage, variations, legacy encodings | Source implemented; ICU legacy tests passed; font backend and GTK qualification pending |
| 6 | Emoji sequences, annotations and variations | Core implemented; native tests passed; annotation coverage and GTK qualification limited |
| 7 | Composer, clipboard and drag-and-drop | Source implemented; native tests passed; GTK qualification pending |
| 8 | Favorites, recent/frequent history and preferences | Source implemented; strict schema and native regressions passed; Vala/GTK qualification pending |
| 9 | Compact picker, desktop search and native insertion | Source implemented; protocol/native checks passed; desktop and IBus qualification pending |
| 10 | Accessibility, localization, performance, regression and ISO integration | Qualification candidate; see Stage 10 report for executed and outstanding gates |

The scope is the Linux-native equivalent of Windows Character Map, macOS
Character Viewer and GNOME Characters. No AI, vendor accounts, Microsoft/Apple
services or external clipboard history are required. GNOME Characters data and algorithms are incorporated in Stage 2; the GNOME
application and GJS are not runtime dependencies of the main application. The optional Shell extension uses GJS.

## Post-stage priorities

- Priority 1: Unicode 17 scalar properties and normalization core implemented;
  official normalization and supplement conformance remains outstanding.
- Priority 2: International browsing, script extensions, configured XKB scopes,
  Han search and metadata import implemented; full official Unihan acquisition
  and runtime GUI qualification remain outstanding. See international-browsing.md.

- Priority 3: Standardized presentation and selector-omission lookup, exact
  search, structural families, CLDR 48 English additions, and locale coverage
  tooling implemented. Full multilingual CLDR import and official emoji-test
  file comparison remain outstanding.
- Priority 4: Exact selected-face rasterization, variable-axis controls and raw
  Microsoft Symbol cmap inspection implemented at source level; native font and
  GTK qualification remain outstanding.
- Priority 5: Expanded legacy repertoire, GB18030-2022 and EUC mappings,
  contextual canonical scalar browsing, and strict multi-scalar conversion
  implemented. Native encoding contracts passed; GTK qualification remains
  outstanding.
- Priority 6: Direct scalar decomposition, Linux Unicode entry guidance, 17-plane
  browsing, reserved/noncharacter inspection, and 27 semantic collections
  implemented. Native contracts passed; GTK qualification remains outstanding.
- Priority 7: Compact picker category filters, bounded full-result pagination,
  frequent history, favorites management, inspector and variation actions,
  state-preserving expansion and private client selection implemented at source
  level. GTK and client integration tests remain to be run. Next functional
  priority: accessibility and localization.
- Priority 8: Native keyboard shortcuts reference, accessible result labels and
  announcements, localized Unicode property descriptions and expanded French,
  German, and Spanish catalogs implemented. Translation and assistive-technology
  review and target GTK qualification remain outstanding. Next priorities:
  external Desktop Shell/ISO integration, then full release qualification.

- Vala migration pass 1 (1.0.9): inspection, scalar formatting, composition
  validation, and emoji variation-family policy moved to Vala. Remaining native
  application policy migration is deferred until full compilation and parity
  benchmarks are available. Generated and vendored data are classified for
  GitHub Linguist without altering their contents.

- Vala migration pass 2 (1.0.10): query syntax and numeric decoding, language
  collection selection, and related-character assembly are implemented in Vala.
  The compact native scanner and scalar lookup primitives remain. Full Vala
  compilation and performance parity are required before further hot-path
  migration. See [pass 2 qualification](vala-migration-pass-2.md).

- Vala migration pass 3 (1.0.11): semantic filters and Han query policies moved
  to Vala, preserving prepared native scanning.
- Vala migration pass 4 (1.0.12): legacy encoding workflows moved to Vala,
  preserving strict ICU operations and compact mapping enumeration.
- Vala migration pass 5 (1.0.13): font inventory validation, ordering, grouping,
  selection restoration, metadata and comparison filtering moved to Vala.
  Fontconfig discovery and exact FreeType operations remain native. See
  [pass 5 qualification](vala-migration-pass-5.md). Establish full compilation
  and parity before consolidating additional native adapters or moving scan loops.

- Vala migration pass 6 (1.0.14): shared scalar validation and bounded ICU
  conversion, centralized text normalization and encoding, and private raw
  symbol storage. Native search traversal, immutable database access and font
  rasterization remain native. See [pass 6 qualification](vala-migration-pass-6.md).
  Establish a full build and benchmark baseline before additional migration.
