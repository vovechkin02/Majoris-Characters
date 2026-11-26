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
The immutable scalar snapshot supplies Unicode 17 properties; ICU supplies
stable names and normalization primitives. The shared normalization module
applies the Unicode 17 delta. See
[Unicode engine](unicode-engine.md) for contracts and data versions.

The native interface is an adapter-like boundary over ICU and GNOME data, not a
claim of a full GoF Adapter hierarchy. There is one concrete engine and no
interchangeable backend requirement. The internal library allocates strings
using the system allocator (GLib 2.46+ uses the same allocator on Linux).
Borrowed catalog strings are immutable and process-lifetime. Related vectors
transfer ownership to the caller. Stage 2 itself creates no worker or callback.

## Stage 3 boundaries

`src/search/query-parser.vala` owns bounded query parsing. The native scanner owns
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

`src/unicode/inspection-snapshot.vala` owns immutable scalar snapshots and safe
previews. `sequence-format.vala` provides explicit numeric representations; the
small native `icu-adapter.c` preserves length-aware ICU normalization. It reuses
the search parser for explicit code points and the Unicode engine for names,
relationships, and catalog data. The snapshot is independent of the search
request and remains valid after the grid model changes or the worker is freed.
A NUL is stored as a scalar, not a C string terminator. All input snapshots have
1–32 scalars; normalized output may expand beyond that limit and is formatted
without truncation. ICU normalization for unassigned data is withheld.

`CharacterInspector` is an XML-backed adaptive `Adw.Dialog`, owned by the
window's dialog presentation lifecycle. It owns one Vala snapshot, a bounded
history of 32 previous numeric identities, and a plain-text report. All access
and GTK updates occur on the main context; no new worker is created. The domain
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
and the periodic check. Pango descriptions are used for fallback-enabled presentation, and nominal Pango
resolution is checked for substitution. The selected face uses actual FreeType
Unicode cmap enumeration. No application-wide typeface is overridden.

Priority 4 introduces `CharactersFontFace`, an adapter over FreeType's native
file/index, cmap, variation and bitmap contracts. `FontSourceRequest` opens and
owns the selected face on its worker; ownership transfers once after publication,
and subsequent mutable operations are confined to the GTK main context.
`FontFaceDialog` owns a separate face and coordinate vector; `FontPreview`
converts owned RGBA bytes to GDK textures. The browser consumes the same adapter
for fallback-disabled cells. A failed raster is not replaced by a Pango glyph.
Raw symbol identifiers are never routed through Unicode actions. This is a
focused native adapter and composition boundary, not a new Strategy hierarchy.
Original TTC, variable and symbol fixtures exercise identity and lifecycle
contracts. The direct rendering paths and Vala adapter require target build and
runtime qualification.

The legacy source builds an immutable scalar-to-byte mapping table with ICU's
strict decoder. Independent units retain alternate mappings, enumerated using
bounded one- through four-byte grammars. Contextual encodings instead enumerate
ICU's round-trip set and verify each standalone scalar. Canonical bytes are
computed on demand; the table never claims to enumerate arbitrary stateful
streams. The source worker owns construction and publishes only the latest
request; large GB18030 repertoires reuse the existing bounded grid and search.
`LegacySequenceDialog` captures the encoding index and delegates bounded,
length-aware conversion to fresh strict ICU converters. It keeps original bytes
on decode and only accepts encoding when a reverse conversion reproduces the
input exactly. The existing character inspector owns sequence metadata and
Unicode actions. The native ICU integration is an adapter with a fixed bounded
conversion contract; a new strategy hierarchy or duplicate clipboard service is
not warranted. See [legacy encodings](legacy-encodings.md).
The repertoire filter
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

`src/emoji/variation-family.vala` owns structural grouping and `variants.vala`
constructs exact alternatives. `emoji-catalog.c` owns immutable annotation and
sequence lookup tables. The
variant key exists only for comparison; all copyable identities are exact
catalog entries, except explicitly supported scalar presentation choices.
`Characters.Emoji.Variants` owns its strings and exposes borrowed immutable values
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

`src/unicode/composition.vala` owns bounded input validation and conversion
policy. It reuses the shared scalar formatter and Unicode normalizer;
`icu-adapter.c` supplies ICU extended grapheme counting. Technical operations run synchronously on explicit input of at
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
subscriptions on close, and displays up to 80 results per page. The application holds
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

## Stage 10: qualification and hardening

The compact picker shares the existing settings backend and disconnects its
preview-size observer alongside its search and collection observers. The same
exact identities and native clipboard contracts remain in place. The input
adapter now transports its query through the child process's standard-input
pipe. DesktopSearch validates an EOF-terminated, at-most-512-byte UTF-8 query
before invoking the existing engine; the old argument-based CLI remains
compatible. Modified shortcuts cancel an active input query rather than
committing a candidate. Explicit insertion history still uses the existing
primary application command. No new service or speculative pattern hierarchy
is introduced. Native English metadata and identifiers remain stable; interface
labels can be translated without changing search results. Source-level
release checks complement, but do not replace, actual GTK and desktop testing.

### International collections (Priority 2)

The native Han and language lookup modules own immutable generated data and
expose borrowed labels or owned property strings. The search request resolves
language filters once, using a bounded snapshot of configured XKB source IDs;
its worker owns all subsequent matching state. Script-extension predicates
reuse the Unicode property snapshot without allocating per-character strings.
The main window observes optional GSettings input-source changes and disconnects
on closure; the existing request controller handles cancellation/supersession.
This uses native Observer signals and a direct data lookup rather than speculative
strategy hierarchies. Formal Han properties and fallbacks have separate contracts.


