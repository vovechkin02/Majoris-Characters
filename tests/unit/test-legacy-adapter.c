#include "legacy-encoding.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Resolve a stable fixture encoding without relying on catalog position. */
static size_t encoding_index(const char *name) {
    for (size_t index = 0; index < characters_legacy_count(); index++) {
        if (strcmp(characters_legacy_name(index), name) == 0) {
            return index;
        }
    }
    assert(!"Missing encoding");
    return SIZE_MAX;
}

/** Verify raw lengths, embedded zero, supplementary and contextual scalars. */
static void test_conversion(void) {
    size_t encoding = encoding_index("GB18030-2022");
    uint8_t input[] = { 0x41, 0, 0x94, 0x39, 0xFC, 0x36 };
    uint32_t expected[] = { 0x41, 0, 0x1F600 };
    size_t length = 0;
    uint32_t *points = characters_legacy_decode_raw(encoding, input, sizeof(input), &length);
    assert(points && length == 3 && memcmp(points, expected, sizeof(expected)) == 0);
    size_t count = 0;
    uint8_t *bytes = characters_legacy_encode_raw(encoding, points, length, &count);
    assert(bytes && count == sizeof(input) && memcmp(bytes, input, count) == 0);
    free(bytes);
    free(points);
    encoding = encoding_index("ISCII Devanagari");
    uint8_t contextual[] = { 0xE8, 0xE8 };
    points = characters_legacy_decode_raw(encoding, contextual, sizeof(contextual), &length);
    assert(points && length == 2 && points[0] == 0x094D && points[1] == 0x200C);
    bytes = characters_legacy_encode_raw(encoding, points, length, &count);
    assert(bytes && count > 0);
    size_t decoded_length = 0;
    uint32_t *decoded = characters_legacy_decode_raw(encoding, bytes, count, &decoded_length);
    assert(decoded && decoded_length == length);
    assert(memcmp(decoded, points, length * sizeof(*points)) == 0);
    free(decoded);
    free(bytes);
    free(points);
}

/** Failure clears lengths and rejects invalid pointers, limits, and scalars. */
static void test_limits(void) {
    size_t ascii = encoding_index("US-ASCII");
    size_t japanese = encoding_index("ISO-2022-JP");
    uint8_t bytes[129] = { 0x41 };
    uint32_t points[33] = { 0x41 };
    size_t length = 1;
    assert(!characters_legacy_decode_raw(ascii, bytes, 129, &length) && !length);
    assert(!characters_legacy_decode_raw(ascii, bytes, 0, &length) && !length);
    assert(!characters_legacy_decode_raw(ascii, NULL, 1, &length) && !length);
    assert(!characters_legacy_decode_raw(ascii, bytes, 1, NULL));
    assert(!characters_legacy_decode_raw(SIZE_MAX, bytes, 1, &length) && !length);
    assert(!characters_legacy_decode_raw(ascii, bytes, 33, &length) && !length);
    uint8_t truncated[] = { 0x1B, 0x24 };
    assert(!characters_legacy_decode_raw(japanese, truncated, 2, &length) && !length);
    assert(!characters_legacy_encode_raw(ascii, points, 33, &length) && !length);
    assert(!characters_legacy_encode_raw(ascii, points, 0, &length) && !length);
    assert(!characters_legacy_encode_raw(ascii, NULL, 1, &length) && !length);
    assert(!characters_legacy_encode_raw(ascii, points, 1, NULL));
    uint32_t lossy[] = { 'A', 0x200D };
    uint8_t *encoded = characters_legacy_encode_raw(ascii, lossy, 2, &length);
    assert(encoded && length == 1 && encoded[0] == 'A');
    free(encoded);
    points[0] = 0xD800;
    assert(!characters_legacy_encode_raw(ascii, points, 1, &length) && !length);
    points[0] = 0x20AC;
    assert(!characters_legacy_encode_raw(ascii, points, 1, &length) && !length);
}

/** Packed aliases preserve zero bytes, order and independent ownership. */
static void test_keys(void) {
    size_t ascii = encoding_index("US-ASCII");
    CharactersLegacy *table = characters_legacy_load(ascii);
    assert(table && characters_legacy_encoding_index(table) == ascii);
    assert(characters_legacy_contains(table, 0));
    assert(!characters_legacy_contains(table, 0x20AC));
    assert(!characters_legacy_contains(table, 0xD800));
    size_t length = 0;
    uint64_t *keys = characters_legacy_keys(table, 0, &length);
    assert(keys && length == 1 && keys[0] == UINT64_C(0x100000000));
    keys[0] = 0;
    free(keys);
    keys = characters_legacy_keys(table, 0, &length);
    assert(keys && length == 1 && keys[0] == UINT64_C(0x100000000));
    free(keys);
    assert(!characters_legacy_keys(table, 0x20AC, &length) && !length);
    assert(!characters_legacy_keys(table, UINT32_MAX, &length) && !length);
    assert(!characters_legacy_keys(table, 0, NULL));
    characters_legacy_free(table);
    table = characters_legacy_load(encoding_index("Shift JIS (Windows-932)"));
    assert(table);
    keys = characters_legacy_keys(table, 0x2160, &length);
    assert(keys && length == 2 && keys[0] == UINT64_C(0x200008754));
    assert(keys[1] == UINT64_C(0x20000FA4A));
    free(keys);
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
