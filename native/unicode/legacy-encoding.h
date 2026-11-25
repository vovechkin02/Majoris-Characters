#ifndef CHARACTERS_LEGACY_ENCODING_H
#define CHARACTERS_LEGACY_ENCODING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Maximum bytes accepted or emitted by the bounded sequence converter. */
#define CHARACTERS_LEGACY_MAX_BYTES 128

/** Immutable scalar repertoire and independent byte alternatives for one code page. */
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
 * Returns: (transfer full) (nullable): Strict ICU repertoire table or NULL on failure.
 * Construct on a worker. Invalid input is never replaced; Independent byte aliases are retained; contextual encodings expose canonical bytes.
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
 * @hex: Hexadecimal byte pairs, optionally separated by one ASCII space.
 *
 * Returns: Decoded Unicode scalar, including NUL; UINT32_MAX for invalid or unmapped input.
 */
uint32_t characters_legacy_decode(const CharactersLegacy *table, const char *hex);

/**
 * characters_legacy_bytes:
 * @table: Borrowed table.
 * @point: Unicode scalar.
 *
 * Returns: (transfer full) (nullable): Independent byte alternatives or canonical contextual bytes (hexadecimal),
 * or NULL if unmapped or allocation fails. The caller frees the string.
 */
char *characters_legacy_bytes(const CharactersLegacy *table, uint32_t point);

/**
 * characters_legacy_is_contextual:
 * @index: Encoding choice index.
 *
 * Returns: True for encodings whose repertoire uses standalone round-trip mappings.
 */
bool characters_legacy_is_contextual(size_t index);

/**
 * characters_legacy_decode_sequence:
 * @encoding: Encoding choice index.
 * @hex: 1–128 bytes in hexadecimal, starting in the converter's initial state.
 * @length: (out): Number of decoded scalars, zero on failure.
 *
 * Returns: (transfer full) (array length=length) (nullable): Exact 1–32 scalars, including
 * NUL and contextual multi-scalar mappings; NULL for invalid, empty or oversized output.
 * Free with free(). No replacement or normalization is performed.
 */
uint32_t *characters_legacy_decode_sequence(size_t encoding, const char *hex, size_t *length);

/**
 * characters_legacy_encode_sequence:
 * @encoding: Encoding choice index.
 * @points: (array length=length): Exact Unicode scalars, including NUL.
 * @length: Number of scalars, 1–32.
 *
 * Returns: (transfer full) (nullable): Canonical hexadecimal bytes (at most 128), including
 * any state shifts or resets. NULL unless strict decoding reproduces every input scalar.
 * Free with free(). The canonical encoding need not reproduce an original byte alias.
 */
char *characters_legacy_encode_sequence(size_t encoding, const uint32_t *points, size_t length);

#endif
