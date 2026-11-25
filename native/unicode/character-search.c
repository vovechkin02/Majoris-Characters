#include "character-search.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <unicode/uchar.h>
#include <unicode/utf8.h>

#include "emoji-catalog.h"
#include "language-collections.h"
#include "scalar-properties.h"
#include "unicode-engine.h"
#include "unihan.h"

#define N_(message) (message)
#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define SCALAR_LIMIT 0x110000u
#define EMOJI_BASE SCALAR_LIMIT
#define EMOJI_ALIAS_BASE 0x180000u
#define NAMED_BASE 0x200000u
#define MAX_QUERY_BYTES 512
#define MAX_SEQUENCE_POINTS CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH
#define MAX_TERMS 64
#define CANCEL_INTERVAL 256u

/** Semantic symbol collections; also the index into symbol_names. */
enum SymbolCollection {
    /** Punctuation collection slot. */
    SYMBOL_PUNCTUATION,
    /** Arrows collection slot. */
    SYMBOL_ARROWS,
    /** Bullets & Stars collection slot. */
    SYMBOL_BULLETS,
    /** Pictures & Shapes collection slot. */
    SYMBOL_PICTURES,
    /** Currencies collection slot. */
    SYMBOL_CURRENCY,
    /** Mathematics collection slot. */
    SYMBOL_MATH,
    /** Letters collection slot. */
    SYMBOL_LETTERS,
    /** Numbers & Fractions collection slot. */
    SYMBOL_NUMBERS,
    /** Technical collection slot. */
    SYMBOL_TECHNICAL,
    /** Music collection slot. */
    SYMBOL_MUSIC,
    /** Games collection slot. */
    SYMBOL_GAMES,
    /** Braille collection slot. */
    SYMBOL_BRAILLE,
    /** Combining Marks collection slot. */
    SYMBOL_MARKS,
    /** Box Drawing collection slot. */
    SYMBOL_BOX_DRAWING,
    /** Phonetics collection slot. */
    SYMBOL_PHONETICS,
    /** Enclosed collection slot. */
    SYMBOL_ENCLOSED,
    /** Special & Formatting collection slot. */
    SYMBOL_SPECIAL,
    /** Superscripts collection slot. */
    SYMBOL_SUPERSCRIPTS,
    /** Subscripts collection slot. */
    SYMBOL_SUBSCRIPTS,
    /** Fractions collection slot. */
    SYMBOL_FRACTIONS,
    /** Whitespace collection slot. */
    SYMBOL_WHITESPACE,
    /** Quotation Marks collection slot. */
    SYMBOL_QUOTATION,
    /** Accessibility Symbols collection slot. */
    SYMBOL_ACCESSIBILITY,
    /** Letterlike Symbols collection slot. */
    SYMBOL_LETTERLIKE,
    /** Religious Symbols collection slot. */
    SYMBOL_RELIGIOUS,
    /** Variation Selectors collection slot. */
    SYMBOL_VARIATION,
    /** Block Elements collection slot. */
    SYMBOL_BLOCK_ELEMENTS,
};

/** Stable English labels corresponding to the SymbolCollection slots. */
static const char *const symbol_names[] = {
    [SYMBOL_PUNCTUATION] = N_("Punctuation"),
    [SYMBOL_ARROWS] = N_("Arrows"),
    [SYMBOL_BULLETS] = N_("Bullets & Stars"),
    [SYMBOL_PICTURES] = N_("Pictures & Shapes"),
    [SYMBOL_CURRENCY] = N_("Currencies"),
    [SYMBOL_MATH] = N_("Mathematics"),
    [SYMBOL_LETTERS] = N_("Letters"),
    [SYMBOL_NUMBERS] = N_("Numbers & Fractions"),
    [SYMBOL_TECHNICAL] = N_("Technical"),
    [SYMBOL_MUSIC] = N_("Music"),
    [SYMBOL_GAMES] = N_("Games"),
    [SYMBOL_BRAILLE] = N_("Braille"),
    [SYMBOL_MARKS] = N_("Combining Marks"),
    [SYMBOL_BOX_DRAWING] = N_("Box Drawing"),
    [SYMBOL_PHONETICS] = N_("Phonetics"),
    [SYMBOL_ENCLOSED] = N_("Enclosed"),
    [SYMBOL_SPECIAL] = N_("Special & Formatting"),
    [SYMBOL_SUPERSCRIPTS] = N_("Superscripts"),
    [SYMBOL_SUBSCRIPTS] = N_("Subscripts"),
    [SYMBOL_FRACTIONS] = N_("Fractions"),
    [SYMBOL_WHITESPACE] = N_("Whitespace"),
    [SYMBOL_QUOTATION] = N_("Quotation Marks"),
    [SYMBOL_ACCESSIBILITY] = N_("Accessibility Symbols"),
    [SYMBOL_LETTERLIKE] = N_("Letterlike Symbols"),
    [SYMBOL_RELIGIOUS] = N_("Religious Symbols"),
    [SYMBOL_VARIATION] = N_("Variation Selectors"),
    [SYMBOL_BLOCK_ELEMENTS] = N_("Block Elements"),
};

/** Unicode planes, indexed by the high sixteen bits of a scalar. */
static const char *const plane_names[] = {
    N_("0: Basic Multilingual Plane"),
    N_("1: Supplementary Multilingual Plane"),
    N_("2: Supplementary Ideographic Plane"),
    N_("3: Tertiary Ideographic Plane"),
    N_("4: Plane 4 (Reserved)"),
    N_("5: Plane 5 (Reserved)"),
    N_("6: Plane 6 (Reserved)"),
    N_("7: Plane 7 (Reserved)"),
    N_("8: Plane 8 (Reserved)"),
    N_("9: Plane 9 (Reserved)"),
    N_("10: Plane 10 (Reserved)"),
    N_("11: Plane 11 (Reserved)"),
    N_("12: Plane 12 (Reserved)"),
    N_("13: Plane 13 (Reserved)"),
    N_("14: Supplementary Special-purpose Plane"),
    N_("15: Supplementary Private Use Area-A"),
    N_("16: Supplementary Private Use Area-B"),
};

