#include "character-search.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <unicode/uchar.h>

#include "../allocation.h"
#include "emoji-catalog.h"
#include "icu-scalar-conversion.h"
#include "language-collections.h"
#include "scalar-properties.h"
#include "unicode-engine.h"
#include "unihan.h"

#define SCALAR_LIMIT 0x110000u
#define EMOJI_BASE SCALAR_LIMIT
#define EMOJI_ALIAS_BASE 0x180000u
#define NAMED_BASE 0x200000u
#define MAX_QUERY_BYTES 512
#define MAX_SEQUENCE_POINTS CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH
#define MAX_TERMS 64
#define CANCEL_INTERVAL 256u
#define RULE_WORDS 4u
#define MAX_RULE_WORDS (RULE_WORDS * 128u)
#define HAN_WORDS 3u
#define MAX_HAN_TERM_BYTES (MAX_QUERY_BYTES * 3 + 1)

#define CATEGORY_COUNT 8u
#define FALLBACK_COUNT 4u
#define METADATA_LABEL_BYTES 64u
#define SYMBOL_COUNT 27u
#define PLANE_COUNT 17u

/** The worker owns mutable result storage; only the cancellation flag is shared during run. */
struct CharactersSearch {
    /** Primary collection, immutable after creation. */
    CharactersSearchScope scope;
    /** Collection-specific filter slot, or -1. */
    gint filter;
    /** General-category group slot, or -1. */
    gint category;
    /** Prepared category group index for each ICU category; -1 means no group. */
    gint category_groups[32];
    /** Copied English search vocabulary for category groups. */
    gchar category_labels[CATEGORY_COUNT][METADATA_LABEL_BYTES];
    /** Copied English fallback scalar search vocabulary. */
    gchar fallback_labels[FALLBACK_COUNT][METADATA_LABEL_BYTES];
    /** Whether the Vala metadata policy has been validated and copied. */
    gboolean metadata_ready;
    /** Owned prepared query; NULL until the Vala domain parser accepts input. */
    struct Query *query;
    /** Locale captured before worker execution; defaults to English. */
    gchar locale[64];
    /** Script membership mask resolved once before scanning a language collection. */
    gboolean scripts[CHARACTERS_LANGUAGE_SCRIPT_LIMIT];
    /** Copied union of Vala semantic clauses, each containing four unsigned words. */
    guint32 rules[MAX_RULE_WORDS];
    /** Number of initialized rule words; zero until semantic configuration succeeds. */
    gsize rule_count;
    /** Optional copied scalar repertoire; NULL denotes ordinary catalog search. */
    gunichar *points;
    /** Number of input scalars in the explicit repertoire. */
    gsize point_count;
    /** Set by any thread; observed at bounded scan intervals. */
    atomic_bool canceled;
    /** Owned result identity vector. */
    guint32 *ids;
    /** Number of initialized result identities. */
    gsize count;
    /** Allocated vector capacity, in identities. */
    gsize capacity;
    /** Scalar and emoji identities already emitted, used to merge catalog matches. */
    guint8 *seen;
    /** Published only once, after scanning finishes. */
    CharactersSearchStatus status;
    /** Enforces single-run ownership of mutable result storage. */
    gboolean started;
};

/** Parsed input in one owned allocation; term pointers refer to its internal buffer. */
struct Query {
    /** Exact literal text, used for single-letter emoji keyword searches. */
    gchar text[MAX_QUERY_BYTES + 1];
    /** Packed, owned metadata term strings. */
    gchar term_text[MAX_QUERY_BYTES + 1];
    /** Borrowed pointers into term_text, valid for the prepared query lifetime. */
    gchar *terms[MAX_TERMS];
    /** Number of parsed AND terms. */
    gsize term_count;
    /** Independently folded Han search terms; empty if folding was unavailable. */
    gchar han_terms[MAX_TERMS][MAX_HAN_TERM_BYTES];
    /** Prepared Han term descriptors: kind, field mask, numeric group. */
    guint32 han_rules[MAX_TERMS][HAN_WORDS];
    /** An explicit Han field disables non-Han sequence matching. */
    gboolean han_field;
    /** Decoded exact scalar sequence, never normalized. */
    gunichar points[MAX_SEQUENCE_POINTS];
    /** Number of valid scalars in points. */
    gsize point_count;
    /** Whether metadata matching is disabled for an exact numeric or literal query. */
    gboolean exact;
    /** True for a single literal Unicode letter, usable as an emoji keyword. */
    gboolean literal_letter;
};

