#ifndef CHARACTERS_COLLECTION_LOCK_H
#define CHARACTERS_COLLECTION_LOCK_H

#include <glib.h>
#include <gio/gio.h>

/** Owns the per-user advisory lock for one synchronous collection transaction. */
typedef struct CharactersCollectionLock CharactersCollectionLock;

/**
 * characters_collection_lock_acquire:
 * @settings: Undelayed settings instance belonging to the caller.
 *
 * Returns: (transfer full) (nullable): Exclusive lock, or NULL on failure or timeout.
 * The memory backend is process-local and does not acquire a filesystem lock.
 * All persistent application writers must use the same lock before reading or writing.
 */
CharactersCollectionLock *characters_collection_lock_acquire(GSettings *settings);

/** Release the advisory lock; safe for NULL. Never unlink the shared lock file. */
void characters_collection_lock_free(CharactersCollectionLock *lock);

/**
 * characters_collection_snapshot:
 * @settings: Undelayed settings instance belonging to the caller.
 *
 * Returns: (transfer full) (nullable): Independent settings object with the same schema,
 * path and backend. Keyfile backends are reopened to read the current on-disk state
 * without relying on main-context file-monitor delivery. Memory and dconf retain
 * their existing backend. The caller owns the returned object.
 */
GSettings *characters_collection_snapshot(GSettings *settings);

#endif
