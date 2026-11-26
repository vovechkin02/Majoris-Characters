#include "language-collections.h"

#include <stdint.h>
#include <string.h>

#include <glib.h>

#include <unicode/uscript.h>

#include "scalar-properties.h"

#define N_(message) (message)

/** ICU locale identifier and its English display name. */
struct LanguageEntry {
    const gchar *tag;
    const gchar *label;
};

/** XKB registry layout/variant ID and its declared ISO language codes. */
struct LayoutEntry {
    const gchar *id;
    const gchar *languages;
};

#include "generated/languages.h"

/** Add ICU locale scripts to a caller-owned snapshot mask. */
void characters_language_add_locale(const gchar *locale, guint8 *scripts, gsize length)
{
    if (!locale || !*locale || !scripts || length != CHARACTERS_LANGUAGE_SCRIPT_LIMIT) {
        return;
    }
    UErrorCode status = U_ZERO_ERROR;
    UScriptCode codes[16];
    int32_t count = uscript_getCode(locale, codes, G_N_ELEMENTS(codes), &status);
    if (U_FAILURE(status)) {
        return;
    }
    for (int32_t index = 0; index < count; index++) {
        gint slot = characters_scalar_script_index(uscript_getShortName(codes[index]));
        if (slot >= 0 && slot < CHARACTERS_LANGUAGE_SCRIPT_LIMIT) {
            scripts[slot] = 1;
        }
    }
}

/** Borrow the language list of an exact XKB layout or registered variant. */
const gchar *characters_language_layout_languages(const gchar *id)
{
    if (!id) {
        return NULL;
    }
    for (gsize index = 0; index < G_N_ELEMENTS(layout_entries); index++) {
        if (strcmp(layout_entries[index].id, id) == 0) {
            return layout_entries[index].languages;
        }
    }
    return NULL;
}

/** Borrow the locale identifier of a named language collection. */
const gchar *characters_language_tag(gsize index)
{
    return index >= 2 && index - 2 < G_N_ELEMENTS(language_entries) ?
           language_entries[index - 2].tag : NULL;
}

/** Return the number of selectable language filters. */
gsize characters_language_count(void)
{
    return G_N_ELEMENTS(language_entries) + 2;
}

/** Borrow a stable label for a language filter. */
const gchar *characters_language_name(gsize index)
{
    if (index == 0) {
        return N_("Current Locale");
    }
    if (index == 1) {
        return N_("Configured Keyboard Languages");
    }
    return index - 2 < G_N_ELEMENTS(language_entries) ? language_entries[index - 2].label : NULL;
}

