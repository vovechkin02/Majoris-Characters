#include "unicode-engine.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/ustring.h>
#include <unicode/utf16.h>
#include <unicode/utf8.h>

#include "emoji-catalog.h"
#include "normalization.h"
#include "scalar-properties.h"
#include "unihan.h"

/* GNOME generated headers use GLib's unsigned 32-bit scalar spelling. */
typedef uint32_t gunichar;
#include "vendor/gnome/blocks.h"
#include "vendor/gnome/confusables.h"
#include "vendor/gnome/emoji.h"
#include "vendor/gnome/names.h"
#include "generated/catalog-version.h"

#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define MAX_SEQUENCE_LENGTH CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH
#define MAX_TEXT_BYTES (1024 * 1024)

/** An official alias and its UCD classification. */
struct AliasEntry {
    uint32_t point;
    const char *name;
    const char *kind;
};

/** An immutable standardized sequence, distinct from emoji display labels. */
struct NamedEntry {
    const char *name;
    uint32_t points[MAX_SEQUENCE_LENGTH];
    size_t length;
};

#include "generated/supplement.h"

/** One semantic emoji collection in upstream display order. */
struct EmojiGroup {
    const char *name;
    const size_t *indices;
    size_t count;
};

#define N_(message) (message)
#define GROUP(name, array) { name, array, COUNT(array) }
static const struct EmojiGroup emoji_groups[] = {
    GROUP(N_("Smileys & Emotion"), emoji_smileys_characters),
    GROUP(N_("People & Body"), emoji_people_characters),
    GROUP(N_("Animals & Nature"), emoji_animals_characters),
    GROUP(N_("Food & Drink"), emoji_food_characters),
    GROUP(N_("Travel & Places"), emoji_travel_characters),
    GROUP(N_("Activities"), emoji_activities_characters),
    GROUP(N_("Objects"), emoji_objects_characters),
    GROUP(N_("Symbols"), emoji_symbols_characters),
    GROUP(N_("Flags"), emoji_flags_characters),
};

/** A bounded sequence used for lexicographic catalog lookup. */
struct Sequence {
    uint32_t points[MAX_SEQUENCE_LENGTH];
    size_t length;
};

/** A growable native scalar candidate vector; failure is sticky. */
struct ScalarCandidates {
    uint32_t *items;
    size_t count;
    size_t capacity;
    bool failed;
};

/** Compare a scalar against an inclusive block range for binary search. */
static int compare_block(const void *key, const void *item) {
    uint32_t point = *(const uint32_t *)key;
    const struct Block *block = item;
    return (point > block->end) - (point < block->start);
}

/** Compare a scalar against the sorted upstream explicit name table. */
static int compare_name(const void *key, const void *item) {
    uint32_t point = *(const uint32_t *)key;
    const struct CharacterName *entry = item;
    return (point > entry->uc) - (point < entry->uc);
}

/** Compare full sequences, including length, without conflating NUL with padding. */
static int compare_emoji(const void *key, const void *item) {
    const struct Sequence *sequence = key;
    const struct EmojiCharacter *entry = item;
    size_t entry_length = (size_t)entry->length;
    size_t shared = sequence->length < entry_length ? sequence->length : entry_length;
    for (size_t index = 0; index < shared; index++) {
        if (sequence->points[index] != entry->uc[index]) {
            return sequence->points[index] < entry->uc[index] ? -1 : 1;
        }
    }
    return (sequence->length > entry_length) - (sequence->length < entry_length);
}

/** Return a borrowed explicit upstream name entry, if present. */
static const struct CharacterName *find_name(uint32_t point) {
    return bsearch(&point, character_names, COUNT(character_names),
                   sizeof(character_names[0]), compare_name);
}

/** Validate a numeric sequence before passing it to ICU or a catalog lookup. */
static bool valid_sequence(const uint32_t *points, size_t length) {
    if (!points || length == 0 || length > MAX_SEQUENCE_LENGTH) {
        return false;
    }
    for (size_t index = 0; index < length; index++) {
        if (!characters_unicode_is_scalar(points[index])) {
            return false;
        }
    }
    return true;
}

