#ifndef CHARACTERS_UNIHAN_H
#define CHARACTERS_UNIHAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Return an owned formal or explicitly labeled fallback property; NULL if absent. */
char *characters_unihan_property(uint32_t point, const char *property);

/** Return the official data release, or "none" when only fallback data is bundled. */
const char *characters_unihan_version(void);

/** Prepared Han predicate kinds shared with the Vala query policy. */
typedef enum {
    /** Ordinary metadata term; other scalar metadata may also satisfy it. */
    CHARACTERS_HAN_ORDINARY,
    /** Explicit Han reading or definition field mask. */
    CHARACTERS_HAN_FIELD,
    /** Exact numeric radical group. */
    CHARACTERS_HAN_RADICAL,
    /** Exact numeric stroke group. */
    CHARACTERS_HAN_STROKES,
    /** Invalid explicit field value; never matches. */
    CHARACTERS_HAN_REJECT
} CharactersHanKind;

/** Bit positions of textual fields in the immutable HanRecord field order. */
#define CHARACTERS_HAN_TEXT_FIELDS UINT32_C(0xCFF)

/** Return owned ICU-folded reading text or NULL on invalid input or overflow. */
char *characters_unihan_fold_text(const char *term);

/** Match a nonempty folded term against the selected immutable textual fields. */
bool characters_unihan_match_fields(uint32_t point, uint32_t fields, const char *term);

/** Match a positive numeric group in official data or labeled fallback. */
bool characters_unihan_match_group(uint32_t point, bool radical, unsigned group);

/** Number of available radical or stroke filter slots. */
size_t characters_unihan_group_count(bool radical);

/** Borrow a stable group label, or NULL outside the available range. */
const char *characters_unihan_group_name(bool radical, size_t index);

/** Match a group index; -1 matches any known group in the selected classification. */
bool characters_unihan_group_matches(uint32_t point, bool radical, int index);

#endif
