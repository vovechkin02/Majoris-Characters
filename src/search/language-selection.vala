namespace Characters.LanguageSelection {
    /** Fixed snapshot script-mask width; one byte per script ID. */
    private const int SCRIPT_LIMIT = 256;

    /** Map the portable locale identifiers to the existing English fallback. */
    private static string normalize_locale (string locale) {
        if (locale == "" || locale == "C" || locale == "POSIX" || locale == "C.UTF-8" ||
            locale.length >= 64) {
            return "en";
        }
        return locale;
    }

    /** Resolve named, current-locale and configured XKB/IBus collections before scanning. */
    public static bool configure (Search.Job job, int filter, string locale, string sources) {
        var scripts = new uint8[SCRIPT_LIMIT];
        if (filter == 0) {
            LanguageData.add_locale (normalize_locale (locale), scripts);
        } else if (filter == 1 && sources.length <= InputSourcePolicy.MAX_SNAPSHOT_BYTES) {
            foreach (var id in sources.split (",")) {
                if (id.has_prefix (InputSourcePolicy.IBUS_PREFIX)) {
                    var language = InputSourcePolicy.engine_language (
                        id.substring (InputSourcePolicy.IBUS_PREFIX.length));
                    if (language != null) {
                        LanguageData.add_locale (language, scripts);
                    }
                    continue;
                }
                var languages = LanguageData.layout_languages (id);
                if (languages == null) {
                    continue;
                }
                foreach (var language in languages.split (" ")) {
                    if (language.length > 0 && language.length < 16) {
                        LanguageData.add_locale (normalize_locale (language), scripts);
                    }
                }
            }
        } else if (filter >= 2) {
            var tag = LanguageData.tag ((size_t) filter);
            if (tag != null) {
                LanguageData.add_locale (normalize_locale (tag), scripts);
            }
        }
        return job.set_scripts (scripts);
    }
}
