# Persistence and font coverage refinements — 1.0.25

## Scope and compatibility

Revised Priority 5 addresses concurrent saved collection mutations and exact
font comparison coverage. The installed application, settings schema, key
names, exact UTF-8 identities, `count:text` records, history limits, actions,
command-line protocol, and Unicode data are unchanged. There is no schema
migration, external storage service, account integration, or new linked
runtime library. `dconf-gsettings-backend` is now a Debian build-test dependency
so the production backend can be qualified on an isolated D-Bus session.

## Coordinated collection transactions

`LibraryStore` remains the sole application mutation policy. Add, remove,
reorder, record, clear, history enable/disable, and retention updates all
follow one transaction boundary:

1. Obtain an exclusive advisory lock for the current configuration directory.
2. Flush this process's pending GSettings writes and open a fresh settings
   object with the same schema, path, and backend. Keyfile backends reopen the
   actual file instead of relying on asynchronous file-monitor notifications.
3. Read the latest values, validate, and stage the requested mutation using
   the independent object's delayed-apply mode.
4. Apply, synchronize, and compare the four collection keys against the
   intended values. Revert a rejected staged mutation without applying it.
5. Release the lock, then notify the owning view. No write proceeds unlocked.

The native Adapter (`native/library/collection-lock.c`) owns only the POSIX
file descriptor and GIO snapshot boundary. The Vala compact binding releases
it deterministically. `Collections.snapshot()` preserves the schema, path,
backend, and keyfile administrator defaults. GSettings signals continue to
provide Observer delivery; external policy repair is coalesced and retried
at most three times outside the settings notification stack. Clean history
snapshots need no repair write. A failed snapshot hides history rather than
exposing possibly stale usage data.

The lock file is `$XDG_CONFIG_HOME/majoris-characters/collections.lock`, with
GLib's configuration directory fallback. It is an empty private 0600 file,
opened with close-on-exec and no final-component symlink following. The
adapter rejects nonregular files, different owners, multiple links, or
permissions granted to other users. The inode is not unlinked after release.
Kernel descriptor cleanup releases the lock on process termination. Acquisition
polls nonblocking `flock()` against a monotonic 500 ms deadline. The in-memory
backend is process-local and uses no filesystem lock.

The existing synchronous Boolean mutation contract is preserved: lock
contention, backend rejection, or read-back disagreement reports failure.
The acquisition deadline bounds lock waiting, not the backend's own `sync()`
I/O; end-to-end desktop responsiveness still requires qualification. No claim
of forensic erasure or crash-proof storage durability is made.

Recording and its policy are serialized. In-app opt-out stages disablement
and clearing together. If an administrator has locked the history key,
recording can still be disabled and the UI reports that old records could not
be cleared; re-enabling cannot restore those records without clearing them.
Limit changes and eviction are staged together. Favorites remain independent.

### Supported concurrency scope

The intended GNOME backend is dconf, whose per-key changes do not overwrite
unrelated preferences. Cooperating updated Characters instances, including
private pickers and the insertion command, use the same lock. The isolated
GLib keyfile backend is also supported for cooperating collection transactions.
A keyfile backend can rewrite its entire file, so arbitrary unrelated direct
keyfile writers (including ordinary uncoordinated preference bindings using
that optional backend) can still overwrite other keys. External dconf editors,
older app versions, custom backend implementations, and tools that bypass the
lock are not serialized by this advisory protocol. External settings edits
remain observed and repaired on a best-effort basis; rapid external opt-out
and re-enable outside the transaction is not guaranteed to be atomic.

## Exact font comparison coverage