/** Broad Unicode general-category groups; also the index into category_names. */
enum CategoryGroup {
    /** Letters collection slot. */
    CATEGORY_LETTERS,
    /** Marks collection slot. */
    CATEGORY_MARKS,
    /** Numbers collection slot. */
    CATEGORY_NUMBERS,
    /** Punctuation collection slot. */
    CATEGORY_PUNCTUATION,
    /** Symbols collection slot. */
    CATEGORY_SYMBOLS,
    /** Separators collection slot. */
    CATEGORY_SEPARATORS,
    /** Controls & Formatting collection slot. */
    CATEGORY_CONTROLS,
    /** Private Use collection slot. */
    CATEGORY_PRIVATE_USE,
};

/** Stable English labels corresponding to the CategoryGroup slots. */
static const char *const category_names[] = {
    [CATEGORY_LETTERS] = N_("Letters"),
    [CATEGORY_MARKS] = N_("Marks"),
    [CATEGORY_NUMBERS] = N_("Numbers"),
    [CATEGORY_PUNCTUATION] = N_("Punctuation"),
    [CATEGORY_SYMBOLS] = N_("Symbols"),
    [CATEGORY_SEPARATORS] = N_("Separators"),
    [CATEGORY_CONTROLS] = N_("Controls & Formatting"),
    [CATEGORY_PRIVATE_USE] = N_("Private Use"),
};

/** The worker owns mutable result storage; only the cancellation flag is shared during run. */
struct CharactersSearch {
    /** Primary collection, immutable after creation. */
    CharactersSearchScope scope;
    /** Collection-specific filter slot, or -1. */
    int filter;
    /** General-category group slot, or -1. */
    int category;
    /** Bounded, owned input; NULL denotes invalid input or missing allocation. */
    char *query;
    /** Locale captured before worker execution; defaults to English. */
    char locale[64];
    /** Configured XKB layout IDs captured before execution. */
    char input_sources[1024];
    /** Script membership mask resolved once before scanning a language collection. */
    bool scripts[CHARACTERS_LANGUAGE_SCRIPT_LIMIT];
    /** Optional copied scalar repertoire; NULL denotes ordinary catalog search. */
    uint32_t *points;
    /** Number of input scalars in the explicit repertoire. */
    size_t point_count;
    /** Set by any thread; observed at bounded scan intervals. */
    atomic_bool canceled;
    /** Owned result identity vector. */
    uint32_t *ids;
    /** Number of initialized result identities. */
    size_t count;
    /** Allocated vector capacity, in identities. */
    size_t capacity;
    /** Scalar and emoji identities already emitted, used to merge catalog matches. */
    uint8_t *seen;
    /** Published only once, after scanning finishes. */
    CharactersSearchStatus status;
    /** Enforces single-run ownership of mutable result storage. */
    bool started;
};

/** Parsed input, containing only stack-owned data; terms point into the local buffer. */
struct Query {
    /** Owned, bounded, tokenized copy of the query. */
    char text[MAX_QUERY_BYTES + 1];
    /** Borrowed pointers into text, valid for the query's stack lifetime. */
    char *terms[MAX_TERMS];
    /** Number of parsed AND terms. */
    size_t term_count;
    /** Independently folded Han search terms; empty if folding was unavailable. */
    char han_terms[MAX_TERMS][MAX_QUERY_BYTES * 3 + 1];
    /** An explicit Han field disables non-Han sequence matching. */
    bool han_field;
    /** Decoded exact scalar sequence, never normalized. */
    uint32_t points[MAX_SEQUENCE_POINTS];
    /** Number of valid scalars in points. */
    size_t point_count;
    /** Whether metadata matching is disabled for an exact numeric or literal query. */
    bool exact;
    /** True for a single literal Unicode letter, usable as an emoji keyword. */
    bool literal_letter;
};

/** Return an ASCII lowercase byte without dependence on the process locale. */
static unsigned char lower_ascii(unsigned char byte) {
    return byte >= 'A' && byte <= 'Z' ? byte + ('a' - 'A') : byte;
}

/** Recognize ASCII query separators; Unicode text itself is not altered. */
static bool is_space(char byte) {
    return byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n';
}

/** Match a term against a nullable English metadata field, ignoring ASCII case. */
static bool contains_term(const char *field, const char *term) {
    if (!field) {
        return false;
    }
    for (; *field; field++) {
        size_t index = 0;
        while (term[index] && field[index] &&
               lower_ascii((unsigned char)field[index]) ==
               lower_ascii((unsigned char)term[index])) {
            index++;
        }
        if (!term[index]) {
            return true;
        }
    }
    return false;
}

/** Parse a numeric token without accepting signs, trailing characters, or overflow. */
static bool parse_number(const char *text, size_t length, unsigned base, uint32_t *point) {
    if (!length || length > 7) {
        return false;
    }
    uint32_t value = 0;
    for (size_t index = 0; index < length; index++) {
        unsigned char byte = lower_ascii((unsigned char)text[index]);
        unsigned digit = byte >= '0' && byte <= '9' ? byte - '0' :
                         byte >= 'a' && byte <= 'f' ? byte - 'a' + 10 : 255;
        if (digit >= base || value > (0x10FFFFu - digit) / base) {
            return false;
        }
        value = value * base + digit;
    }
    if (!characters_unicode_is_scalar(value)) {
        return false;
    }
    *point = value;
    return true;
}

/** Decode exact UTF-8 without normalization; return zero for invalid or oversized sequences. */
static size_t decode_text(const char *text, uint32_t *points) {
    if (!text || strlen(text) > MAX_QUERY_BYTES) {
        return 0;
    }
    int32_t length = (int32_t)strlen(text);
    int32_t offset = 0;
    size_t count = 0;
    while (offset < length) {
        UChar32 point;
        U8_NEXT((const uint8_t *)text, offset, length, point);
        if (point < 0 || count == MAX_SEQUENCE_POINTS) {
            return 0;
        }
        points[count++] = (uint32_t)point;
    }
    return count;
}

