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

## Stage 7: composition and clipboard

`MainWindow` owns one `ComposerView` and its XML-backed native `GtkTextBuffer`.
The composer keeps session text and native edit history in memory only. Dialogs
receive a non-owning reference to the parent editor; nested dialogs pass the
same reference along. The parent window owns the editor and the presentation
lifecycle of those dialogs. Closing the window discards the editor. Adding text
expands the panel without stealing focus from the browser or dialog.

GTK owns editing, clipboard paste, bidirectional layout, selection, undo/redo,
and text drop behavior. Its text buffer is the only mutable composition model;
no duplicate editor state, custom Command/Memento implementation, or clipboard
service is needed. Each programmatic append, clear, or normalization is grouped
as one native user action. Clipboard and drop interactions use exact UTF-8, not
preview text. Recycled CharacterTile instances clear their drag text on unbind;
GDK owns a copied string value after drag preparation. Drag sources offer COPY
only, while the text view retains native editing behavior.

`composition.c` is a private bounded ICU bridge for extended grapheme counting
and conversion. It reuses the inspector's shared scalar formatter and Unicode
normalizer. Technical operations run synchronously on explicit input of at
most 16,384 scalars (and normalization output is checked against the same
bound). The count update checks GTK's scalar count before extracting text.
The existing 32-scalar inspector contract is unchanged. ICU's grapheme result
is version-dependent. No new mutable native service or pattern hierarchy is
introduced. See [composition](composition.md) for limits and interaction rules.

New window actions are `win.open-composer`, `win.add-character`, and
`win.copy-character`. The new resource is `ui/composer-view.ui`; installed IDs,
GSettings keys, native inspection snapshot semantics, and previous actions
remain unchanged. ICU i18n is linked for grapheme segmentation via the existing
Debian `libicu-dev` build dependency.

## Stage 8: saved collections and preferences

`LibraryStore` is the local repository for exact-text favorites and usage
history. It owns a GSettings reference and uses the main context for all
operations; no worker or native Unicode catalog index participates in persisted
identity. The two independent collections use bounded string arrays; usage
counts and recency order are written together in a single setting. Frequency
is derived from immutable `UsageEntry` snapshots. No separate ranking database,
clock-dependent timestamp, global Singleton, or custom history manager is
required. See [saved collections and preferences](library-and-preferences.md)
for the key contracts and privacy boundaries.

| Decision | Pattern / idiom | Ownership and contract |
| --- | --- | --- |
| Shared local repository | Direct repository composition | MainWindow owns LibraryStore; composer and dialogs retain it explicitly. No speculative interchangeable backend. |
| Collection changes | Observer via native GObject signals | The settings backend delivers changes on the owning main context. LibraryDialog disconnects on close/dispose; MainWindow disconnects its own settings observer on close. |
| Frequency ordering | Direct stable bounded sort | UsageEntry values own their text; counts saturate, ties preserve recency. No Strategy hierarchy for a fixed ranking policy. |
| Preferences | Native GSettings property bindings | Bindings follow widget lifetimes; renderer fallback does not change cmap coverage. |
| Collection list | GTK model/view and recycled rows | GtkSingleSelection owns a bounded GtkStringList; selection is restored by exact identity rather than transient catalog ID. |

Copies of original characters are recorded at the explicit action boundary.
ComposerView accounts for explicit appends so nested dialogs do not record the
same insertion twice. Metadata and arbitrary text editing remain untracked.
The three existing window geometry keys and previous application actions retain
their contracts. Stage 8 adds schema keys, XML templates, and `win.favorites`,
`win.recent`, `win.frequent`, `win.save-favorite`, and `win.preferences` actions.
The app has no public library ABI. No new package dependencies are needed.


## Stage 9: desktop integration

The application exports the standard `org.gnome.Shell.SearchProvider2` interface
at `/com/majoris/characters/SearchProvider` in addition to GApplication's
existing actions. `SearchProvider` owns at most two running native jobs and
cancels stale requests; waiting invocations check their generation before
starting. A strong local application reference and GApplication hold span
asynchronous requests. The provider's application field is weak, and unexport
cancels work. Results are limited to 20 copyable identities represented by
canonical Unicode code notation, rather than catalog-local integer indexes.
The provider publishes metadata without changing user data; activating a result
opens the existing inspector. The Home menu can use the same provider protocol.

`CompactPicker` is an independent GTK application window, using the existing
`SearchController` and `LibraryStore`. It owns its model widgets, disconnects
subscriptions on close, and displays up to 80 results. The application holds
its clipboard owner until external replacement or explicit Quit, avoiding a
picker-only process exiting immediately after a copy. `DesktopSearch` supplies
the bounded query/result rules for both desktop search and the headless input
lookup command.

| Decision | Pattern / idiom | Contract |
| --- | --- | --- |
| Shell provider | Adapter | Translates the fixed external SearchProvider2 protocol into existing native requests and inspector activation; no new provider hierarchy. |
| Compact picker | Direct composition and GObject Observer | GTK owns controls; the picker disconnects its search and library observers on close. |
| IBus engine | Framework adapter | IBus owns the current context; the Python engine queries a bounded local CLI protocol and commits only explicitly selected, current candidates. |
| Shortcut extension | Framework integration | Registers one GSettings-based binding and removes it on disable; no input interception or shell private API for injection. |

IBus requests are process-isolated from the application UI. The Python engine
cancels obsolete searches and rejects malformed results, then calls the native
IBus commit interface for the focused client. It does not synthesize keyboard
input or use the clipboard. Explicit insertion history is routed through the
primary application, respecting the existing retention and opt-out policy.
No external service or new public library ABI is introduced. Existing actions,
application identity and GSettings keys are preserved. See [desktop integration](desktop-integration.md)
for installation, protocol contracts and runtime boundaries.
