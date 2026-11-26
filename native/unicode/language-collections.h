#ifndef CHARACTERS_LANGUAGE_COLLECTIONS_H
#define CHARACTERS_LANGUAGE_COLLECTIONS_H

#include <stdint.h>
#include <stddef.h>

/** The property snapshot uses uint8_t script IDs. */
#define CHARACTERS_LANGUAGE_SCRIPT_LIMIT 256

/** Number of language collections, including locale and configured keyboard sources. */
size_t characters_language_count(void);

/** Borrow the English label at an index, or NULL for an invalid slot. */
const char *characters_language_name(size_t index);

/** Borrow a locale identifier for a named language slot; NULL for special slots. */
const char *characters_language_tag(size_t index);

/** Borrow space-separated ISO language codes for an exact XKB layout or variant. */
const char *characters_language_layout_languages(const char *id);

/** OR resolved ICU script IDs into a caller-owned 256-byte snapshot mask. */
void characters_language_add_locale(const char *locale, uint8_t *scripts, size_t length);

#endif
