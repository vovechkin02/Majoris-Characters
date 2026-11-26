#include "language-collections.h"

#include <string.h>

#include <unicode/uscript.h>

#include "scalar-properties.h"

#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define N_(message) (message)

/** ICU locale identifier and its English display name. */
struct LanguageEntry {
    const char *tag;
    const char *label;
};

/** XKB registry layout/variant ID and its declared ISO language codes. */
struct LayoutEntry {
    const char *id;
    const char *languages;
};

#include "generated/languages.h"

/** Add ICU locale scripts to a caller-owned snapshot mask. */
void characters_language_add_locale(const char *locale, uint8_t *scripts, size_t length) {
    if (!locale || !*locale || !scripts || length != CHARACTERS_LANGUAGE_SCRIPT_LIMIT) {
        return;
    }
    UErrorCode status = U_ZERO_ERROR;
    UScriptCode codes[16];
    int32_t count = uscript_getCode(locale, codes, COUNT(codes), &status);
    if (U_FAILURE(status)) {
        return;
    }
    for (int32_t index = 0; index < count; index++) {
        int slot = characters_scalar_script_index(uscript_getShortName(codes[index]));
        if (slot >= 0 && slot < CHARACTERS_LANGUAGE_SCRIPT_LIMIT) {
            scripts[slot] = 1;
        }
    }
}

/** Borrow the language list of an exact XKB layout or registered variant. */
const char *characters_language_layout_languages(const char *id) {
    if (!id) {
        return NULL;
    }
    for (size_t index = 0; index < COUNT(layout_entries); index++) {
        if (strcmp(layout_entries[index].id, id) == 0) {
            return layout_entries[index].languages;
        }
    }
    return NULL;
}

/** Borrow the locale identifier of a named language collection. */
const char *characters_language_tag(size_t index) {
    return index >= 2 && index - 2 < COUNT(language_entries) ?
           language_entries[index - 2].tag : NULL;
}

/** Return the number of selectable language filters. */
size_t characters_language_count(void) {
    return COUNT(language_entries) + 2;
}

/** Borrow a stable label for a language filter. */
const char *characters_language_name(size_t index) {
    if (index == 0) {
        return N_("Current Locale");
    }
    if (index == 1) {
        return N_("Configured Keyboard Languages");
    }
    return index - 2 < COUNT(language_entries) ? language_entries[index - 2].label : NULL;
}