/** Fold one ASCII byte inline in the repertoire scan without process-locale dependence. */
static inline guchar lower_ascii(guchar byte)
{
    return byte >= 'A' && byte <= 'Z' ? byte + ('a' - 'A') : byte;
}

/** Match a term against a nullable English metadata field, ignoring ASCII case. */
static gboolean contains_term(const gchar *field, const gchar *term)
{
    if (!field) {
        return FALSE;
    }
    for (; *field; field++) {
        gsize index = 0;
        while (term[index] && field[index] &&
               lower_ascii((guchar)field[index]) ==
               lower_ascii((guchar)term[index])) {
            index++;
        }
        if (!term[index]) {
            return TRUE;
        }
    }
    return FALSE;
}

/** Match every query term, allowing different terms to occur in different metadata fields. */
static gboolean matches_terms(const struct Query *query, const gchar *name, const gchar *aliases,
                          const gchar *block, const gchar *script, const gchar *group)
{
    if (query->exact) {
        return FALSE;
    }
    for (gsize index = 0; index < query->term_count; index++) {
        const gchar *term = query->terms[index];
        if (!contains_term(name, term) && !contains_term(aliases, term) &&
            !contains_term(block, term) && !contains_term(script, term) &&
            !contains_term(group, term)) {
            return FALSE;
        }
    }
    return TRUE;
}

/** Compare a catalog sequence to exact numeric or literal input. */
static gboolean matches_points(const struct Query *query, const gunichar *points, gsize count)
{
    return count && query->point_count == count &&
           memcmp(query->points, points, count * sizeof(*points)) == 0;
}

/** Read the prepared category-group mapping for a scalar without allocating. */
static gint category_for(const CharactersSearch *search, gunichar point)
{
    gint type = characters_scalar_category(point);
    return type >= 0 && type < 32 ? search->category_groups[type] : -1;
}

/** Resolve English metadata for search, not a tile or localized presentation label. */
static gchar *scalar_search_name(const CharactersSearch *search, gunichar point)
{
    if (!characters_unicode_is_assigned(point)) {
        return characters_try_strdup(search->fallback_labels[
            u_hasBinaryProperty((UChar32)point, UCHAR_NONCHARACTER_CODE_POINT) ? 0 : 1]);
    }
    gchar *name = characters_unicode_name(point);
    if (!name) {
        name = characters_unicode_aliases(point);
    }
    if (!name) {
        name = characters_try_strdup(search->fallback_labels[
            characters_scalar_category(point) == U_PRIVATE_USE_CHAR ? 2 : 3]);
    }
    return name;
}

/** Evaluate the prepared union of property/range clauses without allocating. */
static gboolean rules_match(const CharactersSearch *search, gunichar point)
{
    gint type = characters_scalar_category(point);
    if (type < 0 || type >= 32) {
        return FALSE;
    }
    guint32 category = UINT32_C(1) << type;
    gint decomposition = -1;
    for (gsize index = 0; index < search->rule_count; index += RULE_WORDS) {
        const guint32 *rule = search->rules + index;
        if (!(rule[0] & category) || point < rule[2] || point > rule[3]) {
            continue;
        }
        if (!rule[1]) {
            return TRUE;
        }
        if (decomposition < 0) {
            decomposition = characters_scalar_decomposition_type(point);
        }
        if (decomposition >= 0 && decomposition < 32 &&
            (rule[1] & (UINT32_C(1) << decomposition))) {
            return TRUE;
        }
    }
    return FALSE;
}

/** Whether the scalar passes the collection, script extensions and category constraints. */
static gboolean scalar_matches(const CharactersSearch *search, gunichar point)
{
    if (search->category >= 0 && category_for(search, point) != search->category) {
        return FALSE;
    }
    if (search->scope == CHARACTERS_SEARCH_SYMBOLS) {
        return rules_match(search, point);
    }
    if (search->scope == CHARACTERS_SEARCH_SCRIPT && search->filter >= 0) {
        return characters_scalar_has_script(point, search->filter) &&
               characters_scalar_category(point) != U_UNASSIGNED;
    }
    if (search->scope == CHARACTERS_SEARCH_LANGUAGE && search->filter >= 0) {
        return characters_scalar_matches_scripts(point, search->scripts, G_N_ELEMENTS(search->scripts));
    }
    if (search->scope == CHARACTERS_SEARCH_RADICAL || search->scope == CHARACTERS_SEARCH_STROKES) {
        return characters_unihan_group_matches(point, search->scope == CHARACTERS_SEARCH_RADICAL,
                                               search->filter);
    }
    return TRUE;
}