/** Parse explicit U+, 0x and dec: tokens; each token must declare its base. */
static bool parse_explicit(const char *text, struct Query *query) {
    while (*text) {
        unsigned base = 16;
        if (lower_ascii(text[0]) == 'u' && text[1] == '+') {
            text += 2;
        } else if (text[0] == '0' && lower_ascii(text[1]) == 'x') {
            text += 2;
        } else if (strncasecmp(text, "dec:", 4) == 0) {
            base = 10;
            text += 4;
        } else {
            return false;
        }
        const char *end = text;
        while (*end && !is_space(*end)) {
            end++;
        }
        if (query->point_count == MAX_SEQUENCE_POINTS ||
            !parse_number(text, (size_t)(end - text), base,
                          &query->points[query->point_count])) {
            return false;
        }
        query->point_count++;
        text = end;
        while (is_space(*text)) {
            text++;
        }
    }
    query->exact = true;
    return query->point_count > 0;
}

/** Trim and validate input, recognize explicit or literal queries, and split metadata terms. */
static bool parse_query(const char *input, struct Query *query) {
    if (!input) {
        return false;
    }
    int32_t length = (int32_t)strlen(input);
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT((const uint8_t *)input, offset, length, point);
        if (point < 0) {
            return false;
        }
    }
    while (is_space(*input)) {
        input++;
    }
    size_t bytes = strlen(input);
    while (bytes && is_space(input[bytes - 1])) {
        bytes--;
    }
    memcpy(query->text, input, bytes);
    query->text[bytes] = '\0';
    if (!bytes) {
        return true;
    }
    if (strncasecmp(query->text, "U+", 2) == 0 ||
        strncasecmp(query->text, "0x", 2) == 0 ||
        strncasecmp(query->text, "dec:", 4) == 0) {
        return parse_explicit(query->text, query);
    }
    query->point_count = decode_text(query->text, query->points);
    if (query->point_count == 1) {
        int category = characters_scalar_category(query->points[0]);
        query->literal_letter = query->points[0] > 0x7F &&
            category >= U_UPPERCASE_LETTER && category <= U_OTHER_LETTER;
        query->exact = true;
        return true;
    }
    if (query->point_count > 1 &&
        (characters_unicode_emoji_index(query->points, query->point_count) != SIZE_MAX ||
         characters_emoji_alias_index(query->text) != SIZE_MAX)) {
        query->exact = true;
        return true;
    }
    uint32_t raw;
    if (bytes >= 2 && bytes <= 6 && parse_number(query->text, bytes, 16, &raw)) {
        bool has_digit = false;
        for (size_t index = 0; index < bytes; index++) {
            has_digit |= query->text[index] >= '0' && query->text[index] <= '9';
        }
        if (has_digit) {
            query->points[0] = raw;
            query->point_count = 1;
            query->exact = true;
            return true;
        }
        /* All-letter hex of at least four digits is ambiguous (e.g. FACE). */
        if (bytes >= 4) {
            query->points[0] = raw;
            query->point_count = 1;
        }
    }
    char *cursor = query->text;
    while (*cursor) {
        while (is_space(*cursor)) {
            cursor++;
        }
        if (!*cursor) {
            break;
        }
        if (query->term_count == MAX_TERMS) {
            return false;
        }
        query->terms[query->term_count++] = cursor;
        while (*cursor && !is_space(*cursor)) {
            cursor++;
        }
        if (*cursor) {
            *cursor++ = '\0';
        }
    }
    for (size_t index = 0; index < query->term_count; index++) {
        if (!characters_unihan_fold(query->terms[index], query->han_terms[index],
                                   sizeof(query->han_terms[index]))) {
            query->han_terms[index][0] = '\0';
        }
        query->han_field |= characters_unihan_is_field_query(query->han_terms[index]);
    }
    return true;
}

/** Match every query term, allowing different terms to occur in different metadata fields. */
static bool matches_terms(const struct Query *query, const char *name, const char *aliases,
                          const char *block, const char *script, const char *group) {
    if (query->exact) {
        return false;
    }
    for (size_t index = 0; index < query->term_count; index++) {
        const char *term = query->terms[index];
        if (!contains_term(name, term) && !contains_term(aliases, term) &&
            !contains_term(block, term) && !contains_term(script, term) &&
            !contains_term(group, term)) {
            return false;
        }
    }
    return true;
}

/** Compare a catalog sequence to exact numeric or literal input. */
static bool matches_points(const struct Query *query, const uint32_t *points, size_t count) {
    return count && query->point_count == count &&
           memcmp(query->points, points, count * sizeof(*points)) == 0;
}

/** Read the Unicode general-category group for a scalar; -1 denotes unknown metadata. */
static int category_for(uint32_t point) {
    int type = characters_scalar_category(point);
    switch (type) {
        case U_UPPERCASE_LETTER: case U_LOWERCASE_LETTER: case U_TITLECASE_LETTER:
        case U_MODIFIER_LETTER: case U_OTHER_LETTER:
            return CATEGORY_LETTERS;
        case U_NON_SPACING_MARK: case U_ENCLOSING_MARK: case U_COMBINING_SPACING_MARK:
            return CATEGORY_MARKS;
        case U_DECIMAL_DIGIT_NUMBER: case U_LETTER_NUMBER: case U_OTHER_NUMBER:
            return CATEGORY_NUMBERS;
        case U_DASH_PUNCTUATION: case U_START_PUNCTUATION: case U_END_PUNCTUATION:
        case U_CONNECTOR_PUNCTUATION: case U_OTHER_PUNCTUATION:
        case U_INITIAL_PUNCTUATION: case U_FINAL_PUNCTUATION:
            return CATEGORY_PUNCTUATION;
        case U_MATH_SYMBOL: case U_CURRENCY_SYMBOL: case U_MODIFIER_SYMBOL: case U_OTHER_SYMBOL:
            return CATEGORY_SYMBOLS;
        case U_SPACE_SEPARATOR: case U_LINE_SEPARATOR: case U_PARAGRAPH_SEPARATOR:
            return CATEGORY_SEPARATORS;
        case U_CONTROL_CHAR: case U_FORMAT_CHAR:
            return CATEGORY_CONTROLS;
        case U_PRIVATE_USE_CHAR:
            return CATEGORY_PRIVATE_USE;
        default:
            return -1;
    }
}