/** Locate an exact fully qualified emoji sequence, preserving modifiers and joiners. */
static const struct EmojiCharacter *find_emoji(const uint32_t *points, size_t length) {
    if (!valid_sequence(points, length) || length > EMOJI_SEQUENCE_LENGTH) {
        return NULL;
    }
    struct Sequence key = { .length = length };
    memcpy(key.points, points, length * sizeof(*points));
    return bsearch(&key, emoji_characters, COUNT(emoji_characters),
                   sizeof(emoji_characters[0]), compare_emoji);
}

/** Return whether the bundled snapshot assigns the scalar. */
static bool has_properties(uint32_t point) {
    return characters_scalar_category(point) != U_UNASSIGNED;
}

/** Convert a valid bounded UTF-16 buffer to newly allocated NUL-terminated UTF-8. */
static char *from_utf16(const UChar *text, int32_t length) {
    UErrorCode error = U_ZERO_ERROR;
    int32_t bytes = 0;
    u_strToUTF8(NULL, 0, &bytes, text, length, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error)) {
        return NULL;
    }
    char *result = malloc((size_t)bytes + 1);
    if (!result) {
        return NULL;
    }
    error = U_ZERO_ERROR;
    u_strToUTF8(result, bytes + 1, NULL, text, length, &error);
    if (U_FAILURE(error)) {
        free(result);
        return NULL;
    }
    return result;
}

/** Return the first canonical decomposition scalar, or the original scalar. */
static uint32_t canonical_base(uint32_t point) {
    UErrorCode error = U_ZERO_ERROR;
    const UNormalizer2 *nfd = unorm2_getNFDInstance(&error);
    UChar buffer[64];
    if (U_FAILURE(error)) {
        return point;
    }
    int32_t length = unorm2_getDecomposition(nfd, (UChar32)point, buffer, COUNT(buffer), &error);
    if (U_FAILURE(error) || length <= 0) {
        return point;
    }
    int32_t offset = 0;
    UChar32 base;
    U16_NEXT(buffer, offset, length, base);
    return (uint32_t)base;
}

/** Append a valid non-NUL scalar without encoding or applying presentation policy. */
static void append_scalar(struct ScalarCandidates *list, uint32_t point) {
    if (list->failed || !point || !characters_unicode_is_scalar(point)) {
        return;
    }
    if (list->count == list->capacity) {
        if (list->capacity > INT_MAX / 2) {
            list->failed = true;
            return;
        }
        size_t capacity = list->capacity ? list->capacity * 2 : 16;
        uint32_t *items = realloc(list->items, capacity * sizeof(*items));
        if (!items) {
            list->failed = true;
            return;
        }
        list->items = items;
        list->capacity = capacity;
    }
    list->items[list->count++] = point;
}

/** Compare a scalar against the GNOME confusable-class index. */
static int compare_confusable(const void *key, const void *item) {
    uint32_t point = *(const uint32_t *)key;
    const struct ConfusableCharacterClass *entry = item;
    return (point > entry->uc) - (point < entry->uc);
}

/** Adapt GNOME's single-code-point confusable class lookup. Not a security test. */
static void append_confusables(struct ScalarCandidates *list, uint32_t point) {
    const struct ConfusableCharacterClass *entry = bsearch(
        &point, confusable_character_classes, COUNT(confusable_character_classes),
        sizeof(confusable_character_classes[0]), compare_confusable);
    if (!entry || entry->index >= COUNT(confusable_classes)) {
        return;
    }
    const struct ConfusableClass *group = &confusable_classes[entry->index];
    for (size_t index = group->offset; index < (size_t)group->offset + group->length; index++) {
        if (index < COUNT(confusable_characters)) {
            append_scalar(list, confusable_characters[index]);
        }
    }
}

