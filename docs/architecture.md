# Architecture

## Stage 1 boundaries

`main.vala` initializes gettext and delegates to `Characters.Application`.
The application owns the GTK/GApplication lifecycle, actions and current window.
It uses the framework's single-instance session bus registration, not an
application-defined singleton or service locator. `MainWindow` owns its GSettings
instance, restores the normal requested logical dimensions on construction, and
saves state when closing. All current application and UI operations run on the
GTK main context. There are no worker threads or asynchronous jobs in Stage 1.

The main window template and icon are compiled into the executable as GResources.
The icon, desktop entry, AppStream metadata, GSettings schema and D-Bus service
are also installed into the corresponding standard data directories. GApplication
uses the reverse-DNS application ID for its default resource base path and D-Bus
identity. The Meson project version is the source of the runtime version string.
Desktop/AppStream and Debian release metadata are maintained together per release.

Stage 2 adds an internal native Unicode engine. The app links a private static
library and consumes its VAPI, not ICU or generated table internals. The engine
owns no mutable global state and has no UI dependency. Its catalog is immutable;
ICU handles standard property and normalization semantics. See
[Unicode engine](unicode-engine.md) for contracts and data versions.

The native interface is an adapter-like boundary over ICU and GNOME data, not a
claim of a full GoF Adapter hierarchy. There is one concrete engine and no
interchangeable backend requirement. The internal library allocates strings
using the system allocator (GLib 2.46+ uses the same allocator on Linux).
Borrowed catalog strings are immutable and process-lifetime. Related vectors
transfer ownership to the caller. Stage 2 itself creates no worker or callback.

## Stage 3 boundaries

`native/unicode/character-search.c` owns bounded query parsing, classification,
collection traversal, compact result identities and sequence-aware display
representations. It reuses the immutable engine APIs and ICU, and has no GTK
or GLib dependency. `SearchRequest` owns a native request across worker and
main-context references. `SearchController` serializes the latest pending
request and suppresses obsolete completion. Native cancellation is atomic;
results are only read after main-context publication. The GTK main context owns
all widgets, model replacement, selection, and status changes.

`MainWindow` composes the controller, GTK selection and bounded page model.
`CharacterTile` owns the two XML-backed labels and exposes a bind/clear contract;
its factory creates and recycles tiles without retaining native results. The
window disconnects the controller's completion signal on normal close or
GObject disposal and cancels the active request. The worker's closure keeps its
native request alive through idle delivery; the window is not retained by the
worker. See [character search](character-search.md) for query and lifecycle
contracts.

## Stage 4 boundaries

`character-inspector.c` owns immutable scalar snapshots, explicit numeric
representations, safe previews, and length-aware ICU normalization. It reuses
the search parser for explicit code points and the Unicode engine for names,
relationships, and catalog data. The snapshot is independent of the search
request and remains valid after the grid model changes or the worker is freed.
A NUL is stored as a scalar, not a C string terminator. All input snapshots have
1–32 scalars; normalized output may expand beyond that limit and is formatted
without truncation. ICU normalization for unassigned data is withheld.

`CharacterInspector` is an XML-backed adaptive `Adw.Dialog`, owned by the
window's dialog presentation lifecycle. It owns one native snapshot, a bounded
history of 32 previous numeric identities, and a plain-text report. All access
and GTK updates occur on the main context; no new worker is created. The native
operations inspect at most 32 scalars, fixed catalog tables, and bounded related
classes. A component selector limits displayed property rows to one scalar,
while reports contain all components. Generated row actions carry exact values
as `GVariant` targets rather than capturing their parent dialog in closures.
Only explicit actions copy to the GDK clipboard. No inspector data is persisted.
See [character inspector](character-inspector.md) for user and API contracts.

## GoF applicability and lifecycle

| Area | Decision | Ownership and rationale |
| --- | --- | --- |
| Application activation | Framework lifecycle, not GoF Singleton | GApplication provides one primary process per session bus. No global mutable instance accessor. |
| Actions | Native GAction, command-like idiom | Application owns actions; simple immediate callbacks, no custom Command hierarchy or undo queue. |
| Window construction | Direct constructor | One fixed XML template; no factory or builder abstraction is warranted. |
| Settings | Direct GSettings composition | Window owns its settings object. No alternate storage backends are required at this stage. |
| Signals | Framework observer-like idiom | Actions and receivers use GTK's main context. SearchController publishes completion through a GObject signal; the window disconnects on close/disposal. No independent event bus. |
| Search supersession | Direct composition and single-worker scheduling | SearchController owns active and latest pending requests; no custom Command, Strategy, or State hierarchy is needed. |
| Native search boundary | Adapter-like binding | SearchRequest owns one native job and translates ownership and completion to GObject lifetime. There is no interchangeable backend requirement. |
| Grid tile recycling | GTK framework factory idiom | SignalListItemFactory owns setup, bind and unbind callbacks; CharacterTile clears every field on reuse. No application-defined Factory Method hierarchy. |
| Inspection snapshot | Direct immutable data object | Native snapshot owns its scalar array; no Memento pattern is claimed for the simple bounded code-point navigation history. |
| Inspector actions | Framework action group, command-like idiom | One dialog owns its action group, snapshot, and generated targets; GTK dispatches immediate callbacks, without a custom Command hierarchy. |

