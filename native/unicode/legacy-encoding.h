#ifndef CHARACTERS_LEGACY_ENCODING_H
#define CHARACTERS_LEGACY_ENCODING_H

#include <glib.h>

/** Maximum bytes accepted or emitted by the bounded sequence converter. */
#define CHARACTERS_LEGACY_MAX_BYTES 128

/** Immutable scalar repertoire and independent byte alternatives for one code page. */
typedef struct CharactersLegacy CharactersLegacy;

/**
 * characters_legacy_count:
 *
 * Returns: Number of supported encoding choices.
 */
gsize characters_legacy_count(void);

/**
 * characters_legacy_name:
 * @index: Encoding choice index.
 *
 * Returns: (transfer none) (nullable): Stable display name or NULL for invalid index.
 */
const gchar *characters_legacy_name(gsize index);

/**
 * characters_legacy_load:
 * @index: Encoding choice index.
 *
 * Returns: (transfer full) (nullable): Strict ICU repertoire table or NULL on failure.
 * Construct on a worker. Invalid input is never replaced; Independent byte aliases are retained; contextual encodings expose canonical bytes.
 */
CharactersLegacy *characters_legacy_load(gsize index);

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
 * Free with g_free(); an empty mapping is distinct from allocation failure.
 */
gunichar *characters_legacy_points(const CharactersLegacy *table, gsize *length);

/**
 * characters_legacy_encoding_index:
 * @table: Borrowed immutable table.
 *
 * Returns: Stable converter index or G_MAXSIZE for NULL.
 */
gsize characters_legacy_encoding_index(const CharactersLegacy *table);

/**
 * characters_legacy_contains:
 * @table: Borrowed immutable table.
 * @point: Unicode scalar, including NUL.
 *
 * Returns: True if the scalar is present in the repertoire.
 */
gboolean characters_legacy_contains(const CharactersLegacy *table, gunichar point);

/**
 * characters_legacy_keys:
 * @table: Borrowed immutable table.
 * @point: Unicode scalar.
 * @length: (out): Number of exact independent byte alternatives.
 *
 * Returns: (transfer full) (array length=length) (nullable): Sorted packed keys,
 * with byte length in the high 32 bits and big-endian bytes in the low 32 bits.
 * NULL for unmapped/contextual scalars or allocation failure. Free with g_free().
 */
guint64 *characters_legacy_keys(const CharactersLegacy *table, gunichar point, gsize *length);

/**
 * characters_legacy_is_contextual:
 * @index: Encoding choice index.
 *
 * Returns: True for encodings whose repertoire uses standalone round-trip mappings.
 */
gboolean characters_legacy_is_contextual(gsize index);

/**
 * characters_legacy_decode_raw:
 * @encoding: Encoding choice index.
 * @bytes: (array length=count): Exact byte stream, including zero bytes.
 * @count: Number of bytes, 1–128.
 * @length: (out): Number of decoded scalars, zero on failure.
 *
 * Returns: (transfer full) (array length=length) (nullable): Exact 1–32 scalars
 * from the converter's initial state; NULL for invalid, empty or oversized output.
 * No replacement or normalization is performed. Free with g_free().
 */
gunichar *characters_legacy_decode_raw(gsize encoding, const guint8 *bytes, gsize count,
                                       gsize *length);

/**
 * characters_legacy_encode_raw:
 * @encoding: Encoding choice index.
 * @points: (array length=length): Exact Unicode scalars, including NUL.
 * @length: Number of scalars, 1–32.
 * @count: (out): Number of encoded bytes, zero on failure.
 *
 * Returns: (transfer full) (array length=count) (nullable): Strict ICU output
 * of at most 128 bytes, including state resets. The caller must check exact
 * round-trip equality before publishing conversion results. Free with g_free().
 */
guint8 *characters_legacy_encode_raw(gsize encoding, const gunichar *points, gsize length,
                                     gsize *count);

#endif
