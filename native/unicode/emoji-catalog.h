#ifndef CHARACTERS_EMOJI_CATALOG_H
#define CHARACTERS_EMOJI_CATALOG_H

#include <stdbool.h>
#include <stddef.h>

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

#endif