/** One inclusive range of scalars in a curated semantic collection. */
struct SymbolRange {
    /** First scalar in the range. */
    uint32_t first;
    /** Last scalar in the range. */
    uint32_t last;
};

/** Maximum number of ranges in one collection, including the terminating empty slot. */
#define SYMBOL_RANGE_SLOTS 12

/** Curated semantic ranges indexed by stable collection slot; overlaps are intentional. */
static const struct SymbolRange symbol_ranges[COUNT(symbol_names)][SYMBOL_RANGE_SLOTS] = {
    [SYMBOL_ARROWS] = {
        { 0x2190, 0x21FF },
        { 0x27F0, 0x27FF },
        { 0x2900, 0x297F },
        { 0x2B00, 0x2B11 },
        { 0x1F800, 0x1F8FF },
    },
    [SYMBOL_BULLETS] = {
        { 0x2022, 0x2022 },
        { 0x2023, 0x2023 },
        { 0x2043, 0x2043 },
        { 0x2219, 0x2219 },
        { 0x25E6, 0x25E6 },
        { 0x2605, 0x2606 },
        { 0x2720, 0x274B },
    },
    [SYMBOL_PICTURES] = {
        { 0x25A0, 0x27BF },
        { 0x1F300, 0x1F5FF },
        { 0x1F780, 0x1F7FF },
    },
    [SYMBOL_MATH] = {
        { 0x2200, 0x22FF },
        { 0x27C0, 0x27EF },
        { 0x2980, 0x2AFF },
        { 0x1D400, 0x1D7FF },
    },
    [SYMBOL_LETTERS] = {
        { 0x0000, 0x024F },
        { 0x1E00, 0x1EFF },
        { 0x2100, 0x214F },
    },
    [SYMBOL_TECHNICAL] = {
        { 0x2300, 0x23FF },
        { 0x2400, 0x245F },
        { 0x1FB00, 0x1FBFF },
        { 0x2699, 0x2699 },
        { 0x26A1, 0x26A1 },
        { 0x1F50C, 0x1F50C },
        { 0x1F4BB, 0x1F4BB },
        { 0x1F5A5, 0x1F5A8 },
    },
    [SYMBOL_MUSIC] = {
        { 0x2669, 0x266F },
        { 0x1D000, 0x1D24F },
    },
    [SYMBOL_GAMES] = {
        { 0x2654, 0x2667 },
        { 0x2680, 0x2685 },
        { 0x1F000, 0x1F0FF },
        { 0x1FA00, 0x1FA6F },
    },
    [SYMBOL_BRAILLE] = {
        { 0x2800, 0x28FF },
    },
    [SYMBOL_BOX_DRAWING] = {
        { 0x2500, 0x259F },
    },
    [SYMBOL_PHONETICS] = {
        { 0x0250, 0x02AF },
        { 0x1D00, 0x1DBF },
        { 0x10780, 0x107BF },
    },
    [SYMBOL_ENCLOSED] = {
        { 0x2460, 0x24FF },
        { 0x3200, 0x32FF },
        { 0x1F100, 0x1F2FF },
    },
    [SYMBOL_ACCESSIBILITY] = {
        { 0x267F, 0x267F },
        { 0x1F9AF, 0x1F9AF },
        { 0x1F9BB, 0x1F9BB },
        { 0x1F9BC, 0x1F9BC },
        { 0x1F9BD, 0x1F9BD },
        { 0x1F9BE, 0x1F9BE },
        { 0x1F9BF, 0x1F9BF },
        { 0x1F9CF, 0x1F9CF },
        { 0x2800, 0x28FF },
    },
    [SYMBOL_LETTERLIKE] = {
        { 0x2100, 0x214F },
    },
    [SYMBOL_RELIGIOUS] = {
        { 0x2626, 0x262A },
        { 0x262C, 0x262F },
        { 0x2638, 0x2638 },
        { 0x271D, 0x2721 },
        { 0x1F549, 0x1F54E },
        { 0x1F6D0, 0x1F6D0 },
    },
    [SYMBOL_VARIATION] = {
        { 0x180B, 0x180D },
        { 0x180F, 0x180F },
        { 0xFE00, 0xFE0F },
        { 0xE0100, 0xE01EF },
    },
    [SYMBOL_BLOCK_ELEMENTS] = {
        { 0x2580, 0x259F },
        { 0x1FB00, 0x1FB3B },
        { 0x1FB70, 0x1FB8B },
    },
    [SYMBOL_QUOTATION] = {
        { 0x0022, 0x0022 },
        { 0x0027, 0x0027 },
        { 0x300C, 0x300F },
        { 0x301D, 0x301F },
    },
};

/** Match only ranges in one indexed collection without allocating or looking up properties. */
static bool symbol_range_matches(uint32_t point, int filter) {
    if ((size_t)filter >= COUNT(symbol_ranges)) {
        return false;
    }
    for (size_t index = 0; index < SYMBOL_RANGE_SLOTS; index++) {
        const struct SymbolRange *rule = &symbol_ranges[filter][index];
        if (!rule->last) {
            break;
        }
        if (point >= rule->first && point <= rule->last) {
            return true;
        }
    }
    return false;
}

