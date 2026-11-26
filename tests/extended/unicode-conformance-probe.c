#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "icu-scalar-conversion.h"
#include "unicode-algorithms.h"
#include "unicode-engine.h"
#include "unihan.h"
#include "normalization.h"

/** Limit fixture sequences to the longest expected conformance test row. */
#define MAX_POINTS 256

/** Parse a bounded space-separated hexadecimal scalar fixture. */
static gboolean parse_points(gchar *line, gunichar *points, gsize *count)
{
    *count = 0;
    gchar *save = NULL;
    for (gchar *token = strtok_r(line, " \r\n", &save); token;
         token = strtok_r(NULL, " \r\n", &save)) {
        gchar *end;
        gulong value = strtoul(token, &end, 16);
        if (*end || value > 0x10FFFF || *count == MAX_POINTS ||
            !characters_scalar_is_valid((gunichar)value)) {
            return FALSE;
        }
        points[(*count)++] = (gunichar)value;
    }
    return TRUE;
}

/** Print a scalar sequence in the fixture protocol, including empty results. */
static void print_points(const gunichar *points, gsize count)
{
    for (gsize index = 0; index < count; index++) {
        printf("%s%X", index ? " " : "", points[index]);
    }
    putchar('\n');
}

/** Normalize explicit scalar input without the inspector's assignment restriction. */
static gboolean normalize_points(const gunichar *points, gsize count, const gchar *form)
{
    UChar units[MAX_POINTS * 2];
    gint length = characters_icu_encode_utf16(points, count, units, G_N_ELEMENTS(units));
    if (!length && count) {
        return FALSE;
    }
    gint normalized_length = 0;
    UChar *normalized = characters_normalize_utf16(units, length, form, &normalized_length);
    if (!normalized) {
        return FALSE;
    }
    gunichar *output = g_try_new(gunichar, (gsize)normalized_length + 1);
    if (!output) {
        g_free(normalized);
        return FALSE;
    }
    gsize actual = characters_icu_decode_utf16(normalized, normalized_length, output,
        (gsize)normalized_length + 1);
    print_points(output, actual);
    g_free(output);
    g_free(normalized);
    return TRUE;
}

/** Fold one non-NUL fixture and print its exact output bytes as scalars. */
static gboolean fold_points(const gunichar *points, gsize count, gchar mode)
{
    gchar input[MAX_POINTS * 4 + 1];
    gint length = 0;
    for (gsize index = 0; index < count; index++) {
        if (!points[index]) {
            return FALSE;
        }
        U8_APPEND_UNSAFE((guint8 *)input, length, points[index]);
    }
    input[length] = 0;
    gchar *output = mode == 'C' ? characters_unicode_casefold(input) :
        mode == 'K' ? characters_unicode_nfkc_casefold(input) :
        characters_unihan_fold_text(input);
    if (!output) {
        return FALSE;
    }
    gsize bytes = strlen(output);
    gunichar *decoded = g_try_new(gunichar, bytes + 1);
    if (!decoded) {
        g_free(output);
        return FALSE;
    }
    gsize actual = characters_icu_decode_utf8(output, decoded, bytes + 1);
    print_points(decoded, actual);
    g_free(decoded);
    g_free(output);
    return TRUE;
}

/** Execute one bounded command; grapheme output lists each prefix's cluster count. */
static gboolean process_line(gchar *line)
{
    if (!line[0] || line[1] != ' ') {
        return FALSE;
    }
    gunichar points[MAX_POINTS];
    gsize count;
    if (!parse_points(line + 2, points, &count)) {
        return FALSE;
    }
    switch (line[0]) {
        case 'G':
            for (gsize index = 1; index <= count; index++) {
                printf("%s%d", index == 1 ? "" : " ",
                       characters_unicode_graphemes_points(points, index));
            }
            putchar('\n');
            return TRUE;
        case 'C':
        case 'K':
        case 'H':
            return fold_points(points, count, line[0]);
        case 'N':
            return normalize_points(points, count, "NFC");
        case 'D':
            return normalize_points(points, count, "NFD");
        case 'P':
            return normalize_points(points, count, "NFKC");
        case 'Q':
            return normalize_points(points, count, "NFKD");
        default:
            return FALSE;
    }
}

/** Read the optional conformance driver's plain text protocol from standard input. */
int main(void)
{
    gchar line[MAX_POINTS * 8 + 4];
    while (fgets(line, sizeof(line), stdin)) {
        if (!strchr(line, '\n') || !process_line(line)) {
            puts("ERROR");
        }
    }
    return ferror(stdin) ? 1 : 0;
}