/** Check cancellation with relaxed ordering; no other fields are shared during execution. */
static gboolean is_canceled(const CharactersSearch *search)
{
    return atomic_load_explicit(&search->canceled, memory_order_relaxed);
}

/** Test whether a valid scalar or emoji identity is already in the result vector. */
static gboolean has_id(const CharactersSearch *search, guint32 id)
{
    return id < EMOJI_BASE + characters_unicode_emoji_count() &&
           (search->seen[id / 8] & (1u << (id % 8)));
}

/** Append a result with bounded geometric growth and suppress repeated catalog identities. */
static gboolean append_id(CharactersSearch *search, guint32 id)
{
    if (has_id(search, id)) {
        return TRUE;
    }
    if (search->count == search->capacity) {
        if (search->capacity > G_MAXSIZE / sizeof(*search->ids) / 2) {
            return FALSE;
        }
        gsize capacity = search->capacity ? search->capacity * 2 : 1024;
        guint32 *ids = g_try_realloc_n(search->ids, capacity, sizeof(*ids));
        if (!ids) {
            return FALSE;
        }
        search->ids = ids;
        search->capacity = capacity;
    }
    search->ids[search->count++] = id;
    if (id < EMOJI_BASE + characters_unicode_emoji_count()) {
        search->seen[id / 8] |= (guint8)(1u << (id % 8));
    }
    return TRUE;
}

/** Execute a prepared Han predicate against the immutable database. */
static gboolean han_matches(gunichar point, const struct Query *query, gsize index)
{
    const guint32 *rule = query->han_rules[index];
    switch (rule[0]) {
        case CHARACTERS_HAN_ORDINARY:
        case CHARACTERS_HAN_FIELD:
            return characters_unihan_match_fields(point, rule[1], query->han_terms[index]);
        case CHARACTERS_HAN_RADICAL:
        case CHARACTERS_HAN_STROKES:
            return characters_unihan_match_group(point, rule[0] == CHARACTERS_HAN_RADICAL,
                                                 rule[2]);
        default:
            return FALSE;
    }
}

/** Resolve scalar metadata on demand; empty and numeric queries avoid name allocations. */
static gboolean scalar_query_matches(const CharactersSearch *search, gunichar point,
                                      const struct Query *query)
{
    if (matches_points(query, &point, 1)) {
        return TRUE;
    }
    if (query->exact) {
        return FALSE;
    }
    if (!query->term_count) {
        return TRUE;
    }
    /* Explicit field-only searches need no allocation of scalar name metadata. */
    gboolean needs_metadata = FALSE;
    for (gsize index = 0; index < query->term_count; index++) {
        if (query->han_rules[index][0] != CHARACTERS_HAN_ORDINARY) {
            if (!han_matches(point, query, index)) {
                return FALSE;
            }
        } else {
            needs_metadata = TRUE;
        }
    }
    if (!needs_metadata) {
        return TRUE;
    }
    gchar *name = scalar_search_name(search, point);
    gchar *aliases = characters_unicode_aliases(point);
    const gchar *block = characters_unicode_find_block(point);
    const gchar *script = characters_scalar_category(point) != U_UNASSIGNED ?
        characters_scalar_script_name((gsize)characters_scalar_script(point)) : NULL;
    gint group = category_for(search, point);
    const gchar *category = group >= 0 ? search->category_labels[group] : NULL;
    gboolean matches = TRUE;
    for (gsize index = 0; index < query->term_count; index++) {
        const gchar *term = query->terms[index];
        if (query->han_rules[index][0] != CHARACTERS_HAN_ORDINARY) {
            continue;
        }
        if (!contains_term(name, term) && !contains_term(aliases, term) &&
            !contains_term(block, term) && !contains_term(script, term) &&
            !contains_term(category, term) && !han_matches(point, query, index)) {
            matches = FALSE;
            break;
        }
    }
    g_free(name);
    g_free(aliases);
    return matches;
}

