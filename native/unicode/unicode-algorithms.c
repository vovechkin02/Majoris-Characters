#include "unicode-algorithms.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <unicode/utf8.h>

#include "../scalar-validity.h"
#include "unicode-engine.h"

/** Preserve the existing composer input byte limit. */
#define MAX_GRAPHEME_BYTES (16384u * 4u)
/** Bound folding input to the existing text normalization contract. */
#define MAX_FOLD_BYTES (1024u * 1024u)

/** Grapheme_Cluster_Break values in snapshot generator order. */
typedef enum {
    /** Default break class. */
    GCB_OTHER,
    /** Carriage return. */
    GCB_CR,
    /** Line feed. */
    GCB_LF,
    /** Other control. */
    GCB_CONTROL,
    /** Extending mark. */
    GCB_EXTEND,
    /** Zero width joiner. */
    GCB_ZWJ,
    /** Regional indicator. */
    GCB_RI,
    /** Prepending character. */
    GCB_PREPEND,
    /** Spacing mark. */
    GCB_SPACING_MARK,
    /** Hangul leading Jamo. */
    GCB_L,
    /** Hangul vowel Jamo. */
    GCB_V,
    /** Hangul trailing Jamo. */
    GCB_T,
    /** Hangul LV syllable. */
    GCB_LV,
    /** Hangul LVT syllable. */
    GCB_LVT
} GraphemeClass;

/** Indic_Conjunct_Break values in snapshot generator order. */
typedef enum {
    /** No conjunct role. */
    INCB_NONE,
    /** Extending element in a conjunct. */
    INCB_EXTEND,
    /** Consonant starting or continuing a conjunct. */
    INCB_CONSONANT,
    /** Linker joining consonants. */
    INCB_LINKER
} ConjunctClass;

/** A maximal range with identical packed grapheme properties. */
struct GraphemeRange {
    /** Inclusive lower scalar bound. */
    gunichar first;
    /** Inclusive upper scalar bound. */
    gunichar last;
    /** Packed break class, conjunct class and pictographic bit. */
    guint8 flags;
};

/** A scalar's full mapping into immutable shared storage. */
struct FoldMapping {
    /** Mapping source scalar. */
    gunichar point;
    /** Offset in the immutable scalar pool. */
    guint32 offset;
    /** Mapping length, zero for deletion. */
    guint8 count;
};

/** A scalar's three simple case mappings. */
struct SimpleCase {
    /** Mapping source scalar. */
    gunichar point;
    /** Simple uppercase target. */
    gunichar upper;
    /** Simple lowercase target. */
    gunichar lower;
    /** Simple titlecase target. */
    gunichar title;
};

#include "generated/algorithms.h"

/** State sufficient for UAX #29 GB3–GB13 without rescanning earlier text. */
struct GraphemeState {
    /** Immediately preceding break class. */
    GraphemeClass previous;
    /** Whether a scalar has been consumed. */
    gboolean started;
    /** Left suffix matches Extended_Pictographic Extend*. */
    gboolean pictographic_tail;
    /** Left suffix ends in an eligible pictographic ZWJ. */
    gboolean pictographic_zwj;
    /** Left suffix starts with an Indic consonant. */
    gboolean consonant_tail;
    /** Conjunct suffix has consumed at least one linker. */
    gboolean linker_seen;
    /** Odd length of the uninterrupted regional-indicator run. */
    gboolean odd_ri;
    /** Number of clusters already started. */
    gint count;
};

/** Resolve packed GCB, InCB and Extended_Pictographic data. */
static guint8 grapheme_flags(gunichar point)
{
    gsize low = 0;
    gsize high = G_N_ELEMENTS(grapheme_ranges);
    while (low < high) {
        gsize middle = low + (high - low) / 2;
        const struct GraphemeRange *range = &grapheme_ranges[middle];
        if (point < range->first) {
            high = middle;
        } else if (point > range->last) {
            low = middle + 1;
        } else {
            return range->flags;
        }
    }
    return 0;
}

/** Identify the three classes that force boundaries under GB4 and GB5. */
static gboolean is_control(GraphemeClass value)
{
    return value == GCB_CR || value == GCB_LF || value == GCB_CONTROL;
}

/** Apply the ordered default extended grapheme boundary rules. */
static gboolean breaks_before(const struct GraphemeState *state, GraphemeClass current,
                              ConjunctClass conjunct, gboolean pictographic)
{
    GraphemeClass previous = state->previous;
    if (!state->started) {
        return TRUE;
    }
    if (previous == GCB_CR && current == GCB_LF) {
        return FALSE;
    }
    if (is_control(previous) || is_control(current)) {
        return TRUE;
    }
    if (previous == GCB_L && (current == GCB_L || current == GCB_V ||
        current == GCB_LV || current == GCB_LVT)) {
        return FALSE;
    }
    if ((previous == GCB_LV || previous == GCB_V) &&
        (current == GCB_V || current == GCB_T)) {
        return FALSE;
    }
    if ((previous == GCB_LVT || previous == GCB_T) && current == GCB_T) {
        return FALSE;
    }
    if (current == GCB_EXTEND || current == GCB_ZWJ || current == GCB_SPACING_MARK ||
        previous == GCB_PREPEND) {
        return FALSE;
    }
    if (conjunct == INCB_CONSONANT && state->consonant_tail && state->linker_seen) {
        return FALSE;
    }
    if (pictographic && state->pictographic_zwj) {
        return FALSE;
    }
    if (previous == GCB_RI && current == GCB_RI && state->odd_ri) {
        return FALSE;
    }
    return TRUE;
}

