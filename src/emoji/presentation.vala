namespace Characters.Emoji {
    /** Bound for exact supplementary emoji identities. */
    private const int MAX_ALIAS_POINTS = 32;

    /** Label for a base scalar without an explicit presentation selector. */
    internal const string DEFAULT_PRESENTATION = "Default presentation";
    /** Label for a standardized text selector. */
    internal const string TEXT_PRESENTATION = "Text presentation";
    /** Label for a standardized emoji selector. */
    internal const string EMOJI_PRESENTATION = "Emoji presentation";

    /** Return the original English presentation or catalog label for an alias. */
    public static string? alias_label (size_t index) {
        var points = ScalarSequence.decode (alias_text (index), MAX_ALIAS_POINTS);
        if (points == null || points.length == 0) {
            return null;
        }
        if (is_presentation_base (points[0])) {
            if (points.length == 1) {
                return DEFAULT_PRESENTATION;
            }
            if (points.length == 2 && points[1] == 0xFE0Eu) {
                return TEXT_PRESENTATION;
            }
            if (points.length == 2 && points[1] == 0xFE0Fu) {
                return EMOJI_PRESENTATION;
            }
        }
        return Unicode.emoji_label (alias_canonical (index));
    }

    /** Format a presentation alias without changing its exact Unicode identity. */
    public static string? alias_name (size_t index) {
        var label = alias_label (index);
        var points = ScalarSequence.decode (alias_text (index), MAX_ALIAS_POINTS);
        if (label == null || points == null || points.length == 0) {
            return null;
        }
        if (points.length > 2 || (points.length == 2 && points[1] != 0xFE0Eu &&
            points[1] != 0xFE0Fu)) {
            return label;
        }
        var name = Unicode.name (points[0]);
        return name == null ? label : "%s (%s)".printf (name, label);
    }
}

namespace Characters.Unicode {
    /** Preserve canonical scalar and named-sequence precedence before presentation aliases. */
    public static string? sequence_name (uint32[] points) {
        var name = catalog_sequence_name (points);
        if (name != null || points.length <= 1 || points.length > 32) {
            return name;
        }
        var text = encode (points);
        return text == null ? null : Emoji.alias_name (Emoji.alias_index (text));
    }
}