/** Scan the selected scalar repertoire in numeric order, bounded by block or plane. */
static gboolean scan_scalars(CharactersSearch *search, const struct Query *query)
{
    guint32 start = 0;
    guint32 end = SCALAR_LIMIT - 1;
    if (search->scope == CHARACTERS_SEARCH_BLOCK && search->filter >= 0) {
        start = characters_unicode_block_start((gsize)search->filter);
        end = characters_unicode_block_end((gsize)search->filter);
    }
    if ((search->scope == CHARACTERS_SEARCH_PLANE ||
         search->scope == CHARACTERS_SEARCH_UNASSIGNED) && search->filter >= 0) {
        start = (guint32)search->filter << 16;
        end = start + 0xFFFFu;
    }
    if (query->exact) {
        if (query->point_count != 1 || query->points[0] < start || query->points[0] > end) {
            return TRUE;
        }
        start = end = query->points[0];
    }
    for (gunichar point = start; point <= end; point++) {
        if ((point % CANCEL_INTERVAL == 0) && is_canceled(search)) {
            return TRUE;
        }
        gboolean assigned = characters_unicode_is_assigned(point);
        gboolean included = search->scope == CHARACTERS_SEARCH_UNASSIGNED ?
            characters_unicode_is_scalar(point) && !assigned : assigned;
        if (included && scalar_matches(search, point) &&
            scalar_query_matches(search, point, query) && !append_id(search, point)) {
            return FALSE;
        }
    }
    return TRUE;
}

/** Scan only the explicitly supplied repertoire, preserving its order and deduplicating results. */
static gboolean scan_points(CharactersSearch *search, const struct Query *query)
{
    for (gsize index = 0; index < search->point_count; index++) {
        if (index % CANCEL_INTERVAL == 0 && is_canceled(search)) {
            return TRUE;
        }
        gunichar point = search->points[index];
        if (scalar_matches(search, point) && scalar_query_matches(search, point, query) &&
            !append_id(search, point)) {
            return FALSE;
        }
    }
    return TRUE;
}

/** Search a catalog sequence with exact sequence matching or English metadata terms. */
static gboolean sequence_matches(const struct Query *query, const gchar *text, const gchar *name,
                             const gchar *group, gunichar *points, gsize *count)
{
    *count = characters_icu_decode_utf8(text, points, MAX_SEQUENCE_POINTS);
    return *count && (matches_points(query, points, *count) ||
           matches_terms(query, name, NULL, NULL, NULL, group));
}

/** Match localized terms alongside the catalog name and category without changing exact queries. */
static gboolean emoji_query_matches(const CharactersSearch *search, const struct Query *query,
                                gsize entry, const gchar *name, const gchar *group)
{
    if (query->exact) {
        return query->literal_letter && search->scope == CHARACTERS_SEARCH_EMOJI &&
               characters_emoji_keyword_matches(entry, search->locale, query->text);
    }
    for (gsize index = 0; index < query->term_count; index++) {
        const gchar *term = query->terms[index];
        if (!contains_term(name, term) && !contains_term(group, term) &&
            !characters_emoji_keyword_matches(entry, search->locale, term)) {
            return FALSE;
        }
    }
    return TRUE;
}

