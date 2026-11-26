#ifndef CHARACTERS_EMOJI_CATALOG_H
#define CHARACTERS_EMOJI_CATALOG_H

#include <glib.h>

/**
 * characters_emoji_keyword_matches:
 * @index: Fully qualified catalog entry.
 * @locale: Locale identifier; English is always searched as a fallback.
 * @term: One UTF-8 metadata search term, at most 512 bytes.
 *
 * Returns: Whether localized or English annotations contain the folded term.
 */
gboolean characters_emoji_keyword_matches(gsize index, const gchar *locale, const gchar *term);

/** Return the immutable provenance string for the bundled annotation data. */
const gchar *characters_emoji_annotation_source(void);

/** Return whether text identifies a catalog emoji or supported presentation variant. */
gboolean characters_emoji_is_candidate(const gchar *text);

/** Return whether a scalar has standardized text and emoji presentation pairs. */
gboolean characters_emoji_is_presentation_base(gunichar point);

/** Find an exact noncatalog sequence index, or G_MAXSIZE if absent. */
gsize characters_emoji_alias_index(const gchar *text);

/** Return the number of exact supplementary sequence identities. */
gsize characters_emoji_alias_count(void);

/** Borrow the exact UTF-8 alias text for a valid index. */
const gchar *characters_emoji_alias_text(gsize index);

/** Return the corresponding catalog index, or G_MAXSIZE for standalone pairs. */
gsize characters_emoji_alias_canonical(gsize index);

/** Borrow a label for an exact presentation or selector-omission sequence. */
const gchar *characters_emoji_alias_label(gsize index);

/** Allocate an informative name for an exact alias; free the returned string. */
gchar *characters_emoji_alias_name(gsize index);

#endif
