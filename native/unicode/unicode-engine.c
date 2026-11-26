#include "unicode-engine.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <unicode/uchar.h>

#include "../allocation.h"
#include "../scalar-validity.h"
#include "scalar-properties.h"
#include "unihan.h"

#include "vendor/gnome/blocks.h"
#include "vendor/gnome/confusables.h"
#include "vendor/gnome/emoji.h"
#include "vendor/gnome/names.h"
#include "generated/catalog-version.h"

#define MAX_SEQUENCE_LENGTH CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH

/** An official alias and its UCD classification. */
struct AliasEntry {
    gunichar point;
    const gchar *name;
    const gchar *kind;
};

/** An immutable standardized sequence, distinct from emoji display labels. */
struct NamedEntry {
    const gchar *name;
    gunichar points[MAX_SEQUENCE_LENGTH];
    gsize length;
};

#include "generated/supplement.h"

/** One semantic emoji collection in upstream display order. */
struct EmojiGroup {
    const gchar *name;
    const gsize *indices;
    gsize count;
};

#define N_(message) (message)
#define GROUP(name, array) { name, array, G_N_ELEMENTS(array) }
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
    gunichar points[MAX_SEQUENCE_LENGTH];
    gsize length;
};

/** Compare a scalar against an inclusive block range for binary search. */
static gint compare_block(gconstpointer key, gconstpointer item)
{
    gunichar point = *(const guint32 *)key;
    const struct Block *block = item;
    return (point > block->end) - (point < block->start);
}

/** Compare a scalar against the sorted upstream explicit name table. */
static gint compare_name(gconstpointer key, gconstpointer item)
{
    gunichar point = *(const guint32 *)key;
    const struct CharacterName *entry = item;
    return (point > entry->uc) - (point < entry->uc);
}

/** Compare full sequences, including length, without conflating NUL with padding. */
static gint compare_emoji(gconstpointer key, gconstpointer item)
{
    const struct Sequence *sequence = key;
    const struct EmojiCharacter *entry = item;
    gsize entry_length = (gsize)entry->length;
    gsize shared = sequence->length < entry_length ? sequence->length : entry_length;
    for (gsize index = 0; index < shared; index++) {
        if (sequence->points[index] != entry->uc[index]) {
            return sequence->points[index] < entry->uc[index] ? -1 : 1;
        }
    }
    return (sequence->length > entry_length) - (sequence->length < entry_length);
}

/** Return a borrowed explicit upstream name entry, if present. */
static const struct CharacterName *find_name(gunichar point)
{
    return bsearch(&point, character_names, G_N_ELEMENTS(character_names),
                   sizeof(character_names[0]), compare_name);
}

/** Validate a numeric sequence before passing it to ICU or a catalog lookup. */
static gboolean valid_sequence(const gunichar *points, gsize length)
{
    if (!points || length == 0 || length > MAX_SEQUENCE_LENGTH) {
        return FALSE;
    }
    for (gsize index = 0; index < length; index++) {
        if (!characters_unicode_is_scalar(points[index])) {
            return FALSE;
        }
    }
    return TRUE;
}

/** Locate an exact fully qualified emoji sequence, preserving modifiers and joiners. */
static const struct EmojiCharacter *find_emoji(const gunichar *points, gsize length)
{
    if (!valid_sequence(points, length) || length > EMOJI_SEQUENCE_LENGTH) {
        return NULL;
    }
    struct Sequence key = { .length = length };
    memcpy(key.points, points, length * sizeof(*points));
    return bsearch(&key, emoji_characters, G_N_ELEMENTS(emoji_characters),
                   sizeof(emoji_characters[0]), compare_emoji);
}

/** Return whether the bundled snapshot assigns the scalar. */
static gboolean has_properties(gunichar point)
{
    return characters_scalar_category(point) != U_UNASSIGNED;
}

/** Compare a scalar against the GNOME confusable-class index. */
static gint compare_confusable(gconstpointer key, gconstpointer item)
{
    gunichar point = *(const guint32 *)key;
    const struct ConfusableCharacterClass *entry = item;
    return (point > entry->uc) - (point < entry->uc);
}

