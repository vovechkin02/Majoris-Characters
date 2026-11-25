#ifndef CHARACTERS_LANGUAGE_COLLECTIONS_H
#define CHARACTERS_LANGUAGE_COLLECTIONS_H

#include <stdbool.h>
#include <stddef.h>

/** The property snapshot uses uint8_t script IDs. */
#define CHARACTERS_LANGUAGE_SCRIPT_LIMIT 256

/** Number of language collections, including locale and configured keyboard sources. */
size_t characters_language_count(void);

/** Borrow the English label at an index, or NULL for an invalid slot. */
const char *characters_language_name(size_t index);

/** Resolve a collection into a zero-initialized snapshot script mask.
 * Sources are bounded comma-separated XKB layout or layout+variant IDs.
 */
void characters_language_scripts(int filter, const char *locale, const char *sources,
                                 bool scripts[CHARACTERS_LANGUAGE_SCRIPT_LIMIT]);

#endif
