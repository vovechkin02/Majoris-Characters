#ifndef CHARACTERS_COMPOSITION_H
#define CHARACTERS_COMPOSITION_H

#include "character-inspector.h"

/** Maximum scalar count for synchronous composition analysis and conversion. */
#define CHARACTERS_COMPOSITION_MAX_POINTS 16384

/**
 * characters_composition_graphemes:
 * @text: NUL-terminated UTF-8 text.
 *
 * Count ICU extended grapheme clusters using the root locale. Empty text has zero clusters.
 *
 * Returns: Cluster count, or -1 for invalid, oversized, or unavailable input.
 */
int characters_composition_graphemes(const char *text);

/**
 * characters_composition_format:
 * @text: Nonempty, NUL-terminated UTF-8 text.
 * @format: Inspector representation to generate.
 *
 * Format the entire sequence without changing its code points.
 *
 * Returns: (transfer full) (nullable): Allocated ASCII representation, freed with free().
 */
char *characters_composition_format(const char *text, CharactersInspectorFormat format);

/**
 * characters_composition_normalize:
 * @text: NUL-terminated UTF-8 text within the analysis limit.
 * @form: NFC, NFD, NFKC or NFKD.
 *
 * Normalize across the entire sequence. Reject unavailable ICU properties and output
 * exceeding the analysis limit; never truncate or silently normalize in another form.
 *
 * Returns: (transfer full) (nullable): Allocated UTF-8, freed with free(), or NULL on failure.
 */
char *characters_composition_normalize(const char *text, const char *form);

#endif
