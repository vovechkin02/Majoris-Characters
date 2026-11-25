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

/** Resolve a locale or ISO language code into ICU scripts and then snapshot IDs. */
static void add_locale(const char *locale, bool *scripts) {
    if (!locale || !*locale) {
        return;
    }
    if (strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0 ||
        strcmp(locale, "C.UTF-8") == 0) {
        locale = "en";
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
            scripts[slot] = true;
        }
    }
}

/** Add all declared languages for one exact layout or registered variant. */
static void add_layout(const char *id, size_t length, bool *scripts) {
    for (size_t index = 0; index < COUNT(layout_entries); index++) {
        if (strlen(layout_entries[index].id) != length ||
            strncmp(id, layout_entries[index].id, length) != 0) {
            continue;
        }
        const char *start = layout_entries[index].languages;
        while (*start) {
            size_t size = strcspn(start, " ");
            if (size && size < 16) {
                char language[16];
                memcpy(language, start, size);
                language[size] = '\0';
                add_locale(language, scripts);
            }
            start += size;
            if (*start) {
                start++;
            }
        }
        return;
    }
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

/** Resolve an immutable query-local script mask. */
void characters_language_scripts(int filter, const char *locale, const char *sources,
                                 bool scripts[CHARACTERS_LANGUAGE_SCRIPT_LIMIT]) {
    memset(scripts, 0, CHARACTERS_LANGUAGE_SCRIPT_LIMIT * sizeof(*scripts));
    if (filter == 0) {
        add_locale(locale && *locale ? locale : "en", scripts);
    } else if (filter == 1 && sources) {
        while (*sources) {
            size_t length = strcspn(sources, ",");
            add_layout(sources, length, scripts);
            sources += length;
            if (*sources) {
                sources++;
            }
        }
    } else if (filter >= 2 && (size_t)(filter - 2) < COUNT(language_entries)) {
        add_locale(language_entries[filter - 2].tag, scripts);
    }
}
