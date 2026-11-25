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