## Priority 3: exact emoji sequence identities

`generate_emoji_sequences.py` produces a sorted immutable table from the pinned
presentation bases and catalog selector omissions. `emoji-catalog.c` owns
binary search and canonical association; Vala owns structural family selection. The
Unicode engine uses the same lookup for related characters and inspector names.
The search engine reserves a separate private ID interval for supplementary
identities and emits them only for exact queries. Existing catalog and named
sequence IDs remain unchanged. The supplementary IDs are not persisted. The
application continues to use GSettings text identities for persistence and
GDK text for clipboard operations. This is a direct table lookup and domain
policy, not a new GoF pattern hierarchy. All native lookups are immutable and
thread-safe; variant allocations belong to the picker. Tests verify all bases,
exact query behavior, existing family contracts and deterministic generation.


## Priority 6: scalar inspection and browsing refinements

Plane and reserved-scalar scopes append to the existing native enumeration and
use the same single-run search request and virtualized GTK page model. The plane
bounds are selected before scanning; assigned and unassigned values form a
partition of the valid scalar space. Existing catalog-local identities remain
unchanged. Semantic collections use indexed immutable range rules and the
bundled Unicode 17 decomposition property. A direct mapping query uses ICU raw
normalization mappings and the known Unicode 17 delta, then the shared sequence
formatter; it deliberately differs from recursive normalization. The inspector
reuses its navigation action and owned snapshot for the mapping. GTK's existing
settings observers handle new sidebar visibility keys. No new GoF hierarchy is
needed: these are extensions of existing native adapters and declarative views.


## Priority 7: complete compact picker

`CompactPicker` retains a completed `SearchRequest` and constructs at most 80
native FlowBox cells at a time. Saved identities are bounded by `LibraryStore`;
`LibraryQuery` centralizes matching for both the compact and full saved views.
The picker uses native selection events (Observer), not a custom event bus or
separate emoji engine. The existing `EmojiVariations` dialog can delegate its
explicit selection to the invoking picker, which owns delivery and history
recording. Normal dialog behavior remains unchanged for other callers.
Catalog expansion transfers the native filter and absolute result position;
the full browser re-runs its request and restores the selected ID, with stale
handoffs discarded by the existing search controller. Saved expansion restores
exact text, not a catalog-local numeric ID. Preference changes repaint the
current page instead of restarting a completed search.

The private `--pick-text` entry point runs a non-unique application instance.
Its stdout pipe is a narrow subprocess boundary: a single exact identity on
success, no output on cancellation, and no clipboard replacement. The caller
owns insertion and cancellation; no general input-injection service or extra
D-Bus API is introduced. This is direct composition with two delivery paths,
not a speculative Strategy hierarchy. The standard picker retains its existing
single-instance and clipboard hold contract, including nested dialog copies.
The existing library settings backend is shared across processes; concurrent
mutations are subject to its existing last-writer behavior. No new runtime
dependency or schema migration is required. GUI lifecycle, exact handoff, and
client output tests are registered in the existing integration test profiles.

## Priority 8: accessibility and localization

`ShortcutsDialog` presents the existing application accelerators through GTK's
accelerator parser and localized label formatter. It reads the configured
Shell shortcut from GSettings when opened and does not modify it. Contextual
picker, composer and IBus guidance is declarative XML. The private client picker
hides unavailable application/browser/composer sections. No new shortcut
registry or custom key event framework is introduced.

`PropertyLabels` is a presentation formatter, not a data source or an additional
Unicode engine. It resolves script codes through the native immutable snapshot,
uses gettext for display names and translates enumerated scalar properties.
The browser retains native filter indexes; the inspector retains raw property
values for individual copies and metadata reports. Canonical names, sequence
identities and persisted settings remain unchanged. Extraction markers on the
native tables do not change the native search or inspection contracts.

GTK list item accessibility labels are assigned during factory binding and
cleared on unbind, including font and saved-collection rows. Explicit paging
and picker clipboard notifications use GTK 4.14 accessibility announcements;
existing GTK keyboard navigation, selection state and toast infrastructure are
retained. No custom Observer, Adapter or event bus is needed for these ordinary
presentation responsibilities. The full GTK integration suite and manual
screen-reader checks remain required to validate the runtime behavior.

## Vala migration boundary

The Vala inspection and composition services share scalar validation and a single
formatter. Vala owns the exact snapshot and variation collection lifetimes.
Native `Icu` bindings adapt explicit-length scalar arrays to ICU UTF-16 operations;
`Scalar` and the catalog bindings borrow immutable metadata. Returned buffers
are owned by Vala and have explicit lengths, so NUL remains a scalar. This is a
focused library Adapter and direct composition of domain services, not a new
pattern hierarchy. The Vala `QueryParser` and `LanguageSelection` interpret input on the existing
search worker and supply a copied, prepared query and script mask. The native
scanner retains compact integer results, bitset deduplication, hot metadata
matching and atomic cancellation. `SemanticFilters` supplies a bounded union of
category/decomposition/range clauses; `HanQuery` supplies folded term payloads,
field masks and validated numeric operands. The scanner evaluates these without
interpreting query prefixes or hardcoding semantic collection ranges. `Relationships` assembles, deduplicates and
sorts exact text alternatives from native scalar candidates and emoji catalog
lookups. Native scalar enumeration avoids per-candidate GObjects. The property
tables, ICU normalization, FreeType rasterizer, Fontconfig inventory and legacy
converters remain in C. No new pattern hierarchy or public ABI is introduced.
See [pass 3 qualification](vala-migration-pass-3.md),
[pass 2 qualification](vala-migration-pass-2.md), and the earlier
[pass 1 qualification](vala-migration.md).