/** Classify semantic collections with Unicode properties and curated range data. */
static bool symbol_matches(uint32_t point, int filter) {
    int category = category_for(point);
    int type = characters_scalar_category(point);
    if (filter < 0) {
        if (category >= CATEGORY_MARKS && category <= CATEGORY_CONTROLS) {
            return true;
        }
        if (category != CATEGORY_LETTERS) {
            return false;
        }
        int decomposition = characters_scalar_decomposition_type(point);
        if (decomposition == U_DT_SUPER || decomposition == U_DT_SUB ||
            decomposition == U_DT_FRACTION) {
            return true;
        }
        for (size_t index = 0; index < COUNT(symbol_names); index++) {
            if (symbol_range_matches(point, (int)index)) {
                return true;
            }
        }
        return false;
    }
    switch (filter) {
        case SYMBOL_PUNCTUATION:
            return category == CATEGORY_PUNCTUATION;
        case SYMBOL_CURRENCY:
            return type == U_CURRENCY_SYMBOL;
        case SYMBOL_NUMBERS:
            return category == CATEGORY_NUMBERS;
        case SYMBOL_MARKS:
            return category == CATEGORY_MARKS;
        case SYMBOL_SPECIAL:
            return category == CATEGORY_SEPARATORS || category == CATEGORY_CONTROLS;
        case SYMBOL_SUPERSCRIPTS:
            return characters_scalar_decomposition_type(point) == U_DT_SUPER;
        case SYMBOL_SUBSCRIPTS:
            return characters_scalar_decomposition_type(point) == U_DT_SUB;
        case SYMBOL_FRACTIONS:
            return characters_scalar_decomposition_type(point) == U_DT_FRACTION ||
                   point == 0x2044;
        case SYMBOL_WHITESPACE:
            // White_Space is stable for the Unicode 17 repertoire.
            return u_hasBinaryProperty((UChar32)point, UCHAR_WHITE_SPACE);
        case SYMBOL_QUOTATION:
            if (type == U_INITIAL_PUNCTUATION || type == U_FINAL_PUNCTUATION) {
                return true;
            }
            break;
        case SYMBOL_MATH:
            if (type == U_MATH_SYMBOL) {
                return true;
            }
            break;
        case SYMBOL_LETTERS:
            if (category != CATEGORY_LETTERS) {
                return false;
            }
            break;
        default:
            break;
    }
    return symbol_range_matches(point, filter);
}

/** Whether the scalar passes the collection, script extensions and category constraints. */
static bool scalar_matches(const CharactersSearch *search, uint32_t point) {
    if (search->category >= 0 && category_for(point) != search->category) {
        return false;
    }
    if (search->scope == CHARACTERS_SEARCH_SYMBOLS) {
        return symbol_matches(point, search->filter);
    }
    if (search->scope == CHARACTERS_SEARCH_SCRIPT && search->filter >= 0) {
        return characters_scalar_has_script(point, search->filter) &&
               characters_scalar_category(point) != U_UNASSIGNED;
    }
    if (search->scope == CHARACTERS_SEARCH_LANGUAGE && search->filter >= 0) {
        return characters_scalar_matches_scripts(point, search->scripts, COUNT(search->scripts));
    }
    if (search->scope == CHARACTERS_SEARCH_RADICAL || search->scope == CHARACTERS_SEARCH_STROKES) {
        return characters_unihan_group_matches(point, search->scope == CHARACTERS_SEARCH_RADICAL,
                                               search->filter);
    }
    return true;
}

/** Check cancellation with relaxed ordering; no other fields are shared during execution. */
static bool is_canceled(const CharactersSearch *search) {
    return atomic_load_explicit(&search->canceled, memory_order_relaxed);
}

/** Test whether a valid scalar or emoji identity is already in the result vector. */
static bool has_id(const CharactersSearch *search, uint32_t id) {
    return id < EMOJI_BASE + characters_unicode_emoji_count() &&
           (search->seen[id / 8] & (1u << (id % 8)));
}

/** Append a result with bounded geometric growth and suppress repeated catalog identities. */
static bool append_id(CharactersSearch *search, uint32_t id) {
    if (has_id(search, id)) {
        return true;
    }
    if (search->count == search->capacity) {
        if (search->capacity > SIZE_MAX / sizeof(*search->ids) / 2) {
            return false;
        }
        size_t capacity = search->capacity ? search->capacity * 2 : 1024;
        uint32_t *ids = realloc(search->ids, capacity * sizeof(*ids));
        if (!ids) {
            return false;
        }
        search->ids = ids;
        search->capacity = capacity;
    }
    search->ids[search->count++] = id;
    if (id < EMOJI_BASE + characters_unicode_emoji_count()) {
        search->seen[id / 8] |= (uint8_t)(1u << (id % 8));
    }
    return true;
}

/** Resolve scalar metadata on demand; empty and numeric queries avoid name allocations. */
static bool scalar_query_matches(uint32_t point, const struct Query *query) {
    if (matches_points(query, &point, 1)) {
        return true;
    }
    if (query->exact) {
        return false;
    }
    if (!query->term_count) {
        return true;
    }
    /* Explicit field-only searches need no allocation of scalar name metadata. */
    bool needs_metadata = false;
    for (size_t index = 0; index < query->term_count; index++) {
        if (characters_unihan_is_field_query(query->han_terms[index])) {
            if (!characters_unihan_matches(point, query->han_terms[index])) {
                return false;
            }
        } else {
            needs_metadata = true;
        }
    }
    if (!needs_metadata) {
        return true;
    }
    char *name = characters_search_name(point);
    char *aliases = characters_unicode_aliases(point);
    const char *block = characters_unicode_find_block(point);
    const char *script = characters_scalar_category(point) != U_UNASSIGNED ?
        characters_scalar_script_name((size_t)characters_scalar_script(point)) : NULL;
    const char *category = characters_search_category_name((size_t)category_for(point));
    bool matches = true;
    for (size_t index = 0; index < query->term_count; index++) {
        const char *term = query->terms[index];
        const char *han_term = query->han_terms[index];
        if (characters_unihan_is_field_query(han_term)) {
            continue;
        }
        if (!contains_term(name, term) && !contains_term(aliases, term) &&
            !contains_term(block, term) && !contains_term(script, term) &&
            !contains_term(category, term) && !characters_unihan_matches(point, han_term)) {
            matches = false;
            break;
        }
    }
    free(name);
    free(aliases);
    return matches;
}

