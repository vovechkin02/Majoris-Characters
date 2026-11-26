#ifndef CHARACTERS_ALLOCATION_H
#define CHARACTERS_ALLOCATION_H

#include <string.h>

#include <glib.h>

/** Duplicate a NUL-terminated string with GLib ownership and recoverable allocation failure. */
static inline gchar *characters_try_strdup(const gchar *text)
{
    if (!text) {
        return NULL;
    }
    gsize length = strlen(text) + 1;
    gchar *copy = g_try_malloc(length);
    if (copy) {
        memcpy(copy, text, length);
    }
    return copy;
}

#endif
