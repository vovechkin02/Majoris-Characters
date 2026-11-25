#ifndef CHARACTERS_UNIHAN_H
#define CHARACTERS_UNIHAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Return an owned formal or explicitly labeled fallback property; NULL if absent. */
char *characters_unihan_property(uint32_t point, const char *property);

/** Return the official data release, or "none" when only fallback data is bundled. */
const char *characters_unihan_version(void);

/** Fold one validated UTF-8 term into bounded search storage; false on overflow/error. */
bool characters_unihan_fold(const char *term, char *output, size_t capacity);

/** Whether a term names an explicit Han-only field (even when its value is invalid). */
bool characters_unihan_is_field_query(const char *term);

/** Match an already folded query term against Han metadata or a numeric field constraint. */
bool characters_unihan_matches(uint32_t point, const char *term);

/** Number of available radical or stroke filter slots. */
size_t characters_unihan_group_count(bool radical);

/** Borrow a stable group label, or NULL outside the available range. */
const char *characters_unihan_group_name(bool radical, size_t index);

/** Match a group index; -1 matches any known group in the selected classification. */
bool characters_unihan_group_matches(uint32_t point, bool radical, int index);

#endif
