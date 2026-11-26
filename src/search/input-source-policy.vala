namespace Characters.InputSourcePolicy {
    /** Maximum snapshot payload bytes, leaving room for its terminating NUL. */
    internal const int MAX_SNAPSHOT_BYTES = 1023;
    /** Bound the number of configured source tuples examined per update. */
    private const int MAX_SOURCES = 256;
    /** Maximum engine or layout identifier length in bytes. */
    private const int MAX_ID_BYTES = 255;
    /** Distinguish resolved IBus language operands from legacy raw XKB IDs. */
    internal const string IBUS_PREFIX = "ibus:";

    /** Accept bounded exact registry identifiers without snapshot delimiter injection. */
    private static bool valid_id (string id) {
        if (id.length == 0 || id.length > MAX_ID_BYTES || id.contains (",")) {
            return false;
        }
        for (var index = 0; index < id.length; index++) {
            if (id[index] <= 0x20 || id[index] >= 0x7F) {
                return false;
            }
        }
        return true;
    }

    /** Validate a descriptor locale, rejecting neutral and unspecified languages. */
    internal static string? engine_language (string? language) {
        if (language == null || language.length == 0 || language.length >= 64) {
            return null;
        }
        var tag = language.replace ("-", "_").ascii_down ();
        if (tag.has_suffix (".utf_8")) {
            tag = tag.substring (0, tag.length - 6);
        } else if (tag.has_suffix (".utf8")) {
            tag = tag.substring (0, tag.length - 5);
        }
        if (tag.length == 0) {
            return null;
        }
        var parts = tag.split ("_");
        var primary = parts[0];
        if (primary.length < 2 || primary.length > 3 || primary == "und" ||
            primary == "mul" || primary == "mis" || primary == "zxx") {
            return null;
        }
        for (var index = 0; index < parts.length; index++) {
            var part = parts[index];
            if (part.length < 2 || part.length > 8 || part == "zzzz" ||
                part == "zyyy" || part == "zinh") {
                return null;
            }
            for (var position = 0; position < part.length; position++) {
                char character = part[position];
                bool letter = (character >= 'a' && character <= 'z') ||
                    (character >= 'A' && character <= 'Z');
                bool digit = character >= '0' && character <= '9';
                if (!letter && !(index > 0 && digit)) {
                    return null;
                }
            }
        }
        return tag;
    }

    /** Collect unique exact configured IBus IDs, never deriving a locale from their names. */
    internal static HashTable<string, bool> engine_ids (Variant? sources) {
        var ids = new HashTable<string, bool> (str_hash, str_equal);
        if (sources == null || !sources.is_of_type (new VariantType ("a(ss)"))) {
            return ids;
        }
        for (size_t index = 0; index < sources.n_children () && index < MAX_SOURCES; index++) {
            var tuple = sources.get_child_value (index);
            var kind_value = tuple.get_child_value (0);
            var id_value = tuple.get_child_value (1);
            unowned string id = id_value.get_string ();
            if (kind_value.get_string () == "ibus" && valid_id (id)) {
                ids.insert (id, true);
            }
        }
        return ids;
    }

    /** Serialize only registered XKB IDs and resolved IBus locales for the worker. */
    internal static string snapshot (Variant? sources, HashTable<string, string> languages) {
        if (sources == null || !sources.is_of_type (new VariantType ("a(ss)"))) {
            return "";
        }
        var result = new StringBuilder ();
        var seen = new HashTable<string, bool> (str_hash, str_equal);
        for (size_t index = 0; index < sources.n_children () && index < MAX_SOURCES; index++) {
            var tuple = sources.get_child_value (index);
            var kind_value = tuple.get_child_value (0);
            var id_value = tuple.get_child_value (1);
            unowned string kind = kind_value.get_string ();
            unowned string id = id_value.get_string ();
            if (!valid_id (id)) {
                continue;
            }
            string? operand = null;
            if (kind == "xkb" && !id.has_prefix (IBUS_PREFIX) &&
                LanguageData.layout_languages (id) != null) {
                operand = id;
            } else if (kind == "ibus") {
                var language = engine_language (languages.lookup (id));
                if (language != null) {
                    operand = IBUS_PREFIX + language;
                }
            }
            if (operand == null || seen.contains (operand) ||
                result.len + operand.length + (result.len > 0 ? 1 : 0) > MAX_SNAPSHOT_BYTES) {
                continue;
            }
            seen.insert (operand, true);
            if (result.len > 0) {
                result.append_c (',');
            }
            result.append (operand);
        }
        return result.str.dup ();
    }
}
