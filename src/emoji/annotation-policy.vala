namespace Characters.Emoji {
    /** Maximum locale tag bytes before the POSIX encoding or modifier suffix. */
    private const int MAX_LOCALE_BYTES = 63;

    /** Add a resolved locale once, preserving the caller's fallback order. */
    private static void append_locale (uint32[] locales, ref int count, string language) {
        var index = locale_index (language);
        if (index == uint32.MAX || count == locales.length) {
            return;
        }
        for (var previous = 0; previous < count; previous++) {
            if (locales[previous] == index) {
                return;
            }
        }
        locales[count++] = index;
    }

    /** Resolve POSIX and BCP 47-style tags to the bundled annotation tables. */
    internal static uint32[] annotation_locales (string? locale) {
        var locales = new uint32[MAX_LOCALE_BYTES + 1];
        int count = 0;
        if (locale != null) {
            int length = 0;
            while (length < locale.length && locale[length] != '.' &&
                   locale[length] != '@' && locale[length] != ':') {
                length++;
            }
            if (length <= MAX_LOCALE_BYTES) {
                var language = locale.substring (0, length).replace ("-", "_").ascii_down ();
                append_locale (locales, ref count, language);
                if (language == "zh_tw" || language == "zh_hk" || language == "zh_mo") {
                    append_locale (locales, ref count, "zh_Hant");
                } else if (language == "zh_cn" || language == "zh_sg") {
                    append_locale (locales, ref count, "zh_Hans");
                }
                int separator;
                while ((separator = language.last_index_of_char ('_')) >= 0) {
                    language = language.substring (0, separator);
                    append_locale (locales, ref count, language);
                }
            }
        }
        append_locale (locales, ref count, "en");
        return locales[0:count];
    }

    /** Transfer a resolved locale chain before the native scan begins. */
    internal static bool configure_locale (Search.Job job, string locale) {
        return job.set_annotation_locales (annotation_locales (locale));
    }

    /** Fold each operand once; an empty fold can never match annotation keywords. */
    internal static bool configure_terms (Search.Job job, string text, string[] terms,
                                          bool literal_letter) {
        var folded = new string[literal_letter ? 1 : terms.length];
        if (literal_letter) {
            folded[0] = fold_term (text) ?? "";
        } else {
            for (var index = 0; index < terms.length; index++) {
                folded[index] = fold_term (terms[index]) ?? "";
            }
        }
        return job.set_annotation_terms (folded);
    }
}
