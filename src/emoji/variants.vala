namespace Characters.Emoji {
    /** Owns exact alternatives and display labels in stable insertion order. */
    public class Variants : Object {
        /** Exact alternatives; the selected identity is always first. */
        private string[] texts = {};

        /** Parallel stable catalog or presentation labels. */
        private string[] labels = {};

        /** Number of initialized alternatives. */
        public size_t count () {
            return texts.length;
        }

        /** Borrow exact text for the collection lifetime; null outside its bounds. */
        public unowned string? text (size_t index) {
            return index < texts.length ? texts[index] : null;
        }

        /** Borrow the corresponding stable label; null outside collection bounds. */
        public unowned string? label (size_t index) {
            return index < labels.length ? labels[index] : null;
        }

        /** Append an exact alternative once, without replacing its first label. */
        internal bool append (string? text, string? label) {
            if (text == null || label == null) {
                return false;
            }
            if (!(text in texts)) {
                texts += text;
                labels += label;
            }
            return true;
        }
    }

    /** Build exact alternatives from catalog structure, retaining the selected text first. */
    public static Variants? create_variants (string? text) {
        var points = ScalarSequence.decode (text, Inspector.MAX_POINTS);
        if (text == null || points == null || points.length == 0) {
            return null;
        }
        var selected = Unicode.emoji_index (points);
        var alias = alias_index (text);
        if (selected == size_t.MAX && alias == size_t.MAX) {
            return null;
        }
        var input_label = selected != size_t.MAX ? Unicode.emoji_label (selected) :
            alias_label (alias);
        if (selected == size_t.MAX) {
            selected = alias_canonical (alias);
        }
        var variants = new Variants ();
        if (!variants.append (text, input_label)) {
            return null;
        }
        if (is_presentation_base (points[0]) && (points.length == 1 ||
            (points.length == 2 && (points[1] == 0xFE0E || points[1] == 0xFE0F)))) {
            uint32[] presentation = { points[0], 0xFE0E };
            if (!variants.append (Unicode.encode (presentation[0:1]), "Default presentation") ||
                !variants.append (Unicode.encode (presentation), "Text presentation")) {
                return null;
            }
            presentation[1] = 0xFE0F;
            if (!variants.append (Unicode.encode (presentation), "Emoji presentation")) {
                return null;
            }
        }
        if (selected == size_t.MAX) {
            return variants;
        }
        var canonical = Unicode.emoji_text (selected);
        points = ScalarSequence.decode (canonical, Inspector.MAX_POINTS);
        if (points == null || points.length == 0 ||
            !variants.append (canonical, Unicode.emoji_label (selected))) {
            return null;
        }
        var key = family_key (points);
        for (size_t index = 0; index < alias_count (); index++) {
            if (alias_canonical (index) == selected &&
                !variants.append (alias_text (index), alias_label (index))) {
                return null;
            }
        }
        for (size_t index = 0; index < Unicode.emoji_count (); index++) {
            if (index == selected) {
                continue;
            }
            var candidate = Unicode.emoji_text (index);
            var candidate_points = ScalarSequence.decode (candidate, Inspector.MAX_POINTS);
            if (candidate_points == null || candidate_points.length == 0) {
                return null;
            }
            if (same_family (key, family_key (candidate_points)) &&
                !variants.append (candidate, Unicode.emoji_label (index))) {
                return null;
            }
        }
        return variants;
    }
}
