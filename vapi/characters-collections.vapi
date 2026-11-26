/** Native advisory lock and settings snapshot adapter for saved collections. */
[CCode (cheader_filename = "collection-lock.h")]
namespace Characters.Collections {
    /** Owned per-user lock, released when the compact instance is destroyed. */
    [Compact]
    [CCode (cname = "CharactersCollectionLock", free_function = "characters_collection_lock_free",
        has_type_id = false)]
    public class Lock {
    }

    /** Acquire the shared lock within its bounded wait; null indicates failure. */
    [CCode (cname = "characters_collection_lock_acquire")]
    public static Lock? acquire (GLib.Settings settings);

    /** Create an independent settings snapshot, reopening keyfile backends when needed. */
    [CCode (cname = "characters_collection_snapshot")]
    public static GLib.Settings? snapshot (GLib.Settings settings);
}
