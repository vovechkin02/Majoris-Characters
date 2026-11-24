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

There are no claimed custom GoF pattern instances in Stage 3. The architecture
favors composition and native framework extension points. Revisit pattern
applicability when real backend or algorithm variation is introduced.

## Compatibility surfaces

- Application and D-Bus name: `com.majoris.characters`.
- Executable and package: `majoris-characters`.
- GSettings path: `/com/majoris/characters/`.
- Keys: `window-width` and `window-height` (integer logical pixels, range
  320–8192, default 960×720), `window-maximized` (boolean, default false).
- Actions: `app.about`, `app.quit`, `win.search`, and `win.reset-search`.
- Resource paths: `/com/majoris/characters/ui/main-window.ui` and
  `/com/majoris/characters/ui/character-tile.ui`.
- Search IDs are private, catalog-local, and not a persisted compatibility surface.

Window geometry is a GTK logical-size request, not a guarantee of compositor
allocation. The window manager controls placement. The standard normal-close
path persists settings; an abrupt process termination need not save changes.
The GUI integration test uses 800×600 under Xvfb; it verifies stored logical
settings, not physical allocations or native Wayland behavior.