/** Add the new simple case pairs defined by Unicode 17 before consulting ICU 16. */
static void append_case_variants(struct ScalarCandidates *list, uint32_t point) {
    if (point >= 0x16EA0 && point <= 0x16EB8) {
        append_scalar(list, point + 0x1B);
    } else if (point >= 0x16EBB && point <= 0x16ED3) {
        append_scalar(list, point - 0x1B);
    } else if ((point >= 0xA7CE && point <= 0xA7CF) ||
               (point >= 0xA7D2 && point <= 0xA7D5)) {
        append_scalar(list, point ^ 1u);
    } else {
        append_scalar(list, (uint32_t)u_toupper((UChar32)point));
        append_scalar(list, (uint32_t)u_tolower((UChar32)point));
        append_scalar(list, (uint32_t)u_totitle((UChar32)point));
    }
}

/** Find reverse canonical variants in the same families used by GNOME Characters. */
static void append_accent_variants(struct ScalarCandidates *list, uint32_t point) {
    const char *script = characters_scalar_script_name((size_t)characters_scalar_script(point));
    if (!script) {
        return;
    }
    static const uint32_t latin[] = { 0x0000, 0x0080, 0x0100, 0x0180, 0x1E00 };
    static const uint32_t hiragana[] = { 0x3040 };
    static const uint32_t katakana[] = { 0x30A0, 0x31F0 };
    const uint32_t *starts = NULL;
    size_t count = 0;
    if (strcmp(script, "Latin") == 0) {
        starts = latin;
        count = COUNT(latin);
    } else if (strcmp(script, "Hiragana") == 0) {
        starts = hiragana;
        count = COUNT(hiragana);
    } else if (strcmp(script, "Katakana") == 0) {
        starts = katakana;
        count = COUNT(katakana);
    }
    uint32_t base = canonical_base(point);
    append_scalar(list, base);
    for (size_t index = 0; index < count; index++) {
        const struct Block *block = bsearch(&starts[index], all_blocks, COUNT(all_blocks),
                                            sizeof(all_blocks[0]), compare_block);
        if (!block) {
            continue;
        }
        for (uint32_t candidate = block->start; candidate <= block->end; candidate++) {
            if (has_properties(candidate) && canonical_base(candidate) == base) {
                append_scalar(list, candidate);
            }
        }
    }
}

/** Add canonical decomposition components, including Hangul Jamo. */
static void append_decomposition(struct ScalarCandidates *list, uint32_t point) {
    UErrorCode error = U_ZERO_ERROR;
    const UNormalizer2 *nfd = unorm2_getNFDInstance(&error);
    UChar buffer[64];
    if (U_FAILURE(error)) {
        return;
    }
    int32_t length = unorm2_getDecomposition(nfd, (UChar32)point, buffer, COUNT(buffer), &error);
    if (U_FAILURE(error) || length <= 0) {
        return;
    }
    for (int32_t offset = 0; offset < length;) {
        UChar32 component;
        U16_NEXT(buffer, offset, length, component);
        append_scalar(list, (uint32_t)component);
    }
}

bool characters_unicode_is_scalar(uint32_t point) {
    return point <= 0x10FFFF && !(point >= 0xD800 && point <= 0xDFFF);
}

bool characters_unicode_is_assigned(uint32_t point) {
    return has_properties(point);
}

char *characters_unicode_properties_version(void) {
    return strdup(characters_scalar_version());
}

const char *characters_unicode_catalog_version(void) {
    return CATALOG_VERSION;
}

const char *characters_unicode_supplement_version(void) {
    return SUPPLEMENT_VERSION;
}

char *characters_unicode_name(uint32_t point) {
    if (!characters_unicode_is_assigned(point)) {
        return NULL;
    }
    char name[256];
    UErrorCode error = U_ZERO_ERROR;
    int32_t length = u_charName((UChar32)point, U_UNICODE_CHAR_NAME, name, sizeof(name), &error);
    if (U_SUCCESS(error) && length > 0) {
        return strdup(name);
    }
    const struct CharacterName *entry = find_name(point);
    if (entry) {
        return strdup(entry->name);
    }
    /* Canonical names of Unicode 17 single-scalar emoji omitted by GNOME's name generator. */
    switch (point) {
        case 0x1F6D8: return strdup("LANDSLIDE");
        case 0x1FA8A: return strdup("TROMBONE");
        case 0x1FA8E: return strdup("TREASURE CHEST");
        case 0x1FAC8: return strdup("HAIRY CREATURE");
        case 0x1FACD: return strdup("ORCA");
        case 0x1FAEA: return strdup("DISTORTED FACE");
        case 0x1FAEF: return strdup("FIGHT CLOUD");
        default: return characters_scalar_algorithmic_name(point);
    }
}

