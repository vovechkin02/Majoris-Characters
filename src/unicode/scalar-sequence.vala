namespace Characters.ScalarSequence {
    /** Validate bounded UTF-8 without allocating a scalar array; -1 on failure. */
    public static int bounded_count (string? text, int limit, bool require_assigned = false) {
        if (text == null || limit < 0 || limit > int.MAX / 4 ||
            text.length > limit * 4 || !text.validate ()) {
            return -1;
        }
        var count = text.char_count ();
        if (count > limit) {
            return -1;
        }
        if (require_assigned) {
            int offset = 0;
            unichar character;
            while (text.get_next_char (ref offset, out character)) {
                if (!Unicode.is_assigned ((uint32) character)) {
                    return -1;
                }
            }
        }
        return (int) count;
    }

    /** Decode validated UTF-8 into an exact, bounded scalar array. */
    public static uint32[]? decode (string? text, int limit) {
        var count = bounded_count (text, limit);
        if (text == null || count < 0) {
            return null;
        }
        var points = new uint32[count];
        int offset = 0;
        unichar character;
        int position = 0;
        while (text.get_next_char (ref offset, out character)) {
            points[position++] = (uint32) character;
        }
        return points;
    }
}
