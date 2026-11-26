namespace Characters.Relationships {
    /** Add an encoded candidate once; never substitute display text for exact text. */
    private static bool append (GenericArray<string> result, string original, string? candidate) {
        if (candidate == null) {
            return false;
        }
        if (candidate == original) {
            return true;
        }
        for (uint index = 0; index < result.length; index++) {
            if (result.get (index) == candidate) {
                return true;
            }
        }
        result.add (candidate);
        return true;
    }

    /** Collect exact scalar, component, presentation and canonical emoji relationships. */
    public static string[]? collect (uint32[] points) {
        if (points.length == 0 || points.length > 32) {
            return null;
        }
        foreach (var point in points) {
            if (!Unicode.is_scalar (point)) {
                return null;
            }
            if (point == 0) {
                return points.length == 1 ? new string[0] : null;
            }
        }
        var original = Unicode.encode (points);
        if (original == null) {
            return null;
        }
        var collected = new GenericArray<string> ();
        if (points.length == 1) {
            var candidates = Unicode.related_scalars (points[0]);
            if (candidates == null) {
                return null;
            }
            foreach (var candidate in candidates) {
                uint32[] single = { candidate };
                if (!append (collected, original, Unicode.encode (single))) {
                    return null;
                }
            }
        } else {
            for (var index = 0; index < points.length;) {
                var part = points.length - index;
                while (part > 1 && (part == points.length ||
                    Unicode.emoji_index (points[index:index + part]) == size_t.MAX)) {
                    part--;
                }
                var point = points[index];
                if (point != 0x200D && point != 0xFE0F && point != 0xFE0E &&
                    !append (collected, original, Unicode.encode (points[index:index + part]))) {
                    return null;
                }
                index += part;
            }
        }
        if (Emoji.is_presentation_base (points[0]) && (points.length == 1 ||
            (points.length == 2 && (points[1] == 0xFE0E || points[1] == 0xFE0F)))) {
            uint32[] pair = { points[0], 0xFE0E };
            if (!append (collected, original, Unicode.encode (pair))) {
                return null;
            }
            pair[1] = 0xFE0F;
            if (!append (collected, original, Unicode.encode (pair))) {
                return null;
            }
        }
        var canonical = Emoji.alias_canonical (Emoji.alias_index (original));
        if (canonical < Unicode.emoji_count () &&
            !append (collected, original, Unicode.emoji_text (canonical))) {
            return null;
        }
        var result = new string[(int) collected.length];
        for (uint index = 0; index < collected.length; index++) {
            result[index] = collected.get (index);
        }
        // Related collections are small; stable insertion sort preserves binary UTF-8 order.
        for (var index = 1; index < result.length; index++) {
            var candidate = result[index];
            var position = index;
            while (position > 0 && strcmp0 (result[position - 1], candidate) > 0) {
                result[position] = result[position - 1];
                position--;
            }
            result[position] = candidate;
        }
        return result;
    }
}