/** Scan the selected scalar repertoire in numeric order, bounded by block or plane. */
static bool scan_scalars(CharactersSearch *search, const struct Query *query) {
    uint32_t start = 0;
    uint32_t end = SCALAR_LIMIT - 1;
    if (search->scope == CHARACTERS_SEARCH_BLOCK && search->filter >= 0) {
        start = characters_unicode_block_start((size_t)search->filter);
        end = characters_unicode_block_end((size_t)search->filter);
    }
    if ((search->scope == CHARACTERS_SEARCH_PLANE ||
         search->scope == CHARACTERS_SEARCH_UNASSIGNED) && search->filter >= 0) {
        start = (uint32_t)search->filter << 16;
        end = start + 0xFFFFu;
    }
    if (query->exact) {
        if (query->point_count != 1 || query->points[0] < start || query->points[0] > end) {
            return true;
        }
        start = end = query->points[0];
    }
    for (uint32_t point = start; point <= end; point++) {
        if ((point % CANCEL_INTERVAL == 0) && is_canceled(search)) {
            return true;
        }
        bool assigned = characters_unicode_is_assigned(point);
        bool included = search->scope == CHARACTERS_SEARCH_UNASSIGNED ?
            characters_unicode_is_scalar(point) && !assigned : assigned;
        if (included && scalar_matches(search, point) &&
            scalar_query_matches(point, query) && !append_id(search, point)) {
            return false;
        }
    }
    return true;
}

/** Scan only the explicitly supplied repertoire, preserving its order and deduplicating results. */
static bool scan_points(CharactersSearch *search, const struct Query *query) {
    for (size_t index = 0; index < search->point_count; index++) {
        if (index % CANCEL_INTERVAL == 0 && is_canceled(search)) {
            return true;
        }
        uint32_t point = search->points[index];
        if (scalar_matches(search, point) && scalar_query_matches(point, query) &&
            !append_id(search, point)) {
            return false;
        }
    }
    return true;
}

/** Search a catalog sequence with exact sequence matching or English metadata terms. */
static bool sequence_matches(const struct Query *query, const char *text, const char *name,
                             const char *group, uint32_t *points, size_t *count) {
    *count = decode_text(text, points);
    return *count && (matches_points(query, points, *count) ||
           matches_terms(query, name, NULL, NULL, NULL, group));
}

/** Match localized terms alongside the catalog name and category without changing exact queries. */
static bool emoji_query_matches(const CharactersSearch *search, const struct Query *query,
                                size_t entry, const char *name, const char *group) {
    if (query->exact) {
        return query->literal_letter && search->scope == CHARACTERS_SEARCH_EMOJI &&
               characters_emoji_keyword_matches(entry, search->locale, query->text);
    }
    for (size_t index = 0; index < query->term_count; index++) {
        const char *term = query->terms[index];
        if (!contains_term(name, term) && !contains_term(group, term) &&
            !characters_emoji_keyword_matches(entry, search->locale, term)) {
            return false;
        }
    }
    return true;
}

/** Scan emoji in semantic group order; global browsing merges single scalars. */
static bool scan_emoji(CharactersSearch *search, const struct Query *query) {
    for (size_t group = 0; group < characters_unicode_emoji_group_count(); group++) {
        if (search->scope == CHARACTERS_SEARCH_EMOJI && search->filter >= 0 &&
            group != (size_t)search->filter) {
            continue;
        }
        const char *group_name = characters_unicode_emoji_group_name(group);
        for (size_t index = 0; index < characters_unicode_emoji_group_size(group); index++) {
            if (is_canceled(search)) {
                return true;
            }
            size_t entry = characters_unicode_emoji_group_index(group, index);
            char *text = characters_unicode_emoji_text(entry);
            if (!text) {
                return false;
            }
            uint32_t points[MAX_SEQUENCE_POINTS];
            size_t count;
            bool match = sequence_matches(query, text, characters_unicode_emoji_label(entry),
                                          group_name, points, &count);
            if (!match && count) {
                match = emoji_query_matches(search, query, entry,
                    characters_unicode_emoji_label(entry), group_name);
            }
            free(text);
            if (!count) {
                return false;
            }
            if (match && search->category >= 0 &&
                (count != 1 || category_for(points[0]) != search->category)) {
                continue;
            }
            if (match && !append_id(search, search->scope == CHARACTERS_SEARCH_ALL && count == 1 ?
                                    points[0] : EMOJI_BASE + (uint32_t)entry)) {
                return false;
            }
        }
    }
    return true;
}

