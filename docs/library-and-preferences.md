# Saved collections and preferences

## User workflows

Use **Add to Favorites** from the main browser, character inspector, emoji
variation picker, font browser, or legacy encoding browser. The Favorites
sidebar entry and menu command open the ordered local collection. Select an
identity to copy, inspect, append to the composer, or remove it. Move Up and
Move Down adjust the manual order; clear the search field before reordering.
Recently Used and Frequently Used open independent views of the same retained
history. They provide per-entry removal, explicit saving to Favorites, and a
confirmed Clear History command. All three dialogs support searching by
literal text, name, or Unicode notation. Collection search does not change the
main browser query or filters.

Preferences provides history recording and retention, grid and font preview
sizes (12–72 points), font preview fallback, and visibility of the Emoji,
Symbols, Scripts, Unicode Blocks, Named Sequences, Languages, Radical Groups,
Stroke Groups, Fonts, and Character Sets
sidebar entries. All Characters and saved collections remain accessible. If
an active native scope is hidden, the browser switches to All Characters.
Hiding an entry does not remove any catalog content or disable the Fonts and
Character Sets menu actions. Size changes rebind the visible grid page and
preserve its selected identity; font-browser size/fallback controls and
Preferences share the same settings keys.

## Persistence and identity contracts

The existing `com.majoris.characters` GSettings schema gains these keys.
Existing window keys, application ID, D-Bus name and package paths are unchanged.

| Key | Type | Default | Contract |
| --- | --- | --- | --- |
| `favorites` | `as` | `[]` | Exact UTF-8 strings in user-defined order |
| `usage-history` | `as` | `[]` | Newest-first `count:text` records |
| `history-enabled` | `b` | `true` | Enable explicit usage recording |
| `history-limit` | `i` | `128` | 1–512 distinct retained history identities |
| `grid-preview-size` | `i` | `24` | Main character grid, 12–72 pt |
| `font-preview-size` | `i` | `24` | Font/legacy grid, 12–72 pt |
| `font-fallback` | `b` | `false` | Preview fallback only, not coverage |
| `show-emoji`, `show-symbols`, `show-scripts`, `show-blocks`, `show-named`, `show-languages`, `show-radicals`, `show-strokes`, `show-planes`, `show-unassigned`, `show-fonts`, `show-encodings` | `b` | `true` | Sidebar visibility |

Favorites allow up to 512 independent identities. Each retained identity
must be nonempty valid UTF-8 with at most 32 Unicode scalars; embedded NUL
cannot be carried by GSettings strings or the GTK text clipboard. Text is
not trimmed or normalized. Canonically equivalent but byte-distinct sequences
remain distinct, as do emoji with different modifiers or presentation
selectors. The record format uses the first colon as a separator; subsequent
colons belong to the identity. Counts must be positive 32-bit unsigned
integers. Duplicate/malformed entries are ignored when reading; history is
sanitized on policy changes and when written. Counts saturate at the maximum
unsigned value instead of overflowing.

A recorded use moves the identity to the beginning of recency history and
increments its count. Frequency is descending retained count with recency as
the stable tie breaker. Reducing the limit immediately drops older identities
and their counts; raising it does not resurrect them. The count is not a
lifetime statistic for evicted entries. Favorites are unaffected by recency,
frequency, eviction or history removal.

## Privacy and failure behavior

Only explicit original-character clipboard copies and Add to Composer actions
are recorded. Metadata/encoding copies, native text editing, whole-composition
copies, arbitrary external clipboard contents, browsing, searching, and
ordinary drag operations are not recorded. An explicitly inspected short
sequence may be recorded if the user chooses Copy Character or Add to Composer.
Saving a favorite itself does not increment usage. History is not the system
clipboard history. No account, synchronization service, telemetry, or network
access is used.

Disabling recording clears retained usage records and suppresses future
recording; favorites are kept. Clear History requires confirmation. GSettings
writes can fail when an administrator locks keys or the backend is unwritable;
collection mutations report failure, and disabled history is hidden even if an
old record cannot be erased. The Preferences dialog reports a retained-history
failure on opt-out. Clearing a key is not a forensic secure erase of storage,
backend journals or backups. The app does not control external clipboard
managers or data copied to other applications.

## Implementation

`LibraryStore` owns the bounded GSettings contract, validation, stable ordering,
and privacy policy. `UsageEntry` is an independent immutable count snapshot.
`LibraryDialog` uses a bounded GtkStringList, GtkSingleSelection and recyclable
GtkListView, derives safe display from the existing inspector, and preserves
selection by exact text when records change. `PreferencesDialog` uses native
bidirectional GSettings bindings. The main window owns the shared repository
and passes it to transient dialogs and the composer. No duplicate storage
service or custom event bus is introduced.
