namespace Characters.HanQuery {
    /** Immutable HanRecord textual field bits, excluding the two numeric properties. */
    private const uint32 ALL_FIELDS = 0xCFFu;
    /** Mandarin and labeled fallback. */
    private const uint32 MANDARIN = (1u << 0) | (1u << 10);
    /** Cantonese readings. */
    private const uint32 CANTONESE = 1u << 1;
    /** Japanese on, kun and combined readings. */
    private const uint32 JAPANESE = (1u << 2) | (1u << 3) | (1u << 4);
    /** Korean readings, Hangul and labeled fallback. */
    private const uint32 KOREAN = (1u << 5) | (1u << 6) | (1u << 11);
    /** English definitions. */
    private const uint32 DEFINITION = 1u << 7;
    /** Number of descriptor words: predicate kind, field mask, numeric value. */
    private const int WORDS = 3;

    /** Parse a positive ASCII decimal group without overflow or sign acceptance. */
    private static uint32 parse_group (string value, uint32 maximum) {
        uint32 group = 0;
        for (var index = 0; index < value.length; index++) {
            var digit = value[index];
            if (digit < '0' || digit > '9') {
                return 0;
            }
            if (group > (maximum - (uint32) (digit - '0')) / 10) {
                return 0;
            }
            group = group * 10 + (uint32) (digit - '0');
        }
        return group;
    }

    /** Fold and interpret one Han term; malformed explicit constraints never match. */
    private static string describe (string term, uint32[] rules, int offset) {
        var folded = Han.fold_text (term) ?? "";
        var kind = Han.Kind.ORDINARY;
        uint32 fields = ALL_FIELDS;
        uint32 group = 0;
        var separator = folded.index_of_char (':');
        if (separator >= 0) {
            var prefix = folded.substring (0, separator);
            switch (prefix) {
                case "mandarin":
                    fields = MANDARIN;
                    break;
                case "cantonese":
                    fields = CANTONESE;
                    break;
                case "japanese":
                    fields = JAPANESE;
                    break;
                case "korean":
                    fields = KOREAN;
                    break;
                case "definition":
                    fields = DEFINITION;
                    break;
                case "radical":
                    kind = Han.Kind.RADICAL;
                    fields = 0;
                    break;
                case "strokes":
                    kind = Han.Kind.STROKES;
                    fields = 0;
                    break;
                default:
                    break;
            }
            if (fields != ALL_FIELDS || kind != Han.Kind.ORDINARY) {
                folded = folded.substring (separator + 1);
                if (kind == Han.Kind.ORDINARY) {
                    kind = Han.Kind.FIELD;
                }
                if (kind == Han.Kind.RADICAL || kind == Han.Kind.STROKES) {
                    group = parse_group (folded, kind == Han.Kind.RADICAL ? 214u : 128u);
                }
                if (folded.length == 0 || (fields == 0 && group == 0)) {
                    kind = Han.Kind.REJECT;
                    fields = 0;
                    group = 0;
                }
            }
        }
        rules[offset] = (uint32) kind;
        rules[offset + 1] = fields;
        rules[offset + 2] = group;
        return folded;
    }

    /** Build copied Han operands for all AND terms and prepare the native scanner. */
    internal static bool prepare (Search.Job job, string text, uint32[] points,
                                  string[] terms, bool exact, bool literal_letter) {
        var rules = new uint32[terms.length * WORDS];
        var folded = new string[terms.length];
        for (var index = 0; index < terms.length; index++) {
            folded[index] = describe (terms[index], rules, index * WORDS);
        }
        return job.prepare (text, points, terms, folded, rules, exact, literal_letter) &&
            Emoji.configure_terms (job, text, terms, literal_letter);
    }
}
