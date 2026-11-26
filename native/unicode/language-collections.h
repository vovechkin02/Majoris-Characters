#ifndef CHARACTERS_LANGUAGE_COLLECTIONS_H
#define CHARACTERS_LANGUAGE_COLLECTIONS_H

#include <glib.h>

/** The property snapshot uses guint8 script IDs. */
#define CHARACTERS_LANGUAGE_SCRIPT_LIMIT 256

/** Number of language collections, including locale and configured keyboard sources. */
gsize characters_language_count(void);

/** Borrow the English label at an index, or NULL for an invalid slot. */
const gchar *characters_language_name(gsize index);

/** Borrow a locale identifier for a named language slot; NULL for special slots. */
const gchar *characters_language_tag(gsize index);

/** Borrow space-separated ISO language codes for an exact XKB layout or variant. */
const gchar *characters_language_layout_languages(const gchar *id);

/** OR resolved ICU script IDs into a caller-owned 256-byte snapshot mask. */
void characters_language_add_locale(const gchar *locale, guint8 *scripts, gsize length);

#endif
