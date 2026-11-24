#ifndef CHARACTERS_LEGACY_ENCODING_H
#define CHARACTERS_LEGACY_ENCODING_H

#include <stddef.h>
#include <stdint.h>

/** Immutable scalar-to-byte mapping table for one supported stateless code page. */
typedef struct CharactersLegacy CharactersLegacy;

/**
 * characters_legacy_count:
 *
 * Returns: Number of supported encoding choices.
 */
size_t characters_legacy_count(void);

/**
 * characters_legacy_name:
 * @index: Encoding choice index.
 *
 * Returns: (transfer none) (nullable): Stable display name or NULL for invalid index.
 */
const char *characters_legacy_name(size_t index);

/**
 * characters_legacy_load:
 * @index: Encoding choice index.
 *
 * Returns: (transfer full) (nullable): Complete strict ICU mapping table or NULL on failure.
 * Construct on a worker. Invalid input is never replaced; ICU decoder aliases are retained.
 */
CharactersLegacy *characters_legacy_load(size_t index);

/**
 * characters_legacy_free:
 * @table: (transfer full) (nullable): Immutable table no longer used by any consumer.
 */
void characters_legacy_free(CharactersLegacy *table);

/**
 * characters_legacy_points:
 * @table: Borrowed table.
 * @length: (out): Number of distinct scalars returned.
 *
 * Returns: (transfer full) (array length=length) (nullable): Sorted unique Unicode scalars.
 * Free with free(); an empty mapping is distinct from allocation failure.
 */
uint32_t *characters_legacy_points(const CharactersLegacy *table, size_t *length);

/**
 * characters_legacy_decode:
 * @table: Borrowed table.
 * @hex: Exactly two or four hexadecimal digits, optionally separated by one ASCII space.
 *
 * Returns: Decoded Unicode scalar, including NUL; UINT32_MAX for invalid or unmapped input.
 */
uint32_t characters_legacy_decode(const CharactersLegacy *table, const char *hex);

/**
 * characters_legacy_bytes:
 * @table: Borrowed table.
 * @point: Unicode scalar.
 *
 * Returns: (transfer full) (nullable): All valid byte sequences (hexadecimal, comma separated),
 * or NULL if unmapped or allocation fails. The caller frees the string.
 */
char *characters_legacy_bytes(const CharactersLegacy *table, uint32_t point);

#endif