There are no claimed custom GoF pattern instances through Stage 4. The architecture
favors composition and native framework extension points. Revisit pattern
applicability when real backend or algorithm variation is introduced.

## Compatibility surfaces

- Application and D-Bus name: `com.majoris.characters`.
- Executable and package: `majoris-characters`.
- GSettings path: `/com/majoris/characters/`.
- Keys: `window-width` and `window-height` (integer logical pixels, range
  320–8192, default 960×720), `window-maximized` (boolean, default false).
- Actions: `app.about`, `app.quit`, `win.search`, `win.reset-search`, `win.inspect-character`, and `win.identify-character`.
- Resource paths: `/com/majoris/characters/ui/main-window.ui` and
  `/com/majoris/characters/ui/character-tile.ui`, and
  `/com/majoris/characters/ui/character-inspector.ui`.
- Search IDs are private, catalog-local, and not a persisted compatibility surface.

Window geometry is a GTK logical-size request, not a guarantee of compositor
allocation. The window manager controls placement. The standard normal-close
path persists settings; an abrupt process termination need not save changes.
The GUI integration test uses 800×600 under Xvfb; it verifies stored logical
settings, not physical allocations or native Wayland behavior.

## Stage 5: font and character-set sources

The Fontconfig catalog owns a private configuration and an inventory of faces
identified by file and face index, not by localized labels. Coverage is derived
from the face's mapped scalar set. A worker owns the source request until its
main-context completion transfers the immutable result. `FontSourceLoader`
serializes inventory and repertoire operations and suppresses canceled or
superseded completions; the existing search controller independently filters a
copied, bounded repertoire. This is idiomatic asynchronous ownership, not a new
pattern hierarchy. No source operation waits for a worker on the GTK thread.

`FontBrowser` owns its dialog state, preview controls, source loader, search
controller, and periodic freshness check. Closing the dialog stops publication
and the periodic check. Pango descriptions are used only for presentation;
Fontconfig file/index identity remains the coverage authority. Nominal Pango
resolution is checked and substitution is reported rather than presented as
exact coverage. No application-wide typeface is overridden.

The legacy source builds an immutable scalar-to-byte mapping table with ICU's
strict decoder. Alternate mappings remain available. The repertoire filter
excludes additional catalog sequences and restricts all search results to the
selected source. Search identity helpers accept valid mapped but unassigned
scalars, while ordinary All Characters remains restricted to assigned scalars
and catalog sequences. Explicit numeric inspector construction is used for
mapped unassigned identities. Existing application IDs and settings remain
unchanged. New window actions are `win.browse-fonts`, `win.browse-encodings`,
and inspector action `inspect.font-variations`.

See [font inspection](font-inspection.md) for operational boundaries and
[Stage 5 qualification](stage-5-qualification.md) for evidence.

## Stage 6: emoji variation and annotation catalog

`emoji-catalog.c` owns variant grouping and immutable annotation tables. The
variant key exists only for comparison; all copyable identities are exact
catalog entries, except explicitly supported scalar presentation choices.
`CharactersEmojiVariants` owns its strings and exposes borrowed immutable values
until release. The dialog copies a value before handing it to another inspector
or clipboard. Catalog-sized bounded work is performed only when opening a picker;
no global mutable variant cache or new framework is introduced. This is a direct
catalog lookup and composition, not a speculative Strategy or Factory hierarchy.

Search jobs capture their locale before worker execution. Annotation lookups use
immutable locale records and ICU case folding, so they introduce no shared
mutable state or new cancellation protocol. The existing worker and selection
contracts remain unchanged. New window action `win.emoji-variations` and inspector
action `inspect.emoji-variations` open the XML-backed `EmojiVariations` dialog.
Existing IDs, GSettings, installed paths, and clipboard semantics are preserved.
See [emoji functionality](emoji-functionality.md) for the coverage boundaries.
