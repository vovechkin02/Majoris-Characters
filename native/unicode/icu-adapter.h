#ifndef CHARACTERS_ICU_ADAPTER_H
#define CHARACTERS_ICU_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

/** Count ICU extended grapheme clusters in validated, bounded UTF-8. */
int characters_icu_graphemes(const char *text);

/** Write direct decomposition scalars to a caller-owned 32-element array; zero if absent. */
size_t characters_icu_decomposition(uint32_t point, uint32_t *points);

/**
 * Normalize a bounded scalar array, preserving embedded NUL.
 * Return an owned scalar array (free with free), or NULL; length is in scalars.
 */
uint32_t *characters_icu_normalize_points(const uint32_t *points, int count,
                                         const char *form, int *output_count);

#endif