gboolean characters_unicode_is_scalar(gunichar point)
{
    return characters_scalar_is_valid(point);
}

gboolean characters_unicode_is_assigned(gunichar point)
{
    return has_properties(point);
}

gchar *characters_unicode_properties_version(void)
{
    return characters_try_strdup(characters_scalar_version());
}

const gchar *characters_unicode_catalog_version(void)
{
    return CATALOG_VERSION;
}

const gchar *characters_unicode_supplement_version(void)
{
    return SUPPLEMENT_VERSION;
}

gchar *characters_unicode_name(gunichar point)
{
    if (!characters_unicode_is_assigned(point)) {
        return NULL;
    }
    gchar name[256];
    UErrorCode error = U_ZERO_ERROR;
    int32_t length = u_charName((UChar32)point, U_UNICODE_CHAR_NAME, name, sizeof(name), &error);
    if (U_SUCCESS(error) && length > 0) {
        return characters_try_strdup(name);
    }
    const struct CharacterName *entry = find_name(point);
    if (entry) {
        return characters_try_strdup(entry->name);
    }
    /* Canonical names of Unicode 17 single-scalar emoji omitted by GNOME's name generator. */
    switch (point) {
        case 0x1F6D8:
            return characters_try_strdup("LANDSLIDE");
        case 0x1FA8A:
            return characters_try_strdup("TROMBONE");
        case 0x1FA8E:
            return characters_try_strdup("TREASURE CHEST");
        case 0x1FAC8:
            return characters_try_strdup("HAIRY CREATURE");
        case 0x1FACD:
            return characters_try_strdup("ORCA");
        case 0x1FAEA:
            return characters_try_strdup("DISTORTED FACE");
        case 0x1FAEF:
            return characters_try_strdup("FIGHT CLOUD");
        default:
            return characters_scalar_algorithmic_name(point);
    }
}

