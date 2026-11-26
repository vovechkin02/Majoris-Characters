#ifndef CHARACTERS_EMOJI_CATALOG_H
#define CHARACTERS_EMOJI_CATALOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * characters_emoji_keyword_matches:
 * @index: Fully qualified catalog entry.
 * @locale: Locale identifier; English is always searched as a fallback.
 * @term: One UTF-8 metadata search term, at most 512 bytes.
 *
 * Returns: Whether localized or English annotations contain the folded term.
 */
bool characters_emoji_keyword_matches(size_t index, const char *locale, const char *term);

/** Return the immutable provenance string for the bundled annotation data. */
const char *characters_emoji_annotation_source(void);

/** Return whether text identifies a catalog emoji or supported presentation variant. */
bool characters_emoji_is_candidate(const char *text);

/** Return whether a scalar has standardized text and emoji presentation pairs. */
bool characters_emoji_is_presentation_base(uint32_t point);

/** Find an exact noncatalog sequence index, or SIZE_MAX if absent. */
size_t characters_emoji_alias_index(const char *text);

/** Return the number of exact supplementary sequence identities. */
size_t characters_emoji_alias_count(void);

/** Borrow the exact UTF-8 alias text for a valid index. */
const char *characters_emoji_alias_text(size_t index);

/** Return the corresponding catalog index, or SIZE_MAX for standalone pairs. */
size_t characters_emoji_alias_canonical(size_t index);

/** Borrow a label for an exact presentation or selector-omission sequence. */
const char *characters_emoji_alias_label(size_t index);

/** Allocate an informative name for an exact alias; free the returned string. */
char *characters_emoji_alias_name(size_t index);

#endif