/** Scan emoji in semantic group order; global browsing merges single scalars. */
static gboolean scan_emoji(CharactersSearch *search, const struct Query *query)
{
    for (gsize group = 0; group < characters_unicode_emoji_group_count(); group++) {
        if (search->scope == CHARACTERS_SEARCH_EMOJI && search->filter >= 0 &&
            group != (gsize)search->filter) {
            continue;
        }
        const gchar *group_name = characters_unicode_emoji_group_name(group);
        for (gsize index = 0; index < characters_unicode_emoji_group_size(group); index++) {
            if (is_canceled(search)) {
                return TRUE;
            }
            gsize entry = characters_unicode_emoji_group_index(group, index);
            gchar *text = characters_unicode_emoji_text(entry);
            if (!text) {
                return FALSE;
            }
            gunichar points[MAX_SEQUENCE_POINTS];
            gsize count;
            gboolean match = sequence_matches(query, text, characters_unicode_emoji_label(entry),
                                          group_name, points, &count);
            if (!match && count) {
                match = emoji_query_matches(search, query, entry,
                    characters_unicode_emoji_label(entry), group_name);
            }
            g_free(text);
            if (!count) {
                return FALSE;
            }
            if (match && search->category >= 0 &&
                (count != 1 || category_for(search, points[0]) != search->category)) {
                continue;
            }
            if (match && !append_id(search, search->scope == CHARACTERS_SEARCH_ALL && count == 1 ?
                                    points[0] : EMOJI_BASE + (guint32)entry)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

/** Add an exact noncatalog emoji identity without changing ordinary browse counts. */
static gboolean scan_emoji_alias(CharactersSearch *search, const struct Query *query)
{
    if (is_canceled(search) || !query->exact || !query->point_count ||
        (search->scope == CHARACTERS_SEARCH_ALL && query->point_count == 1)) {
        return TRUE;
    }
    gchar *text = characters_unicode_encode(query->points, query->point_count);
    gsize alias = characters_emoji_alias_index(text);
    g_free(text);
    if (alias == G_MAXSIZE || alias >= NAMED_BASE - EMOJI_ALIAS_BASE) {
        return TRUE;
    }
    if (search->category >= 0 && (query->point_count != 1 ||
        category_for(search, query->points[0]) != search->category)) {
        return TRUE;
    }
    if (search->scope == CHARACTERS_SEARCH_EMOJI && search->filter >= 0) {
        gsize canonical = characters_emoji_alias_canonical(alias);
        gsize group = (gsize)search->filter;
        gboolean found = FALSE;
        if (canonical == G_MAXSIZE) {
            /* Standalone keycap bases belong to Symbols, not Flags or People. */
            const gchar *name = characters_unicode_emoji_group_name(group);
            found = name && strcmp(name, "Symbols") == 0;
        } else {
            for (gsize index = 0; index < characters_unicode_emoji_group_size(group); index++) {
                if (characters_unicode_emoji_group_index(group, index) == canonical) {
                    found = TRUE;
                    break;
                }
            }
        }
        if (!found) {
            return TRUE;
        }
    }
    return append_id(search, EMOJI_ALIAS_BASE + (guint32)alias);
}

/** Scan standardized named sequences, retaining their distinct catalog identity. */
static gboolean scan_named(CharactersSearch *search, const struct Query *query)
{
    if (search->category >= 0) {
        return TRUE;
    }
    for (gsize index = 0; index < characters_unicode_named_count(); index++) {
        if (is_canceled(search)) {
            return TRUE;
        }
        gchar *text = characters_unicode_named_text(index);
        if (!text) {
            return FALSE;
        }
        gunichar points[MAX_SEQUENCE_POINTS];
        gsize count;
        gboolean match = sequence_matches(query, text, characters_unicode_named_name(index),
                                      NULL, points, &count);
        g_free(text);
        if (!count) {
            return FALSE;
        }
        if (match && search->scope == CHARACTERS_SEARCH_ALL) {
            gsize emoji_index = characters_unicode_emoji_index(points, count);
            if (emoji_index != G_MAXSIZE && has_id(search, EMOJI_BASE + (guint32)emoji_index)) {
                continue;
            }
            if (count == 1 && has_id(search, points[0])) {
                continue;
            }
        }
        if (match && !append_id(search, NAMED_BASE + (guint32)index)) {
            return FALSE;
        }
    }
    return TRUE;
}

/** Allocate an unprepared worker request with an independently owned result vector. */
CharactersSearch *characters_search_allocate(CharactersSearchScope scope, gint filter,
                                             gint category)
{
    CharactersSearch *search = g_try_new0(CharactersSearch, 1);
    if (!search) {
        return NULL;
    }
    search->scope = scope;
    search->filter = filter;
    search->category = category;
    search->status = CHARACTERS_SEARCH_INVALID;
    atomic_init(&search->canceled, FALSE);
    return search;
}

/** Copy a scalar repertoire while rejecting invalid points and oversized input. */
CharactersSearch *characters_search_allocate_points(const gunichar *points, gsize length,
                                                    gint category)
{
    if (length > SCALAR_LIMIT || (length && !points)) {
        return NULL;
    }
    CharactersSearch *search = characters_search_allocate(CHARACTERS_SEARCH_ALL, -1, category);
    if (!search) {
        return NULL;
    }
    search->points = g_try_malloc_n((length ? length : 1), sizeof(*points));
    if (!search->points) {
        characters_search_free(search);
        return NULL;
    }
    for (gsize index = 0; index < length; index++) {
        if (!characters_unicode_is_scalar(points[index])) {
            characters_search_free(search);
            return NULL;
        }
        search->points[index] = points[index];
    }
    search->point_count = length;
    return search;
}

void characters_search_set_locale(CharactersSearch *search, const gchar *locale)
{
    if (search && !search->started && locale &&
        strnlen(locale, sizeof(search->locale)) < sizeof(search->locale)) {
        strcpy(search->locale, locale);
    }
}

/** Copy the prepared language mask before worker scanning. */
void characters_search_set_scripts(CharactersSearch *search, const guint8 *scripts,
                                   gsize length)
{
    if (!search || search->started || !scripts || length != G_N_ELEMENTS(search->scripts)) {
        return;
    }
    for (gsize index = 0; index < length; index++) {
        search->scripts[index] = scripts[index] != 0;
    }
}

/** Copy a validated union of semantic rules before worker traversal. */
gboolean characters_search_set_rules(CharactersSearch *search, const guint32 *rules,
                                 gsize length)
{
    if (!search || search->started || search->scope != CHARACTERS_SEARCH_SYMBOLS ||
        search->rule_count || !rules || !length || length > MAX_RULE_WORDS ||
        length % RULE_WORDS) {
        return FALSE;
    }
    for (gsize index = 0; index < length; index += RULE_WORDS) {
        if (!rules[index] || rules[index] & ~UINT32_C(0x3FFFFFFF) ||
            rules[index + 1] & ~UINT32_C(0x3FFFF) ||
            rules[index + 2] > rules[index + 3] || rules[index + 3] >= SCALAR_LIMIT) {
            return FALSE;
        }
    }
    memcpy(search->rules, rules, length * sizeof(*rules));
    search->rule_count = length;
    return TRUE;
}

/** Validate prepared Han operands without interpreting query syntax. */
static gboolean valid_han_rule(const guint32 *rule)
{
    switch (rule[0]) {
        case CHARACTERS_HAN_ORDINARY:
        case CHARACTERS_HAN_FIELD:
            return rule[1] && !(rule[1] & ~CHARACTERS_HAN_TEXT_FIELDS) && !rule[2];
        case CHARACTERS_HAN_RADICAL:
        case CHARACTERS_HAN_STROKES:
            return !rule[1] && rule[2] > 0 &&
                   rule[2] <= (rule[0] == CHARACTERS_HAN_RADICAL ? 214u : 128u);
        case CHARACTERS_HAN_REJECT:
            return !rule[1] && !rule[2];
        default:
            return FALSE;
    }
}

/** Copy bounded parser output and prepared Han operands. */
gboolean characters_search_prepare(CharactersSearch *search, const gchar *text,
                               const gunichar *points, gsize point_count,
                               gchar *const *terms, gsize term_count,
                               gchar *const *han_terms, gsize han_count,
                               const guint32 *han_rules, gsize han_rule_count,
                               gboolean exact, gboolean literal_letter)
{
    if (!search || search->started || search->query || !text ||
        strnlen(text, MAX_QUERY_BYTES + 1) > MAX_QUERY_BYTES ||
        point_count > MAX_SEQUENCE_POINTS || (point_count && !points) ||
        term_count > MAX_TERMS || (term_count && (!terms || !han_terms || !han_rules)) ||
        han_count != term_count || han_rule_count != term_count * HAN_WORDS ||
        (exact && (!point_count || term_count)) ||
        (literal_letter && (!exact || point_count != 1))) {
        return FALSE;
    }
    for (gsize index = 0; index < point_count; index++) {
        if (!characters_unicode_is_scalar(points[index])) {
            return FALSE;
        }
    }
    gsize offset = 0;
    for (gsize index = 0; index < term_count; index++) {
        if (!terms[index] || !han_terms[index] ||
            strnlen(han_terms[index], MAX_HAN_TERM_BYTES) >= MAX_HAN_TERM_BYTES ||
            !valid_han_rule(han_rules + index * HAN_WORDS)) {
            return FALSE;
        }
        gsize length = strnlen(terms[index], MAX_QUERY_BYTES + 1);
        if (!length || length >= MAX_QUERY_BYTES + 1 - offset) {
            return FALSE;
        }
        offset += length + 1;
    }
    struct Query *query = g_try_new0(struct Query, 1);
    if (!query) {
        search->status = CHARACTERS_SEARCH_FAILED;
        return FALSE;
    }
    strcpy(query->text, text);
    if (point_count) {
        memcpy(query->points, points, point_count * sizeof(*points));
    }
    query->point_count = point_count;
    query->term_count = term_count;
    query->exact = exact;
    query->literal_letter = literal_letter;
    offset = 0;
    for (gsize index = 0; index < term_count; index++) {
        gsize length = strlen(terms[index]) + 1;
        query->terms[index] = query->term_text + offset;
        memcpy(query->terms[index], terms[index], length);
        offset += length;
        strcpy(query->han_terms[index], han_terms[index]);
        memcpy(query->han_rules[index], han_rules + index * HAN_WORDS,
               sizeof(query->han_rules[index]));
        query->han_field |= query->han_rules[index][0] != CHARACTERS_HAN_ORDINARY;
    }
    search->query = query;
    return TRUE;
}

void characters_search_free(CharactersSearch *search)
{
    if (search) {
        g_free(search->query);
        g_free(search->points);
        g_free(search->ids);
        g_free(search->seen);
        g_free(search);
    }
}

void characters_search_cancel(CharactersSearch *search)
{
    if (search) {
        atomic_store_explicit(&search->canceled, TRUE, memory_order_relaxed);
    }
}

/** Validate one filter slot against native registry bounds; labels are presentation policy. */
static gboolean valid_filter(CharactersSearchScope scope, gsize index)
{
    switch (scope) {
        case CHARACTERS_SEARCH_EMOJI:
            return characters_unicode_emoji_group_name(index) != NULL;
        case CHARACTERS_SEARCH_SYMBOLS:
            return index < SYMBOL_COUNT;
        case CHARACTERS_SEARCH_SCRIPT:
            return characters_scalar_script_name(index) != NULL;
        case CHARACTERS_SEARCH_BLOCK:
            return characters_unicode_block_name(index) != NULL;
        case CHARACTERS_SEARCH_LANGUAGE:
            return characters_language_name(index) != NULL;
        case CHARACTERS_SEARCH_RADICAL:
            return characters_unihan_group_name(TRUE, index) != NULL;
        case CHARACTERS_SEARCH_STROKES:
            return characters_unihan_group_name(FALSE, index) != NULL;
        case CHARACTERS_SEARCH_PLANE:
        case CHARACTERS_SEARCH_UNASSIGNED:
            return index < PLANE_COUNT;
        default:
            return FALSE;
    }
}

gboolean characters_search_set_metadata(CharactersSearch *search, const guint32 *masks,
                                          gsize mask_count, gchar *const *labels,
                                          gsize label_count, gchar *const *fallbacks,
                                          gsize fallback_count)
{
    if (!search || search->started || search->metadata_ready || !masks || !labels ||
        !fallbacks || mask_count != CATEGORY_COUNT || label_count != CATEGORY_COUNT ||
        fallback_count != FALLBACK_COUNT) {
        return FALSE;
    }
    guint32 used = 0;
    for (gsize index = 0; index < mask_count; index++) {
        if (!masks[index] || (masks[index] & ~UINT32_C(0x3FFFFFFF)) ||
            (used & masks[index]) || !labels[index] || !labels[index][0] ||
            strnlen(labels[index], METADATA_LABEL_BYTES) >= METADATA_LABEL_BYTES) {
            return FALSE;
        }
        used |= masks[index];
    }
    for (gsize index = 0; index < fallback_count; index++) {
        if (!fallbacks[index] || !fallbacks[index][0] ||
            strnlen(fallbacks[index], METADATA_LABEL_BYTES) >= METADATA_LABEL_BYTES) {
            return FALSE;
        }
    }
    for (gsize type = 0; type < G_N_ELEMENTS(search->category_groups); type++) {
        search->category_groups[type] = -1;
        for (gsize index = 0; index < mask_count; index++) {
            if (masks[index] & (UINT32_C(1) << type)) {
                search->category_groups[type] = (gint)index;
                break;
            }
        }
    }
    for (gsize index = 0; index < label_count; index++) {
        strcpy(search->category_labels[index], labels[index]);
    }
    for (gsize index = 0; index < fallback_count; index++) {
        strcpy(search->fallback_labels[index], fallbacks[index]);
    }
    search->metadata_ready = TRUE;
    return TRUE;
}

CharactersSearchStatus characters_search_run(CharactersSearch *search)
{
    if (!search || search->started) {
        return CHARACTERS_SEARCH_INVALID;
    }
    search->started = TRUE;
    const struct Query *query = search->query;
    if (search->scope < CHARACTERS_SEARCH_ALL || search->scope > CHARACTERS_SEARCH_UNASSIGNED ||
        search->filter < -1 || (search->filter >= 0 &&
        !valid_filter(search->scope, (gsize)search->filter)) ||
        search->category < -1 || search->category >= (gint)CATEGORY_COUNT ||
        (search->scope == CHARACTERS_SEARCH_SYMBOLS && !search->rule_count) || !query ||
        !search->metadata_ready) {
        return search->status;
    }
    if (is_canceled(search)) {
        search->status = CHARACTERS_SEARCH_CANCELED;
        return search->status;
    }
    gsize emoji_count = characters_unicode_emoji_count();
    if (emoji_count > EMOJI_ALIAS_BASE - EMOJI_BASE ||
        characters_emoji_alias_count() > NAMED_BASE - EMOJI_ALIAS_BASE ||
        characters_unicode_named_count() >= G_MAXUINT32 - NAMED_BASE) {
        search->status = CHARACTERS_SEARCH_FAILED;
        return search->status;
    }
    search->seen = g_try_malloc0_n((EMOJI_BASE + emoji_count + 7) / 8, 1);
    if (!search->seen) {
        search->status = CHARACTERS_SEARCH_FAILED;
        return search->status;
    }
    gboolean success = TRUE;
    if (search->points) {
        success = scan_points(search, query);
    } else if (search->scope != CHARACTERS_SEARCH_EMOJI && search->scope != CHARACTERS_SEARCH_NAMED) {
        success = scan_scalars(search, query);
    }
    if (!search->points && success && !query->han_field && !is_canceled(search) &&
        (search->scope == CHARACTERS_SEARCH_ALL || search->scope == CHARACTERS_SEARCH_EMOJI)) {
        success = scan_emoji(search, query) && scan_emoji_alias(search, query);
    }
    if (!search->points && success && !query->han_field && !is_canceled(search) &&
        (search->scope == CHARACTERS_SEARCH_ALL || search->scope == CHARACTERS_SEARCH_NAMED)) {
        success = scan_named(search, query);
    }
    g_free(search->seen);
    search->seen = NULL;
    g_free(search->query);
    search->query = NULL;
    search->status = !success ? CHARACTERS_SEARCH_FAILED :
                     is_canceled(search) ? CHARACTERS_SEARCH_CANCELED : CHARACTERS_SEARCH_OK;
    if (search->status != CHARACTERS_SEARCH_OK) {
        g_free(search->ids);
        search->ids = NULL;
        search->count = search->capacity = 0;
    }
    return search->status;
}

gsize characters_search_count(const CharactersSearch *search)
{
    return search && search->status == CHARACTERS_SEARCH_OK ? search->count : 0;
}

guint32 characters_search_id(const CharactersSearch *search, gsize index)
{
    return index < characters_search_count(search) ? search->ids[index] : G_MAXUINT32;
}

gchar *characters_search_text(guint32 id)
{
    if (id < SCALAR_LIMIT) {
        return characters_unicode_is_scalar(id) ? characters_unicode_encode(&id, 1) : NULL;
    }
    if (id < EMOJI_ALIAS_BASE) {
        return characters_unicode_emoji_text(id - EMOJI_BASE);
    }
    if (id < NAMED_BASE) {
        const gchar *text = characters_emoji_alias_text(id - EMOJI_ALIAS_BASE);
        return text ? characters_try_strdup(text) : NULL;
    }
    return characters_unicode_named_text(id - NAMED_BASE);
}

gchar *characters_search_sequence_label(guint32 id)
{
    if (id < EMOJI_BASE) {
        return NULL;
    }
    if (id >= EMOJI_ALIAS_BASE && id < NAMED_BASE) {
        return characters_emoji_alias_name(id - EMOJI_ALIAS_BASE);
    }
    const gchar *name = id < EMOJI_ALIAS_BASE ? characters_unicode_emoji_label(id - EMOJI_BASE) :
                       characters_unicode_named_name(id - NAMED_BASE);
    return name ? characters_try_strdup(name) : NULL;
}
