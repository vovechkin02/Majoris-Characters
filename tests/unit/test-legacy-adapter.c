#include "legacy-encoding.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#include <glib.h>

/** Resolve a stable fixture encoding without relying on catalog position. */
static gsize encoding_index(const gchar *name) {
    for (gsize index = 0; index < characters_legacy_count(); index++) {
        if (strcmp(characters_legacy_name(index), name) == 0) {
            return index;
        }
    }
    assert(!"Missing encoding");
    return G_MAXSIZE;
}

/** Verify raw lengths, embedded zero, supplementary and contextual scalars. */
static void test_conversion(void) {
    gsize encoding = encoding_index("GB18030-2022");
    guint8 input[] = { 0x41, 0, 0x94, 0x39, 0xFC, 0x36 };
    guint32 expected[] = { 0x41, 0, 0x1F600 };
    gsize length = 0;
    gunichar *points = characters_legacy_decode_raw(encoding, input, sizeof(input), &length);
    assert(points && length == 3 && memcmp(points, expected, sizeof(expected)) == 0);
    gsize count = 0;
    guint8 *bytes = characters_legacy_encode_raw(encoding, points, length, &count);
    assert(bytes && count == sizeof(input) && memcmp(bytes, input, count) == 0);
    g_free(bytes);
    g_free(points);
    encoding = encoding_index("ISCII Devanagari");
    guint8 contextual[] = { 0xE8, 0xE8 };
    points = characters_legacy_decode_raw(encoding, contextual, sizeof(contextual), &length);
    assert(points && length == 2 && points[0] == 0x094D && points[1] == 0x200C);
    bytes = characters_legacy_encode_raw(encoding, points, length, &count);
    assert(bytes && count > 0);
    gsize decoded_length = 0;
    guint32 *decoded = characters_legacy_decode_raw(encoding, bytes, count, &decoded_length);
    assert(decoded && decoded_length == length);
    assert(memcmp(decoded, points, length * sizeof(*points)) == 0);
    g_free(decoded);
    g_free(bytes);
    g_free(points);
}

/** Failure clears lengths and rejects invalid pointers, limits, and scalars. */
static void test_limits(void) {
    gsize ascii = encoding_index("US-ASCII");
    gsize japanese = encoding_index("ISO-2022-JP");
    guint8 bytes[129] = { 0x41 };
    gunichar points[33] = { 0x41 };
    gsize length = 1;
    assert(!characters_legacy_decode_raw(ascii, bytes, 129, &length) && !length);
    assert(!characters_legacy_decode_raw(ascii, bytes, 0, &length) && !length);
    assert(!characters_legacy_decode_raw(ascii, NULL, 1, &length) && !length);
    assert(!characters_legacy_decode_raw(ascii, bytes, 1, NULL));
    assert(!characters_legacy_decode_raw(G_MAXSIZE, bytes, 1, &length) && !length);
    assert(!characters_legacy_decode_raw(ascii, bytes, 33, &length) && !length);
    guint8 truncated[] = { 0x1B, 0x24 };
    assert(!characters_legacy_decode_raw(japanese, truncated, 2, &length) && !length);
    assert(!characters_legacy_encode_raw(ascii, points, 33, &length) && !length);
    assert(!characters_legacy_encode_raw(ascii, points, 0, &length) && !length);
    assert(!characters_legacy_encode_raw(ascii, NULL, 1, &length) && !length);
    assert(!characters_legacy_encode_raw(ascii, points, 1, NULL));
    guint32 lossy[] = { 'A', 0x200D };
    guint8 *encoded = characters_legacy_encode_raw(ascii, lossy, 2, &length);
    assert(encoded && length == 1 && encoded[0] == 'A');
    g_free(encoded);
    points[0] = 0xD800;
    assert(!characters_legacy_encode_raw(ascii, points, 1, &length) && !length);
    points[0] = 0x20AC;
    assert(!characters_legacy_encode_raw(ascii, points, 1, &length) && !length);
}

/** Packed aliases preserve zero bytes, order and independent ownership. */
static void test_keys(void) {
    gsize ascii = encoding_index("US-ASCII");
    CharactersLegacy *table = characters_legacy_load(ascii);
    assert(table && characters_legacy_encoding_index(table) == ascii);
    assert(characters_legacy_contains(table, 0));
    assert(!characters_legacy_contains(table, 0x20AC));
    assert(!characters_legacy_contains(table, 0xD800));
    gsize length = 0;
    guint64 *keys = characters_legacy_keys(table, 0, &length);
    assert(keys && length == 1 && keys[0] == UINT64_C(0x100000000));
    keys[0] = 0;
    g_free(keys);
    keys = characters_legacy_keys(table, 0, &length);
    assert(keys && length == 1 && keys[0] == UINT64_C(0x100000000));
    g_free(keys);
    assert(!characters_legacy_keys(table, 0x20AC, &length) && !length);
    assert(!characters_legacy_keys(table, G_MAXUINT32, &length) && !length);
    assert(!characters_legacy_keys(table, 0, NULL));
    characters_legacy_free(table);
    table = characters_legacy_load(encoding_index("Shift JIS (Windows-932)"));
    assert(table);
    keys = characters_legacy_keys(table, 0x2160, &length);
    assert(keys && length == 2 && keys[0] == UINT64_C(0x200008754));
    assert(keys[1] == UINT64_C(0x20000FA4A));
    g_free(keys);
    characters_legacy_free(table);
    table = characters_legacy_load(encoding_index("ISO-2022-JP"));
    assert(table && characters_legacy_contains(table, 0x3042));
    assert(!characters_legacy_keys(table, 0x3042, &length) && !length);
    characters_legacy_free(table);
}

/** Run the strict native converter and mapping boundary contracts. */
int main(void) {
    test_conversion();
    test_limits();
    test_keys();
    puts("3 native legacy adapter contract groups passed");
    return 0;
}
