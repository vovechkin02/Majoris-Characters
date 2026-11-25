#ifndef CHARACTERS_EMOJI_CATALOG_H
#define CHARACTERS_EMOJI_CATALOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Owned immutable list of exact emoji and supported presentation alternatives. */
typedef struct CharactersEmojiVariants CharactersEmojiVariants;

/**
 * characters_emoji_variants_new:
 * @text: Exact UTF-8 sequence, at most 32 scalars.
 *
 * Returns: (transfer full) (nullable): Alternatives including the selected sequence,
 * or NULL for non-emoji, malformed input or allocation failure. No invented ZWJ or tone sequences.
 */
CharactersEmojiVariants *characters_emoji_variants_new(const char *text);

/** Release a variant collection; NULL is accepted. */
void characters_emoji_variants_free(CharactersEmojiVariants *variants);

/** Return the number of alternatives, or zero for NULL. */
size_t characters_emoji_variants_count(const CharactersEmojiVariants *variants);

/** Return the borrowed exact UTF-8 value at index, or NULL outside the collection. */
const char *characters_emoji_variants_text(const CharactersEmojiVariants *variants, size_t index);

/** Return the borrowed English catalog or presentation label, or NULL outside the collection. */
const char *characters_emoji_variants_label(const CharactersEmojiVariants *variants, size_t index);

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