/** Add an exact noncatalog emoji identity without changing ordinary browse counts. */
static bool scan_emoji_alias(CharactersSearch *search, const struct Query *query) {
    if (is_canceled(search) || !query->exact || !query->point_count ||
        (search->scope == CHARACTERS_SEARCH_ALL && query->point_count == 1)) {
        return true;
    }
    char *text = characters_unicode_encode(query->points, query->point_count);
    size_t alias = characters_emoji_alias_index(text);
    free(text);
    if (alias == SIZE_MAX || alias >= NAMED_BASE - EMOJI_ALIAS_BASE) {
        return true;
    }
    if (search->category >= 0 && (query->point_count != 1 ||
        category_for(query->points[0]) != search->category)) {
        return true;
    }
    if (search->scope == CHARACTERS_SEARCH_EMOJI && search->filter >= 0) {
        size_t canonical = characters_emoji_alias_canonical(alias);
        size_t group = (size_t)search->filter;
        bool found = false;
        if (canonical == SIZE_MAX) {
            /* Standalone keycap bases belong to Symbols, not Flags or People. */
            const char *name = characters_unicode_emoji_group_name(group);
            found = name && strcmp(name, "Symbols") == 0;
        } else {
            for (size_t index = 0; index < characters_unicode_emoji_group_size(group); index++) {
                if (characters_unicode_emoji_group_index(group, index) == canonical) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            return true;
        }
    }
    return append_id(search, EMOJI_ALIAS_BASE + (uint32_t)alias);
}

/** Scan standardized named sequences, retaining their distinct catalog identity. */
static bool scan_named(CharactersSearch *search, const struct Query *query) {
    if (search->category >= 0) {
        return true;
    }
    for (size_t index = 0; index < characters_unicode_named_count(); index++) {
        if (is_canceled(search)) {
            return true;
        }
        char *text = characters_unicode_named_text(index);
        if (!text) {
            return false;
        }
        uint32_t points[MAX_SEQUENCE_POINTS];
        size_t count;
        bool match = sequence_matches(query, text, characters_unicode_named_name(index),
                                      NULL, points, &count);
        free(text);
        if (!count) {
            return false;
        }
        if (match && search->scope == CHARACTERS_SEARCH_ALL) {
            size_t emoji_index = characters_unicode_emoji_index(points, count);
            if (emoji_index != SIZE_MAX && has_id(search, EMOJI_BASE + (uint32_t)emoji_index)) {
                continue;
            }
            if (count == 1 && has_id(search, points[0])) {
                continue;
            }
        }
        if (match && !append_id(search, NAMED_BASE + (uint32_t)index)) {
            return false;
        }
    }
    return true;
}

CharactersSearch *characters_search_new(CharactersSearchScope scope, int filter,
                                        int category, const char *query) {
    CharactersSearch *search = calloc(1, sizeof(*search));
    if (!search) {
        return NULL;
    }
    search->scope = scope;
    search->filter = filter;
    search->category = category;
    search->status = CHARACTERS_SEARCH_INVALID;
    atomic_init(&search->canceled, false);
    if (query && strnlen(query, MAX_QUERY_BYTES + 1) <= MAX_QUERY_BYTES) {
        search->query = strdup(query);
        if (!search->query) {
            free(search);
            return NULL;
        }
    }
    return search;
}

CharactersSearch *characters_search_new_points(const uint32_t *points, size_t length,
                                               int category, const char *query) {
    if (length > SCALAR_LIMIT || (length && !points)) {
        return NULL;
    }
    CharactersSearch *search = characters_search_new(CHARACTERS_SEARCH_ALL, -1, category, query);
    if (!search) {
        return NULL;
    }
    search->points = malloc((length ? length : 1) * sizeof(*points));
    if (!search->points) {
        characters_search_free(search);
        return NULL;
    }
    for (size_t index = 0; index < length; index++) {
        if (!characters_unicode_is_scalar(points[index])) {
            characters_search_free(search);
            return NULL;
        }
        search->points[index] = points[index];
    }
    search->point_count = length;
    return search;
}

void characters_search_set_locale(CharactersSearch *search, const char *locale) {
    if (search && !search->started && locale &&
        strnlen(locale, sizeof(search->locale)) < sizeof(search->locale)) {
        strcpy(search->locale, locale);
    }
}

void characters_search_set_input_sources(CharactersSearch *search, const char *sources) {
    if (search && !search->started && sources &&
        strnlen(sources, sizeof(search->input_sources)) < sizeof(search->input_sources)) {
        strcpy(search->input_sources, sources);
    }
}

void characters_search_free(CharactersSearch *search) {
    if (search) {
        free(search->query);
        free(search->points);
        free(search->ids);
        free(search->seen);
        free(search);
    }
}

void characters_search_cancel(CharactersSearch *search) {
    if (search) {
        atomic_store_explicit(&search->canceled, true, memory_order_relaxed);
    }
}

CharactersSearchStatus characters_search_run(CharactersSearch *search) {
    if (!search || search->started) {
        return CHARACTERS_SEARCH_INVALID;
    }
    search->started = true;
    struct Query query = { 0 };
    if (search->scope < CHARACTERS_SEARCH_ALL || search->scope > CHARACTERS_SEARCH_UNASSIGNED ||
        search->filter < -1 || (search->filter >= 0 &&
        !characters_search_filter_name(search->scope, (size_t)search->filter)) ||
        search->category < -1 || search->category >= (int)COUNT(category_names) ||
        !parse_query(search->query, &query)) {
        return search->status;
    }
    if (is_canceled(search)) {
        search->status = CHARACTERS_SEARCH_CANCELED;
        return search->status;
    }
    if (search->scope == CHARACTERS_SEARCH_LANGUAGE && search->filter >= 0) {
        characters_language_scripts(search->filter, search->locale, search->input_sources,
                                    search->scripts);
    }
    size_t emoji_count = characters_unicode_emoji_count();
    if (emoji_count > EMOJI_ALIAS_BASE - EMOJI_BASE ||
        characters_emoji_alias_count() > NAMED_BASE - EMOJI_ALIAS_BASE ||
        characters_unicode_named_count() >= UINT32_MAX - NAMED_BASE) {
        search->status = CHARACTERS_SEARCH_FAILED;
        return search->status;
    }
    search->seen = calloc((EMOJI_BASE + emoji_count + 7) / 8, 1);
    if (!search->seen) {
        search->status = CHARACTERS_SEARCH_FAILED;
        return search->status;
    }
    bool success = true;
    if (search->points) {
        success = scan_points(search, &query);
    } else if (search->scope != CHARACTERS_SEARCH_EMOJI && search->scope != CHARACTERS_SEARCH_NAMED) {
        success = scan_scalars(search, &query);
    }
    if (!search->points && success && !query.han_field && !is_canceled(search) &&
        (search->scope == CHARACTERS_SEARCH_ALL || search->scope == CHARACTERS_SEARCH_EMOJI)) {
        success = scan_emoji(search, &query) && scan_emoji_alias(search, &query);
    }
    if (!search->points && success && !query.han_field && !is_canceled(search) &&
        (search->scope == CHARACTERS_SEARCH_ALL || search->scope == CHARACTERS_SEARCH_NAMED)) {
        success = scan_named(search, &query);
    }
    free(search->seen);
    search->seen = NULL;
    search->status = !success ? CHARACTERS_SEARCH_FAILED :
                     is_canceled(search) ? CHARACTERS_SEARCH_CANCELED : CHARACTERS_SEARCH_OK;
    if (search->status != CHARACTERS_SEARCH_OK) {
        free(search->ids);
        search->ids = NULL;
        search->count = search->capacity = 0;
    }
    return search->status;
}

size_t characters_search_count(const CharactersSearch *search) {
    return search && search->status == CHARACTERS_SEARCH_OK ? search->count : 0;
}

uint32_t characters_search_id(const CharactersSearch *search, size_t index) {
    return index < characters_search_count(search) ? search->ids[index] : UINT32_MAX;
}

size_t characters_search_filter_count(CharactersSearchScope scope) {
    switch (scope) {
        case CHARACTERS_SEARCH_EMOJI:
            return characters_unicode_emoji_group_count();
        case CHARACTERS_SEARCH_SYMBOLS:
            return COUNT(symbol_names);
        case CHARACTERS_SEARCH_SCRIPT:
            return characters_scalar_script_count();
        case CHARACTERS_SEARCH_BLOCK:
            return characters_unicode_block_count();
        case CHARACTERS_SEARCH_LANGUAGE:
            return characters_language_count();
        case CHARACTERS_SEARCH_RADICAL:
            return characters_unihan_group_count(true);
        case CHARACTERS_SEARCH_STROKES:
            return characters_unihan_group_count(false);
        case CHARACTERS_SEARCH_PLANE:
        case CHARACTERS_SEARCH_UNASSIGNED:
            return COUNT(plane_names);
        default:
            return 0;
    }
}

const char *characters_search_filter_name(CharactersSearchScope scope, size_t index) {
    if (index >= characters_search_filter_count(scope)) {
        return NULL;
    }
    switch (scope) {
        case CHARACTERS_SEARCH_EMOJI:
            return characters_unicode_emoji_group_name(index);
        case CHARACTERS_SEARCH_SYMBOLS:
            return symbol_names[index];
        case CHARACTERS_SEARCH_SCRIPT:
            return characters_scalar_script_name(index);
        case CHARACTERS_SEARCH_BLOCK:
            return characters_unicode_block_name(index);
        case CHARACTERS_SEARCH_LANGUAGE:
            return characters_language_name(index);
        case CHARACTERS_SEARCH_RADICAL:
            return characters_unihan_group_name(true, index);
        case CHARACTERS_SEARCH_STROKES:
            return characters_unihan_group_name(false, index);
        case CHARACTERS_SEARCH_PLANE:
        case CHARACTERS_SEARCH_UNASSIGNED:
            return plane_names[index];
        default:
            return NULL;
    }
}

size_t characters_search_category_count(void) {
    return COUNT(category_names);
}

const char *characters_search_category_name(size_t index) {
    return index < COUNT(category_names) ? category_names[index] : NULL;
}

char *characters_search_text(uint32_t id) {
    if (id < SCALAR_LIMIT) {
        return characters_unicode_is_scalar(id) ? characters_unicode_encode(&id, 1) : NULL;
    }
    if (id < EMOJI_ALIAS_BASE) {
        return characters_unicode_emoji_text(id - EMOJI_BASE);
    }
    if (id < NAMED_BASE) {
        const char *text = characters_emoji_alias_text(id - EMOJI_ALIAS_BASE);
        return text ? strdup(text) : NULL;
    }
    return characters_unicode_named_text(id - NAMED_BASE);
}

char *characters_search_display(uint32_t id) {
    if (id < SCALAR_LIMIT && characters_unicode_is_scalar(id)) {
        int category = category_for(id);
        if (!characters_unicode_is_assigned(id)) {
            return strdup("·");
        }
        if (category == CATEGORY_SEPARATORS || category == CATEGORY_CONTROLS ||
            id == 0x034F || id == 0x115F || id == 0x1160 || id == 0x3164 || id == 0xFFA0) {
            return strdup("·");
        }
        if (category == CATEGORY_MARKS) {
            uint32_t points[] = { 0x25CC, id };
            return characters_unicode_encode(points, COUNT(points));
        }
    }
    return characters_search_text(id);
}

char *characters_search_name(uint32_t id) {
    if (id < SCALAR_LIMIT) {
        if (!characters_unicode_is_scalar(id)) {
            return NULL;
        }
        if (!characters_unicode_is_assigned(id)) {
            return strdup(u_hasBinaryProperty((UChar32)id, UCHAR_NONCHARACTER_CODE_POINT) ?
                          N_("Noncharacter") : N_("Unassigned Character"));
        }
        char *name = characters_unicode_name(id);
        if (name) {
            return name;
        }
        name = characters_unicode_aliases(id);
        if (name) {
            return name;
        }
        return strdup(category_for(id) == CATEGORY_PRIVATE_USE ?
                      "Private Use Character" : "Unnamed Character");
    }
    if (id >= EMOJI_ALIAS_BASE && id < NAMED_BASE) {
        return characters_emoji_alias_name(id - EMOJI_ALIAS_BASE);
    }
    const char *name = id < EMOJI_ALIAS_BASE ? characters_unicode_emoji_label(id - EMOJI_BASE) :
                       characters_unicode_named_name(id - NAMED_BASE);
    return name ? strdup(name) : NULL;
}

char *characters_search_code(uint32_t id) {
    uint32_t points[MAX_SEQUENCE_POINTS];
    size_t count;
    if (id < SCALAR_LIMIT && characters_unicode_is_scalar(id)) {
        points[0] = id;
        count = 1;
    } else {
        char *text = characters_search_text(id);
        count = decode_text(text, points);
        free(text);
    }
    if (!count) {
        return NULL;
    }
    char code[MAX_SEQUENCE_POINTS * 9 + 1];
    size_t offset = 0;
    for (size_t index = 0; index < count; index++) {
        offset += (size_t)snprintf(code + offset, sizeof(code) - offset, "%sU+%04X",
                                  index ? " " : "", (unsigned)points[index]);
    }
    return strdup(code);
}

size_t characters_search_decode_code(const char *code, uint32_t *points) {
    if (!code || !points || strnlen(code, MAX_QUERY_BYTES + 1) > MAX_QUERY_BYTES) {
        return 0;
    }
    while (is_space(*code)) {
        code++;
    }
    struct Query query = { 0 };
    if (!parse_explicit(code, &query)) {
        return 0;
    }
    memcpy(points, query.points, query.point_count * sizeof(*points));
    return query.point_count;
}
