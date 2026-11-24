# Development roadmap

| Stage | Scope | Status |
| --- | --- | --- |
| 1 | Vala app, XML UI, Meson, resources, GSettings, theme, Debian packaging | Source implemented; build/runtime qualification pending |
| 2 | Unicode engine, GNOME C reuse and Vala bindings, database and metadata | Core implemented; native tests passed; full Unicode 17 properties and Vala qualification pending |
| 3 | Virtualized character browsing, scripts, blocks, search and filters | Not started |
| 4 | Inspector, relationships, encodings and normalization | Not started |
| 5 | Installed fonts, coverage, variations, legacy encodings | Not started |
| 6 | Emoji sequences, annotations and variations | Not started |
| 7 | Composer, clipboard and drag-and-drop | Not started |
| 8 | Favorites, recent/frequent history and preferences | Not started |
| 9 | Compact picker, desktop search and native insertion | Not started |
| 10 | Accessibility, localization, performance, regression and ISO integration | Not started |

The scope is the Linux-native equivalent of Windows Character Map, macOS
Character Viewer and GNOME Characters. No AI, vendor accounts, Microsoft/Apple
services or external clipboard history are required. GNOME Characters data and algorithms are incorporated in Stage 2; the GNOME
application and GJS are not runtime dependencies.