Fontconfig still discovers installed paths, face indexes, families, styles,
and capability metadata. It is no longer the authority for cmap membership.
For each face matching the label query, the native adapter opens the exact
file and FreeType face/named-instance index, selects the Unicode charmap,
and requires `FT_Get_Char_Index()` to return a nonzero glyph index. It does
not use cached Fontconfig charset membership even as a negative prefilter.
The lightweight probe does not enumerate axes, load symbol maps, rasterize
pixels, or substitute another font. Missing or unreadable files and symbol-only
faces are excluded. Valid empty mapped glyphs, such as space, are included.
Scalar coverage does not guarantee visible ink, a variation-selector mapping,
composite shaping, or rasterization. A newly installed face still requires
inventory refresh; existing stale file labels may also change on refresh.

The existing `FontSourceLoader` now owns comparison requests in addition to
inventory, face and encoding requests. The immutable sorted inventory and
query are retained by the worker. Label filtering precedes file opening;
cooperative cancellation is checked between faces. The existing supersession
contract prevents obsolete results from publishing. Closing the dialog does
not wait for the worker. No new thread pool, backend hierarchy, or per-glyph
cache is introduced. Target latency and memory measurements remain pending.

## Qualification

| Check | Result |
| --- | --- |
| Release source contracts | 19 passed |
| Input protocol tests | 3 passed |
| Unicode algorithm snapshot tests | 3 passed |
| Emoji data tests | 5 passed |
| Unihan importer tests | 3 passed |
| Isolated official archive import tests | 3 passed |
| Official production manifest and output digest verification | Passed |
| Native collection lock and snapshot runtime probe | Keyfile and dconf scenarios passed |
| Exact FreeType coverage fixture assertions | 14 passed |
| Stale Fontconfig inventory/file replacement probe | Passed |
| Declaration documentation | 1,900 of 1,900 detected declarations |
| XML, UI, SVG, JSON, Python, PO parsing | Passed |
| Strict GSettings schema validation | Passed |
| Debian changelog and archive integrity | Passed |
| Full bundled Han regeneration test | Not run |
| Official development-header native build | Not run |
| Vala store, worker, and GUI tests | Not run |
| Full application compilation and Debian binary packaging | Not run |

The new runtime probes are supplemental, not a substitute for the real
application tests. GCC compiled the actual collection-lock module, actual
font-catalog module, and extracted lightweight coverage function with
`-Wall -Wextra -Werror`, using temporary minimal GLib/GIO/Fontconfig/FreeType/
Pango declarations because official development headers are unavailable.
The linked libraries were the installed GLib/GIO, dconf, FreeType and
Fontconfig runtimes. The persistence driver exercised a temporary equivalent
policy harness invoking the compiled native lock and snapshot functions:
four processes retained 32 independent additions and all 32 shared-use
increments, a concurrent removal survived, a stale writer could not repopulate
history after disablement, limit changes trimmed records, lock contention
rejected writes, and process termination released the advisory lock. Both
keyfile and isolated D-Bus dconf backend scenarios passed. The font probe
used the new exact-coverage assertions and the actual stale catalog test
functions with the supplied TTC, variable and symbol fixtures. Neither
probe executes the Vala application. Temporary declarations, harnesses,
compiled objects, and local runtime data are not included in the ZIP.

The production regression tests are registered in Meson. On Debian 13 with
all build dependencies installed, run the full gate, or specifically:

```sh
meson test -C build --print-errorlogs \
  'Saved collection ordering and privacy' \
  'Saved collections across process restarts' \
  'Installed font catalog contracts' \
  'Exact coverage with stale font catalog' \
  'Exact font face and variation contracts' \
  'Font and legacy source bindings and supersession'
```

Also exercise rapid comparison query changes, close/refresh during active
comparison, multi-picker add/remove and count changes, administrator-locked
history keys, and desktop opt-out while another picker is open. Validate
responsiveness under lock contention and a slow dconf service. No generated
Unicode or upstream source data has changed; all 41 original data files are
byte-identical to 1.0.24.

## Follow-up

Revised Priority 6 is completing the existing French, German, and Spanish
interface translations and refreshing gettext extraction. Full compiler and
runtime qualification remains an independent release blocker.