gchar *characters_unicode_aliases(gunichar point)
{
    if (!characters_unicode_is_scalar(point)) {
        return NULL;
    }
    gsize low = 0;
    gsize high = G_N_ELEMENTS(aliases);
    while (low < high) {
        gsize middle = low + (high - low) / 2;
        if (aliases[middle].point < point) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    gsize end = low;
    gsize bytes = 0;
    while (end < G_N_ELEMENTS(aliases) && aliases[end].point == point) {
        bytes += strlen(aliases[end++].name) + 2;
    }
    if (!bytes) {
        gchar name[256];
        UErrorCode error = U_ZERO_ERROR;
        int32_t length = u_charName((UChar32)point, U_CHAR_NAME_ALIAS, name, sizeof(name), &error);
        return U_SUCCESS(error) && length > 0 ? characters_try_strdup(name) : NULL;
    }
    gchar *result = g_try_malloc(bytes);
    if (!result) {
        return NULL;
    }
    gchar *cursor = result;
    for (gsize index = low; index < end; index++) {
        if (index != low) {
            *cursor++ = ';';
            *cursor++ = ' ';
        }
        gsize length = strlen(aliases[index].name);
        memcpy(cursor, aliases[index].name, length);
        cursor += length;
    }
    *cursor = '\0';
    return result;
}

const gchar *characters_unicode_unihan_version(void)
{
    return characters_unihan_version();
}

/** Resolve an ISO script code through the same snapshot used by browsing. */
const gchar *characters_unicode_script_name(const gchar *code)
{
    gint index = characters_scalar_script_index(code);
    return index < 0 ? NULL : characters_scalar_script_name((gsize)index);
}

gsize characters_unicode_block_count(void)
{
    return G_N_ELEMENTS(all_blocks);
}

const gchar *characters_unicode_block_name(gsize index)
{
    return index < G_N_ELEMENTS(all_blocks) ? all_blocks[index].name : NULL;
}

guint32 characters_unicode_block_start(gsize index)
{
    return index < G_N_ELEMENTS(all_blocks) ? all_blocks[index].start : G_MAXUINT32;
}

guint32 characters_unicode_block_end(gsize index)
{
    return index < G_N_ELEMENTS(all_blocks) ? all_blocks[index].end : G_MAXUINT32;
}

const gchar *characters_unicode_find_block(gunichar point)
{
    const struct Block *block = bsearch(&point, all_blocks, G_N_ELEMENTS(all_blocks),
                                        sizeof(all_blocks[0]), compare_block);
    return block ? block->name : NULL;
}

gchar *characters_unicode_catalog_sequence_name(const gunichar *points, gsize length)
{
    if (!valid_sequence(points, length)) {
        return NULL;
    }
    if (length == 1) {
        return characters_unicode_name(points[0]);
    }
    for (gsize index = 0; index < G_N_ELEMENTS(named_sequences); index++) {
        const struct NamedEntry *entry = &named_sequences[index];
        if (entry->length == length && !memcmp(entry->points, points, length * sizeof(*points))) {
            return characters_try_strdup(entry->name);
        }
    }
    const struct EmojiCharacter *emoji = find_emoji(points, length);
    if (emoji) {
        return characters_try_strdup(emoji->name);
    }
    return NULL;
}

gsize characters_unicode_emoji_group_count(void)
{
    return G_N_ELEMENTS(emoji_groups);
}

const gchar *characters_unicode_emoji_group_name(gsize group)
{
    return group < G_N_ELEMENTS(emoji_groups) ? emoji_groups[group].name : NULL;
}

gsize characters_unicode_emoji_group_size(gsize group)
{
    return group < G_N_ELEMENTS(emoji_groups) ? emoji_groups[group].count : 0;
}

gsize characters_unicode_emoji_group_index(gsize group, gsize index)
{
    if (group >= G_N_ELEMENTS(emoji_groups) || index >= emoji_groups[group].count) {
        return G_MAXSIZE;
    }
    return emoji_groups[group].indices[index];
}

gsize characters_unicode_emoji_index(const gunichar *points, gsize length)
{
    const struct EmojiCharacter *entry = find_emoji(points, length);
    return entry ? (gsize)(entry - emoji_characters) : G_MAXSIZE;
}

gsize characters_unicode_emoji_count(void)
{
    return G_N_ELEMENTS(emoji_characters);
}

gchar *characters_unicode_emoji_text(gsize index)
{
    if (index >= G_N_ELEMENTS(emoji_characters)) {
        return NULL;
    }
    return characters_unicode_encode(emoji_characters[index].uc,
                                     (gsize)emoji_characters[index].length);
}

const gchar *characters_unicode_emoji_label(gsize index)
{
    return index < G_N_ELEMENTS(emoji_characters) ? emoji_characters[index].name : NULL;
}

gsize characters_unicode_named_count(void)
{
    return G_N_ELEMENTS(named_sequences);
}

const gchar *characters_unicode_named_name(gsize index)
{
    return index < G_N_ELEMENTS(named_sequences) ? named_sequences[index].name : NULL;
}

gchar *characters_unicode_named_text(gsize index)
{
    if (index >= G_N_ELEMENTS(named_sequences)) {
        return NULL;
    }
    return characters_unicode_encode(named_sequences[index].points, named_sequences[index].length);
}

gunichar *characters_unicode_confusables(gunichar point, gint *count)
{
    if (!count) {
        return NULL;
    }
    *count = 0;
    if (!characters_unicode_is_scalar(point)) {
        return NULL;
    }
    const struct ConfusableCharacterClass *entry = bsearch(
        &point, confusable_character_classes, G_N_ELEMENTS(confusable_character_classes),
        sizeof(confusable_character_classes[0]), compare_confusable);
    gsize length = 0;
    gsize offset = 0;
    if (entry && entry->index < G_N_ELEMENTS(confusable_classes)) {
        const struct ConfusableClass *group = &confusable_classes[entry->index];
        offset = group->offset;
        length = group->length;
        if (offset > G_N_ELEMENTS(confusable_characters) ||
            length > G_N_ELEMENTS(confusable_characters) - offset || length > G_MAXINT) {
            return NULL;
        }
    }
    gunichar *items = g_try_new(gunichar, length ? length : 1);
    if (!items) {
        return NULL;
    }
    if (length) {
        memcpy(items, confusable_characters + offset, length * sizeof(*items));
    }
    *count = (gint)length;
    return items;
}