char *characters_unicode_aliases(uint32_t point) {
    if (!characters_unicode_is_scalar(point)) {
        return NULL;
    }
    size_t low = 0;
    size_t high = COUNT(aliases);
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (aliases[middle].point < point) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    size_t end = low;
    size_t bytes = 0;
    while (end < COUNT(aliases) && aliases[end].point == point) {
        bytes += strlen(aliases[end++].name) + 2;
    }
    if (!bytes) {
        char name[256];
        UErrorCode error = U_ZERO_ERROR;
        int32_t length = u_charName((UChar32)point, U_CHAR_NAME_ALIAS, name, sizeof(name), &error);
        return U_SUCCESS(error) && length > 0 ? strdup(name) : NULL;
    }
    char *result = malloc(bytes);
    if (!result) {
        return NULL;
    }
    char *cursor = result;
    for (size_t index = low; index < end; index++) {
        if (index != low) {
            *cursor++ = ';';
            *cursor++ = ' ';
        }
        size_t length = strlen(aliases[index].name);
        memcpy(cursor, aliases[index].name, length);
        cursor += length;
    }
    *cursor = '\0';
    return result;
}

const char *characters_unicode_unihan_version(void) {
    return characters_unihan_version();
}

/** Resolve an ISO script code through the same snapshot used by browsing. */
const char *characters_unicode_script_name(const char *code) {
    int index = characters_scalar_script_index(code);
    return index < 0 ? NULL : characters_scalar_script_name((size_t)index);
}

char *characters_unicode_property(uint32_t point, const char *property) {
    if (!property || !characters_unicode_is_scalar(point)) {
        return NULL;
    }
    if (strcmp(property, "Plane") == 0) {
        char text[8];
        snprintf(text, sizeof(text), "%u", (unsigned)(point >> 16));
        return strdup(text);
    }
    char *value = characters_scalar_property(point, property);
    return value ? value : characters_unihan_property(point, property);
}

size_t characters_unicode_block_count(void) {
    return COUNT(all_blocks);
}

const char *characters_unicode_block_name(size_t index) {
    return index < COUNT(all_blocks) ? all_blocks[index].name : NULL;
}

uint32_t characters_unicode_block_start(size_t index) {
    return index < COUNT(all_blocks) ? all_blocks[index].start : UINT32_MAX;
}

uint32_t characters_unicode_block_end(size_t index) {
    return index < COUNT(all_blocks) ? all_blocks[index].end : UINT32_MAX;
}

const char *characters_unicode_find_block(uint32_t point) {
    const struct Block *block = bsearch(&point, all_blocks, COUNT(all_blocks),
                                        sizeof(all_blocks[0]), compare_block);
    return block ? block->name : NULL;
}

char *characters_unicode_encode(const uint32_t *points, size_t length) {
    if (!valid_sequence(points, length)) {
        return NULL;
    }
    uint8_t text[MAX_SEQUENCE_LENGTH * 4 + 1];
    int32_t offset = 0;
    for (size_t index = 0; index < length; index++) {
        if (points[index] == 0) {
            return NULL;
        }
        U8_APPEND_UNSAFE(text, offset, points[index]);
    }
    text[offset] = 0;
    return strdup((const char *)text);
}

