#ifndef CHARACTERS_CHARACTER_INSPECTOR_H
#define CHARACTERS_CHARACTER_INSPECTOR_H

#include <stddef.h>
#include <stdint.h>

/** Immutable sequence snapshot, independent of the search result lifetime. */
typedef struct CharactersInspector CharactersInspector;

/** Numeric representations of a scalar sequence; host languages may reinterpret some values. */
typedef enum {
    /** Unicode code point notation. */
    CHARACTERS_INSPECTOR_CODE,
    /** Decimal scalar values. */
    CHARACTERS_INSPECTOR_DECIMAL,
    /** UTF-8 bytes in hexadecimal. */
    CHARACTERS_INSPECTOR_UTF8,
    /** UTF-16 code units in hexadecimal. */
    CHARACTERS_INSPECTOR_UTF16,
    /** UTF-32 code units in hexadecimal. */
    CHARACTERS_INSPECTOR_UTF32,
    /** Decimal numeric character references. */
    CHARACTERS_INSPECTOR_HTML_DECIMAL,
    /** Hexadecimal numeric character references. */
    CHARACTERS_INSPECTOR_HTML_HEX,
    /** Fixed-width Unicode escapes (C and Python style). */
    CHARACTERS_INSPECTOR_ESCAPE,
    /** CSS hexadecimal escapes with terminating spaces. */
    CHARACTERS_INSPECTOR_CSS,
} CharactersInspectorFormat;

/**
 * characters_inspector_from_id:
 * @id: Opaque browsing identity.
 *
 * Returns: (transfer full) (nullable): Snapshot or NULL for an invalid identity or allocation failure.
 */
CharactersInspector *characters_inspector_from_id(uint32_t id);

/**
 * characters_inspector_from_text:
 * @text: Exact NUL-terminated UTF-8, containing 1 to 32 scalars.
 *
 * Returns: (transfer full) (nullable): Snapshot or NULL for invalid, empty or
 * oversized input or allocation failure.
 */
CharactersInspector *characters_inspector_from_text(const char *text);

/**
 * characters_inspector_from_code:
 * @code: Explicit U+, 0x, or dec: notation for 1 to 32 scalars.
 *
 * Returns: (transfer full) (nullable): Snapshot or NULL for invalid input or allocation failure.
 */
CharactersInspector *characters_inspector_from_code(const char *code);

/**
 * characters_inspector_free:
 * @snapshot: (nullable) (transfer full): Snapshot to release.
 */
void characters_inspector_free(CharactersInspector *snapshot);

/**
 * characters_inspector_count:
 * @snapshot: Borrowed immutable snapshot.
 *
 * Returns: Number of constituent scalars, or zero for NULL.
 */
size_t characters_inspector_count(const CharactersInspector *snapshot);

/**
 * characters_inspector_point:
 * @snapshot: Borrowed immutable snapshot.
 * @index: Zero-based scalar position.
 *
 * Returns: Scalar or UINT32_MAX for an invalid position.
 */
uint32_t characters_inspector_point(const CharactersInspector *snapshot, size_t index);

/**
 * characters_inspector_name:
 * @snapshot: Borrowed immutable snapshot.
 *
 * Returns: (transfer full) (nullable): Canonical scalar or catalog sequence name,
 * or NULL when unavailable.
 */
char *characters_inspector_name(const CharactersInspector *snapshot);

/**
 * characters_inspector_text:
 * @snapshot: Borrowed immutable snapshot.
 *
 * Returns: (transfer full) (nullable): Exact UTF-8 text, or NULL for embedded NUL or allocation failure.
 */
char *characters_inspector_text(const CharactersInspector *snapshot);

/**
 * characters_inspector_display:
 * @snapshot: Borrowed immutable snapshot.
 *
 * Returns: (transfer full) (nullable): Safe preview, never the source for copying, or NULL on failure.
 */
char *characters_inspector_display(const CharactersInspector *snapshot);

/**
 * characters_inspector_format:
 * @snapshot: Borrowed immutable snapshot.
 * @format: Requested textual encoding.
 *
 * Returns: (transfer full) (nullable): Complete representation or NULL for an invalid format or failure.
 */
char *characters_inspector_format(const CharactersInspector *snapshot,
                                  CharactersInspectorFormat format);

/**
 * characters_inspector_normalize:
 * @snapshot: Borrowed immutable snapshot.
 * @form: NFC, NFD, NFKC or NFKD.
 *
 * Returns: (transfer full) (nullable): Normalized scalar notation, including NUL,
 * or NULL when properties are unavailable or normalization fails.
 */
char *characters_inspector_normalize(const CharactersInspector *snapshot, const char *form);

/**
 * characters_inspector_related:
 * @snapshot: Borrowed immutable snapshot.
 *
 * Returns: (transfer full) (array zero-terminated=1) (nullable): Owned alternatives,
 * or NULL when unavailable. Free each string and the vector.
 */
char **characters_inspector_related(const CharactersInspector *snapshot);

#endif
