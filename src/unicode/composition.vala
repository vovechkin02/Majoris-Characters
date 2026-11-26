namespace Characters.Composition {
    /** Maximum scalar count for technical operations, not an editor limit. */
    public const int MAX_POINTS = 16384;

    /** Count extended graphemes using ICU after validating input bounds. */
    public static int graphemes (string? text) {
        if (text == null || ScalarSequence.bounded_count (text, MAX_POINTS) < 0) {
            return -1;
        }
        return Icu.graphemes (text);
    }

    /** Format an entire bounded sequence using the shared scalar formatter. */
    public static string? format (string? text, Inspector.Format format) {
        var points = ScalarSequence.decode (text, MAX_POINTS);
        return points == null ? null : Inspector.format_points (points, format);
    }

    /** Normalize assigned scalars explicitly and reject oversized normalized output. */
    public static string? normalize (string? text, string? form) {
        if (text == null || form == null ||
            ScalarSequence.bounded_count (text, MAX_POINTS, true) < 0) {
            return null;
        }
        var result = Unicode.normalize (text, form);
        if (ScalarSequence.bounded_count (result, MAX_POINTS, true) < 0) {
            return null;
        }
        return result;
    }
}