/** Advance the bounded left-context state after processing one valid scalar. */
static void consume(struct GraphemeState *state, gunichar point)
{
    guint8 flags = grapheme_flags(point);
    GraphemeClass current = (GraphemeClass)(flags & 15u);
    ConjunctClass conjunct = (ConjunctClass)((flags >> 4) & 3u);
    gboolean pictographic = (flags & 64u) != 0;
    if (breaks_before(state, current, conjunct, pictographic)) {
        state->count++;
    }
    state->pictographic_zwj = current == GCB_ZWJ && state->pictographic_tail;
    state->pictographic_tail = pictographic ||
        (current == GCB_EXTEND && state->pictographic_tail);
    if (conjunct == INCB_CONSONANT) {
        state->consonant_tail = TRUE;
        state->linker_seen = FALSE;
    } else if (conjunct == INCB_LINKER) {
        state->linker_seen = state->consonant_tail;
    } else if (conjunct != INCB_EXTEND) {
        state->consonant_tail = FALSE;
        state->linker_seen = FALSE;
    }
    state->odd_ri = current == GCB_RI && !state->odd_ri;
    state->previous = current;
    state->started = TRUE;
}

/**
 * characters_unicode_graphemes:
 * Count the default extended grapheme clusters in valid UTF-8.
 */
gint characters_unicode_graphemes(const gchar *text)
{
    if (!text || strnlen(text, MAX_GRAPHEME_BYTES + 1) > MAX_GRAPHEME_BYTES) {
        return -1;
    }
    struct GraphemeState state = { 0 };
    gint length = (gint)strlen(text);
    for (gint offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT((const guint8 *)text, offset, length, point);
        if (point < 0) {
            return -1;
        }
        consume(&state, (gunichar)point);
    }
    return state.count;
}

/**
 * characters_unicode_graphemes_points:
 * Count clusters without interpreting NUL as a string terminator.
 */
gint characters_unicode_graphemes_points(const gunichar *points, gsize count)
{
    if ((!points && count) || count > MAX_GRAPHEME_BYTES) {
        return -1;
    }
    struct GraphemeState state = { 0 };
    for (gsize index = 0; index < count; index++) {
        if (!characters_scalar_is_valid(points[index])) {
            return -1;
        }
        consume(&state, points[index]);
    }
    return state.count;
}

/** Compare a scalar against an immutable simple case mapping. */
static gint compare_case(gconstpointer key, gconstpointer value)
{
    gunichar point = *(const gunichar *)key;
    const struct SimpleCase *entry = value;
    return (point > entry->point) - (point < entry->point);
}

/**
 * characters_unicode_simple_case:
 * Resolve pinned Unicode 17 simple case mappings independently of system ICU.
 */
gunichar characters_unicode_simple_case(gunichar point, CharactersCaseMapping mapping)
{
    const struct SimpleCase *entry = bsearch(&point, simple_cases,
        G_N_ELEMENTS(simple_cases), sizeof(simple_cases[0]), compare_case);
    if (!entry) {
        return point;
    }
    switch (mapping) {
        case CHARACTERS_CASE_UPPER:
            return entry->upper;
        case CHARACTERS_CASE_LOWER:
            return entry->lower;
        case CHARACTERS_CASE_TITLE:
            return entry->title;
        default:
            return point;
    }
}

/** Compare a scalar against a sorted full folding table. */
static gint compare_fold(gconstpointer key, gconstpointer value)
{
    gunichar point = *(const gunichar *)key;
    const struct FoldMapping *entry = value;
    return (point > entry->point) - (point < entry->point);
}

/** Map every scalar, retaining exact identity where the selected table has no entry. */
static gchar *fold_text(const gchar *text, const struct FoldMapping *table, gsize count,
                        const gunichar *pool)
{
    if (!text || strnlen(text, MAX_FOLD_BYTES + 1) > MAX_FOLD_BYTES) {
        return NULL;
    }
    gint length = (gint)strlen(text);
    gsize capacity = (gsize)length + 1;
    gchar *output = g_try_malloc(capacity);
    if (!output) {
        return NULL;
    }
    gint written = 0;
    for (gint offset = 0; offset < length;) {
        UChar32 decoded;
        U8_NEXT((const guint8 *)text, offset, length, decoded);
        if (decoded < 0) {
            g_free(output);
            return NULL;
        }
        gunichar point = (gunichar)decoded;
        const struct FoldMapping *entry = bsearch(&point, table, count,
            sizeof(*entry), compare_fold);
        const gunichar *points = entry ? pool + entry->offset : &point;
        gsize size = entry ? entry->count : 1;
        if ((gsize)written + size * 4 + 1 > capacity) {
            gsize next = (gsize)written + size * 4 + 1 + capacity;
            gchar *replacement = g_try_realloc_n(output, next, 1);
            if (!replacement) {
                g_free(output);
                return NULL;
            }
            output = replacement;
            capacity = next;
        }
        for (gsize index = 0; index < size; index++) {
            U8_APPEND_UNSAFE((guint8 *)output, written, points[index]);
        }
    }
    output[written] = 0;
    return output;
}

/**
 * characters_unicode_casefold:
 * Apply the full default, non-Turkic case folding mapping.
 */
gchar *characters_unicode_casefold(const gchar *text)
{
    return fold_text(text, casefold_mappings, G_N_ELEMENTS(casefold_mappings), casefold_points);
}

/**
 * characters_unicode_nfkc_casefold:
 * Apply per-scalar NFKC_CF and then canonical composition as specified by UAX #15.
 */
gchar *characters_unicode_nfkc_casefold(const gchar *text)
{
    gchar *mapped = fold_text(text, nfkc_casefold_mappings,
        G_N_ELEMENTS(nfkc_casefold_mappings), nfkc_casefold_points);
    if (!mapped) {
        return NULL;
    }
    gchar *result = characters_unicode_normalize(mapped, "NFC");
    g_free(mapped);
    return result;
}
