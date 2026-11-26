#ifndef CHARACTERS_EMOJI_CATALOG_H
#define CHARACTERS_EMOJI_CATALOG_H

#include <glib.h>

/** Resolve an exact locale tag to an immutable index, or G_MAXUINT32. */
guint32 characters_emoji_locale_index(const gchar *language);

/** Return the number of immutable annotation locale tables. */
gsize characters_emoji_locale_count(void);

/** Allocate an ICU NFKC-casefolded term, or NULL for invalid or empty input. */
gchar *characters_emoji_fold_term(const gchar *term);

/** Match a prepared folded term using caller-selected locale indexes and exact scalars. */
gboolean characters_emoji_keyword_matches(const gunichar *points, gsize count,
                                          const guint32 *locales, gsize locale_count,
                                          const gchar *term);

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


#endif