char *characters_unicode_sequence_name(const uint32_t *points, size_t length) {
    if (!valid_sequence(points, length)) {
        return NULL;
    }
    if (length == 1) {
        return characters_unicode_name(points[0]);
    }
    for (size_t index = 0; index < COUNT(named_sequences); index++) {
        const struct NamedEntry *entry = &named_sequences[index];
        if (entry->length == length && !memcmp(entry->points, points, length * sizeof(*points))) {
            return strdup(entry->name);
        }
    }
    const struct EmojiCharacter *emoji = find_emoji(points, length);
    if (emoji) {
        return strdup(emoji->name);
    }
    char *text = characters_unicode_encode(points, length);
    size_t alias = characters_emoji_alias_index(text);
    free(text);
    return characters_emoji_alias_name(alias);
}

char *characters_unicode_normalize(const char *text, const char *form) {
    if (!text || strnlen(text, MAX_TEXT_BYTES + 1) > MAX_TEXT_BYTES) {
        return NULL;
    }
    UErrorCode error = U_ZERO_ERROR;
    int32_t length = 0;
    u_strFromUTF8(NULL, 0, &length, text, -1, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error)) {
        return NULL;
    }
    UChar *input = malloc(((size_t)length + 1) * sizeof(*input));
    if (!input) {
        return NULL;
    }
    error = U_ZERO_ERROR;
    u_strFromUTF8(input, length + 1, NULL, text, -1, &error);
    if (U_FAILURE(error)) {
        free(input);
        return NULL;
    }
    int32_t needed = 0;
    UChar *output = characters_normalize_utf16(input, length, form, &needed);
    free(input);
    char *result = output ? from_utf16(output, needed) : NULL;
    free(output);
    return result;
}

size_t characters_unicode_emoji_group_count(void) {
    return COUNT(emoji_groups);
}

const char *characters_unicode_emoji_group_name(size_t group) {
    return group < COUNT(emoji_groups) ? emoji_groups[group].name : NULL;
}

size_t characters_unicode_emoji_group_size(size_t group) {
    return group < COUNT(emoji_groups) ? emoji_groups[group].count : 0;
}

size_t characters_unicode_emoji_group_index(size_t group, size_t index) {
    if (group >= COUNT(emoji_groups) || index >= emoji_groups[group].count) {
        return SIZE_MAX;
    }
    return emoji_groups[group].indices[index];
}

size_t characters_unicode_emoji_index(const uint32_t *points, size_t length) {
    const struct EmojiCharacter *entry = find_emoji(points, length);
    return entry ? (size_t)(entry - emoji_characters) : SIZE_MAX;
}

size_t characters_unicode_emoji_count(void) {
    return COUNT(emoji_characters);
}

char *characters_unicode_emoji_text(size_t index) {
    if (index >= COUNT(emoji_characters)) {
        return NULL;
    }
    return characters_unicode_encode(emoji_characters[index].uc,
                                     (size_t)emoji_characters[index].length);
}

const char *characters_unicode_emoji_label(size_t index) {
    return index < COUNT(emoji_characters) ? emoji_characters[index].name : NULL;
}

size_t characters_unicode_named_count(void) {
    return COUNT(named_sequences);
}

const char *characters_unicode_named_name(size_t index) {
    return index < COUNT(named_sequences) ? named_sequences[index].name : NULL;
}

char *characters_unicode_named_text(size_t index) {
    if (index >= COUNT(named_sequences)) {
        return NULL;
    }
    return characters_unicode_encode(named_sequences[index].points, named_sequences[index].length);
}

uint32_t *characters_unicode_related_scalars(uint32_t point, int *count) {
    if (!count) {
        return NULL;
    }
    *count = 0;
    if (!characters_unicode_is_scalar(point)) {
        return NULL;
    }
    struct ScalarCandidates list = { 0 };
    if (point && has_properties(point)) {
        append_case_variants(&list, point);
        append_scalar(&list, (uint32_t)u_charMirror((UChar32)point));
        append_decomposition(&list, point);
        append_accent_variants(&list, point);
    }
    if (point) {
        append_confusables(&list, point);
    }
    if (list.failed) {
        free(list.items);
        return NULL;
    }
    if (!list.items) {
        list.items = calloc(1, sizeof(*list.items));
    }
    if (list.items) {
        *count = (int)list.count;
    }
    return list.items;
}
