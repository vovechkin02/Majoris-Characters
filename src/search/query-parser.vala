namespace Characters.QueryParser {
    /** Maximum UTF-8 input size, matching the native prepared-query contract. */
    internal const int MAX_BYTES = 512;
    /** Maximum exact sequence length. */
    private const int MAX_POINTS = 32;
    /** Maximum number of AND-connected metadata terms. */
    private const int MAX_TERMS = 64;

    /** Recognize only the four ASCII separators used by the search syntax. */
    private static bool is_separator (char character) {
        return character == ' ' || character == '\t' || character == '\r' || character == '\n';
    }

    /** Trim ASCII separators without altering any Unicode text. */
    private static string trim_query (string text) {
        int start = 0;
        int end = text.length;
        while (start < end && is_separator (text[start])) {
            start++;
        }
        while (end > start && is_separator (text[end - 1])) {
            end--;
        }
        return text.substring (start, end - start);
    }

    /** Split bounded input on ASCII separators, preserving each term's exact bytes. */
    private static string[]? split_terms (string text) {
        string[] terms = {};
        int offset = 0;
        while (offset < text.length) {
            while (offset < text.length && is_separator (text[offset])) {
                offset++;
            }
            if (offset == text.length) {
                break;
            }
            if (terms.length == MAX_TERMS) {
                return null;
            }
            var start = offset;
            while (offset < text.length && !is_separator (text[offset])) {
                offset++;
            }
            terms += text.substring (start, offset - start);
        }
        return terms;
    }

    /** Parse unsigned ASCII digits, enforcing scalar bounds and rejecting overflow. */
    private static bool parse_number (string text, uint32 radix, out uint32 point) {
        point = 0;
        if (text.length == 0 || text.length > 7) {
            return false;
        }
        uint32 value = 0;
        for (var index = 0; index < text.length; index++) {
            var character = text[index];
            uint32 digit = 255;
            if (character >= '0' && character <= '9') {
                digit = (uint32) (character - '0');
            } else if (character >= 'a' && character <= 'f') {
                digit = (uint32) (character - 'a' + 10);
            } else if (character >= 'A' && character <= 'F') {
                digit = (uint32) (character - 'A' + 10);
            }
            if (digit >= radix || value > (0x10FFFFu - digit) / radix) {
                return false;
            }
            value = value * radix + digit;
        }
        if (!Unicode.is_scalar (value)) {
            return false;
        }
        point = value;
        return true;
    }

    /** Decode explicitly prefixed numeric tokens; never infer bases or normalize text. */
    public static uint32[]? decode_code (string? input) {
        if (input == null || input.length > MAX_BYTES || !input.validate ()) {
            return null;
        }
        var terms = split_terms (trim_query (input));
        if (terms == null || terms.length == 0 || terms.length > MAX_POINTS) {
            return null;
        }
        var points = new uint32[terms.length];
        for (var index = 0; index < terms.length; index++) {
            var term = terms[index];
            var lower = term.ascii_down ();
            uint32 radix = 16;
            int prefix_length = 2;
            if (lower.has_prefix ("dec:")) {
                radix = 10;
                prefix_length = 4;
            } else if (!lower.has_prefix ("u+") && !lower.has_prefix ("0x")) {
                return null;
            }
            uint32 point;
            if (!parse_number (term.substring (prefix_length), radix, out point)) {
                return null;
            }
            points[index] = point;
        }
        return points;
    }

    /** Prepare one native scan without transferring ownership of any Vala strings or arrays. */
    internal static bool prepare (Search.Job job, string? input, string locale = "en") {
        if (!Search.configure_metadata (job) || !Emoji.configure_locale (job, locale)) {
            return false;
        }
        if (input == null || input.length > MAX_BYTES || !input.validate ()) {
            return false;
        }
        var text = trim_query (input);
        string[] no_terms = {};
        uint32[] no_points = {};
        if (text.length == 0) {
            return HanQuery.prepare (job, text, no_points, no_terms, false, false);
        }
        var lower = text.ascii_down ();
        if (lower.has_prefix ("u+") || lower.has_prefix ("0x") || lower.has_prefix ("dec:")) {
            var explicit_points = decode_code (text);
            return explicit_points != null &&
                HanQuery.prepare (job, text, explicit_points, no_terms, true, false);
        }
        var decoded = ScalarSequence.decode (text, MAX_POINTS);
        if (decoded == null) {
            decoded = new uint32[0];
        }
        uint32[] points = (owned) decoded;
        if (points.length == 1) {
            var category = Unicode.get_property_value (points[0], "General_Category");
            var literal_letter = points[0] > 0x7F && category != null &&
                category.has_prefix ("L");
            return HanQuery.prepare (job, text, points, no_terms, true, literal_letter);
        }
        if (points.length > 1 && (Unicode.emoji_index (points) != size_t.MAX ||
            Emoji.alias_index (text) != size_t.MAX)) {
            return HanQuery.prepare (job, text, points, no_terms, true, false);
        }
        uint32 raw;
        if (text.length >= 2 && text.length <= 6 && parse_number (text, 16, out raw)) {
            for (var index = 0; index < text.length; index++) {
                if (text[index] >= '0' && text[index] <= '9') {
                    uint32[] numeric = { raw };
                    return HanQuery.prepare (job, text, numeric, no_terms, true, false);
                }
            }
            // All-letter hexadecimal values of four or more digits are also metadata terms.
            if (text.length >= 4) {
                points = new uint32[] { raw };
            }
        }
        var terms = split_terms (text);
        return terms != null && HanQuery.prepare (job, text, points, terms, false, false);
    }

    /** Construct a prepared synchronous request; used by headless clients and tests. */
    public static Search.Job? create (Search.Scope scope, int filter, int category,
                                      string? input) {
        return create_localized (scope, filter, category, input, "en");
    }

    /** Construct a synchronous request with explicit annotation locale selection. */
    public static Search.Job? create_localized (Search.Scope scope, int filter, int category,
                                                string? input, string locale) {
        var job = Search.allocate (scope, filter, category);
        if (job != null) {
            SearchPlan.prepare (job, scope, filter, input, locale, "");
        }
        return (owned) job;
    }

    /** Construct a prepared request over a copied explicit scalar repertoire. */
    public static Search.Job? create_points (uint32[]? points, int category, string? input) {
        var job = Search.allocate_points (points, category);
        if (job != null) {
            SearchPlan.prepare (job, Search.Scope.ALL, -1, input, "en", "", true);
        }
        return (owned) job;
    }
}
