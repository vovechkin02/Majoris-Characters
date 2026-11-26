#include "collection-lock.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

/** Maximum time waiting for another cooperating process, in microseconds. */
#define LOCK_TIMEOUT_US (500 * G_TIME_SPAN_MILLISECOND)
/** Sleep between nonblocking advisory lock attempts. */
#define LOCK_POLL_US (5 * G_TIME_SPAN_MILLISECOND)

/** Holds a descriptor until the owning transaction finishes or the process exits. */
struct CharactersCollectionLock {
    /** Negative only for the process-local memory backend. */
    gint descriptor;
};

/** Obtain the current backend's concrete identity without creating a new default backend. */
static gboolean is_memory_backend(GSettings *settings)
{
    GSettingsBackend *backend = NULL;
    g_object_get(settings, "backend", &backend, NULL);
    gboolean memory = backend && strcmp(G_OBJECT_TYPE_NAME(backend),
        "GMemorySettingsBackend") == 0;
    g_clear_object(&backend);
    return memory;
}

/** Open a stable, private regular lock file without following a final-component symlink. */
static gint open_lock_file(void)
{
    g_autofree gchar *directory = g_build_filename(g_get_user_config_dir(),
        "majoris-characters", NULL);
    if (g_mkdir_with_parents(directory, 0700) != 0) {
        return -1;
    }
    g_autofree gchar *path = g_build_filename(directory, "collections.lock", NULL);
    gint descriptor = open(path, O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_RDWR | O_NONBLOCK, 0600);
    if (descriptor < 0) {
        return -1;
    }
    struct stat metadata;
    if (fstat(descriptor, &metadata) != 0 || !S_ISREG(metadata.st_mode) ||
        metadata.st_uid != geteuid() || metadata.st_nlink != 1 || (metadata.st_mode & 0077)) {
        close(descriptor);
        return -1;
    }
    return descriptor;
}

/** Wait for an exclusive advisory lock using a monotonic, bounded acquisition deadline. */
static gboolean lock_descriptor(gint descriptor)
{
    gint64 deadline = g_get_monotonic_time() + LOCK_TIMEOUT_US;
    do {
        if (flock(descriptor, LOCK_EX | LOCK_NB) == 0) {
            return TRUE;
        }
        if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR) {
            return FALSE;
        }
        g_usleep(LOCK_POLL_US);
    } while (g_get_monotonic_time() < deadline);
    return FALSE;
}

CharactersCollectionLock *characters_collection_lock_acquire(GSettings *settings)
{
    if (!settings) {
        return NULL;
    }
    CharactersCollectionLock *lock = g_try_new0(CharactersCollectionLock, 1);
    if (!lock) {
        return NULL;
    }
    lock->descriptor = -1;
    if (!is_memory_backend(settings)) {
        lock->descriptor = open_lock_file();
        if (lock->descriptor < 0 || !lock_descriptor(lock->descriptor)) {
            characters_collection_lock_free(lock);
            return NULL;
        }
    }
    return lock;
}

void characters_collection_lock_free(CharactersCollectionLock *lock)
{
    if (!lock) {
        return;
    }
    if (lock->descriptor >= 0) {
        close(lock->descriptor);
    }
    g_free(lock);
}

/** Reopen the same keyfile backend, preserving its path and administrator defaults. */
static GSettingsBackend *reopen_keyfile(GSettingsBackend *backend)
{
    g_autofree gchar *filename = NULL;
    g_autofree gchar *root_path = NULL;
    g_autofree gchar *root_group = NULL;
    g_autofree gchar *defaults_dir = NULL;
    g_object_get(backend, "filename", &filename, "root-path", &root_path,
        "root-group", &root_group, "defaults-dir", &defaults_dir, NULL);
    if (!filename || !root_path) {
        return NULL;
    }
    return g_object_new(G_OBJECT_TYPE(backend), "filename", filename,
        "root-path", root_path, "root-group", root_group, "defaults-dir", defaults_dir, NULL);
}

GSettings *characters_collection_snapshot(GSettings *settings)
{
    if (!settings) {
        return NULL;
    }
    GSettingsBackend *backend = NULL;
    GSettingsSchema *schema = NULL;
    g_autofree gchar *path = NULL;
    g_object_get(settings, "backend", &backend, "settings-schema", &schema,
        "path", &path, NULL);
    if (backend && strcmp(G_OBJECT_TYPE_NAME(backend), "GKeyfileSettingsBackend") == 0) {
        GSettingsBackend *fresh = reopen_keyfile(backend);
        g_object_unref(backend);
        backend = fresh;
    }
    GSettings *snapshot = backend && schema && path ?
        g_settings_new_full(schema, backend, path) : NULL;
    g_clear_object(&backend);
    if (schema) {
        g_settings_schema_unref(schema);
    }
    return snapshot;
}
