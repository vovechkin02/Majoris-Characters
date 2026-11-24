#include "unicode-engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/uscript.h>
#include <unicode/ustring.h>
#include <unicode/utf16.h>
#include <unicode/utf8.h>

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

#define GROUP(name, array) { name, array, COUNT(array) }
static const struct EmojiGroup emoji_groups[] = {
    GROUP("Smileys & Emotion", emoji_smileys_characters),
    GROUP("People & Body", emoji_people_characters),
    GROUP("Animals & Nature", emoji_animals_characters),
    GROUP("Food & Drink", emoji_food_characters),
    GROUP("Travel & Places", emoji_travel_characters),
    GROUP("Activities", emoji_activities_characters),
    GROUP("Objects", emoji_objects_characters),
    GROUP("Symbols", emoji_symbols_characters),
    GROUP("Flags", emoji_flags_characters),
};

/** A bounded sequence used for lexicographic catalog lookup. */
struct Sequence {
    uint32_t points[MAX_SEQUENCE_LENGTH];
    size_t length;
};

/** A growable, owned collection of UTF-8 alternatives; failure is sticky. */
struct RelatedList {
    char **items;
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

/** Return whether ICU itself recognizes the scalar's general category. */
static bool has_properties(uint32_t point) {
    return characters_unicode_is_scalar(point) && u_charType((UChar32)point) != U_UNASSIGNED;
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

/** Resolve a normalization form without accepting arbitrary ICU identifiers. */
static const UNormalizer2 *normalizer(const char *form, UErrorCode *error) {
    if (!form) {
        return NULL;
    }
    if (strcmp(form, "NFC") == 0) {
        return unorm2_getNFCInstance(error);
    }
    if (strcmp(form, "NFD") == 0) {
        return unorm2_getNFDInstance(error);
    }
    if (strcmp(form, "NFKC") == 0) {
        return unorm2_getNFKCInstance(error);
    }
    if (strcmp(form, "NFKD") == 0) {
        return unorm2_getNFKDInstance(error);
    }
    return NULL;
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

/** Append one allocated alternative, growing the vector and retaining a NULL terminator. */
static void append_related(struct RelatedList *list, const uint32_t *points, size_t length) {
    if (list->failed || !valid_sequence(points, length)) {
        return;
    }
    for (size_t index = 0; index < length; index++) {
        if (!points[index]) {
            return;
        }
    }
    char *text = characters_unicode_encode(points, length);
    if (!text) {
        list->failed = true;
        return;
    }
    if (list->count + 1 >= list->capacity) {
        size_t capacity = list->capacity ? list->capacity * 2 : 16;
        char **items = realloc(list->items, capacity * sizeof(*items));
        if (!items) {
            free(text);
            list->failed = true;
            return;
        }
        list->items = items;
        list->capacity = capacity;
    }
    list->items[list->count++] = text;
    list->items[list->count] = NULL;
}

/** Append a single non-NUL scalar as an alternative. */
static void append_scalar(struct RelatedList *list, uint32_t point) {
    append_related(list, &point, 1);
}

/** Compare a scalar against the GNOME confusable-class index. */
static int compare_confusable(const void *key, const void *item) {
    uint32_t point = *(const uint32_t *)key;
    const struct ConfusableCharacterClass *entry = item;
    return (point > entry->uc) - (point < entry->uc);
}

/** Adapt GNOME's single-code-point confusable class lookup. Not a security test. */
static void append_confusables(struct RelatedList *list, uint32_t point) {
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

/** Find reverse canonical variants in the same families used by GNOME Characters. */
static void append_accent_variants(struct RelatedList *list, uint32_t point) {
    UErrorCode error = U_ZERO_ERROR;
    UScriptCode script = uscript_getScript((UChar32)point, &error);
    if (U_FAILURE(error)) {
        return;
    }
    static const uint32_t latin[] = { 0x0000, 0x0080, 0x0100, 0x0180, 0x1E00 };
    static const uint32_t hiragana[] = { 0x3040 };
    static const uint32_t katakana[] = { 0x30A0, 0x31F0 };
    const uint32_t *starts = NULL;
    size_t count = 0;
    if (script == USCRIPT_LATIN) {
        starts = latin;
        count = COUNT(latin);
    } else if (script == USCRIPT_HIRAGANA) {
        starts = hiragana;
        count = COUNT(hiragana);
    } else if (script == USCRIPT_KATAKANA) {
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
static void append_decomposition(struct RelatedList *list, uint32_t point) {
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

/** Compare UTF-8 strings in scalar order; valid UTF-8 preserves lexicographic ordering. */
static int compare_text(const void *left, const void *right) {
    return strcmp(*(char *const *)left, *(char *const *)right);
}

/** Release a failed result or return a sorted unique vector excluding the input. */
static char **finish_related(struct RelatedList *list, const char *original) {
    if (list->failed) {
        for (size_t index = 0; index < list->count; index++) {
            free(list->items[index]);
        }
        free(list->items);
        return NULL;
    }
    if (!list->items) {
        return calloc(1, sizeof(char *));
    }
    qsort(list->items, list->count, sizeof(char *), compare_text);
    size_t written = 0;
    for (size_t index = 0; index < list->count; index++) {
        char *item = list->items[index];
        if ((original && strcmp(original, item) == 0) ||
            (written && strcmp(list->items[written - 1], item) == 0)) {
            free(item);
        } else {
            list->items[written++] = item;
        }
    }
    list->items[written] = NULL;
    return list->items;
}

bool characters_unicode_is_scalar(uint32_t point) {
    return point <= 0x10FFFF && !(point >= 0xD800 && point <= 0xDFFF);
}

bool characters_unicode_is_assigned(uint32_t point) {
    return characters_unicode_is_scalar(point) &&
           (has_properties(point) || find_name(point) || find_emoji(&point, 1));
}

char *characters_unicode_properties_version(void) {
    UVersionInfo version;
    char text[U_MAX_VERSION_STRING_LENGTH];
    u_getUnicodeVersion(version);
    u_versionToString(version, text);
    return strdup(text);
}

const char *characters_unicode_catalog_version(void) {
    return CATALOG_VERSION;
}

const char *characters_unicode_supplement_version(void) {
    return SUPPLEMENT_VERSION;
}

char *characters_unicode_name(uint32_t point) {
    if (!characters_unicode_is_scalar(point)) {
        return NULL;
    }
    char name[256];
    UErrorCode error = U_ZERO_ERROR;
    int32_t length = u_charName((UChar32)point, U_UNICODE_CHAR_NAME, name, sizeof(name), &error);
    if (U_SUCCESS(error) && length > 0) {
        return strdup(name);
    }
    const struct CharacterName *entry = find_name(point);
    return entry ? strdup(entry->name) : NULL;
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

char *characters_unicode_property(uint32_t point, const char *property) {
    if (!property || !characters_unicode_is_scalar(point)) {
        return NULL;
    }
    if (strcmp(property, "Plane") == 0) {
        char text[8];
        snprintf(text, sizeof(text), "%u", (unsigned)(point >> 16));
        return strdup(text);
    }
    if (!has_properties(point)) {
        return NULL;
    }
    UChar32 value = (UChar32)point;
    if (strcmp(property, "Age") == 0) {
        UVersionInfo version;
        char text[U_MAX_VERSION_STRING_LENGTH];
        u_charAge(value, version);
        u_versionToString(version, text);
        return strdup(text);
    }
    if (strcmp(property, "Script_Extensions") == 0) {
        UScriptCode scripts[USCRIPT_CODE_LIMIT];
        UErrorCode error = U_ZERO_ERROR;
        int32_t count = uscript_getScriptExtensions(value, scripts, COUNT(scripts), &error);
        if (U_FAILURE(error) || count <= 0) {
            return NULL;
        }
        char *result = calloc((size_t)count * 5, 1);
        if (!result) {
            return NULL;
        }
        for (int32_t index = 0; index < count; index++) {
            if (index) {
                strcat(result, " ");
            }
            strcat(result, uscript_getShortName(scripts[index]));
        }
        return result;
    }
    if (strcmp(property, "Canonical_Combining_Class") == 0) {
        char text[4];
        snprintf(text, sizeof(text), "%u", (unsigned)u_getCombiningClass(value));
        return strdup(text);
    }
    UProperty key = u_getPropertyEnum(property);
    if (key != UCHAR_GENERAL_CATEGORY && key != UCHAR_SCRIPT && key != UCHAR_BIDI_CLASS &&
        key != UCHAR_BIDI_MIRRORED && key != UCHAR_DECOMPOSITION_TYPE &&
        key != UCHAR_EAST_ASIAN_WIDTH && key != UCHAR_EMOJI &&
        key != UCHAR_EMOJI_PRESENTATION && key != UCHAR_EMOJI_MODIFIER) {
        return NULL;
    }
    const char *name = u_getPropertyValueName(key, u_getIntPropertyValue(value, key),
                                               U_SHORT_PROPERTY_NAME);
    return name ? strdup(name) : NULL;
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
    return emoji ? strdup(emoji->name) : NULL;
}

char *characters_unicode_normalize(const char *text, const char *form) {
    if (!text || strnlen(text, MAX_TEXT_BYTES + 1) > MAX_TEXT_BYTES) {
        return NULL;
    }
    UErrorCode error = U_ZERO_ERROR;
    const UNormalizer2 *instance = normalizer(form, &error);
    if (!instance || U_FAILURE(error)) {
        return NULL;
    }
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
    int32_t needed = unorm2_normalize(instance, input, length, NULL, 0, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error)) {
        free(input);
        return NULL;
    }
    UChar *output = malloc(((size_t)needed + 1) * sizeof(*output));
    if (!output) {
        free(input);
        return NULL;
    }
    error = U_ZERO_ERROR;
    unorm2_normalize(instance, input, length, output, needed + 1, &error);
    free(input);
    char *result = U_SUCCESS(error) ? from_utf16(output, needed) : NULL;
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

char **characters_unicode_related(const uint32_t *points, size_t length) {
    if (!valid_sequence(points, length)) {
        return NULL;
    }
    for (size_t index = 0; index < length; index++) {
        if (points[index] == 0) {
            return length == 1 ? calloc(1, sizeof(char *)) : NULL;
        }
    }
    struct RelatedList list = { 0 };
    if (length > 1) {
        for (size_t index = 0; index < length;) {
            size_t part = length - index;
            while (part > 1 && (part == length || !find_emoji(points + index, part))) {
                part--;
            }
            if (points[index] != 0x200D && points[index] != 0xFE0F && points[index] != 0xFE0E) {
                append_related(&list, points + index, part);
            }
            index += part;
        }
    } else {
        uint32_t point = points[0];
        uint32_t variant[] = { point, 0xFE0F };
        if (find_emoji(variant, COUNT(variant))) {
            append_related(&list, variant, COUNT(variant));
        }
        if (has_properties(point)) {
            append_scalar(&list, (uint32_t)u_toupper((UChar32)point));
            append_scalar(&list, (uint32_t)u_tolower((UChar32)point));
            append_scalar(&list, (uint32_t)u_totitle((UChar32)point));
            append_scalar(&list, (uint32_t)u_charMirror((UChar32)point));
            append_decomposition(&list, point);
            append_accent_variants(&list, point);
        }
        append_confusables(&list, point);
    }
    char *original = characters_unicode_encode(points, length);
    if (!original) {
        list.failed = true;
    }
    char **result = finish_related(&list, original);
    free(original);
    return result;
}
