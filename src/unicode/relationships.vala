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

    /** Encode only valid non-NUL candidates, retaining exact scalar identity. */
    private static bool append_scalar (GenericArray<string> result, string original,
        uint32 point) {
        if (point == 0 || !Unicode.is_scalar (point)) {
            return true;
        }
        uint32[] single = { point };
        return append (result, original, Unicode.encode (single));
    }

    /** Assemble simple case alternatives from the pinned Unicode mapping adapter. */
    private static bool append_case_variants (GenericArray<string> result, string original,
        uint32 point) {
        return append_scalar (result, original, Icu.uppercase (point)) &&
            append_scalar (result, original, Icu.lowercase (point)) &&
            append_scalar (result, original, Icu.titlecase (point));
    }

    /** Return the first recursive canonical component, reusing caller-owned scratch space. */
    private static uint32 canonical_base (uint32 point, uint32[] scratch) {
        return Icu.canonical_decomposition (point, scratch) > 0 ? scratch[0] : point;
    }

    /** Scan a fixed inclusive range for assigned characters sharing the canonical base. */
    private static bool append_accent_range (GenericArray<string> result, string original,
        uint32 first, uint32 last, uint32 base_point, uint32[] scratch) {
        for (var candidate = first; candidate <= last; candidate++) {
            if (Unicode.is_assigned (candidate) &&
                canonical_base (candidate, scratch) == base_point &&
                !append_scalar (result, original, candidate)) {
                return false;
            }
        }
        return true;
    }

    /** Preserve the existing GNOME-derived reverse accent families and primary-script policy. */
    private static bool append_accent_variants (GenericArray<string> result, string original,
        uint32 point, uint32[] scratch) {
        var script_id = Scalar.script (point);
        if (script_id < 0) {
            return true;
        }
        unowned string? script = Scalar.script_name ((size_t) script_id);
        if (script == null) {
            return true;
        }
        var base_point = canonical_base (point, scratch);
        if (!append_scalar (result, original, base_point)) {
            return false;
        }
        // Adjacent Basic Latin through Latin Extended-B blocks form one contiguous range.
        switch (script) {
            case "Latin":
                return append_accent_range (result, original, 0x0000, 0x024F,
                    base_point, scratch) &&
                    append_accent_range (result, original, 0x1E00, 0x1EFF,
                        base_point, scratch);
            case "Hiragana":
                return append_accent_range (result, original, 0x3040, 0x309F,
                    base_point, scratch);
            case "Katakana":
                return append_accent_range (result, original, 0x30A0, 0x30FF,
                    base_point, scratch) &&
                    append_accent_range (result, original, 0x31F0, 0x31FF,
                        base_point, scratch);
            default:
                return true;
        }
    }

    /** Assemble assigned scalar mappings and advisory confusable class members. */
    private static bool append_scalar_relationships (GenericArray<string> result,
        string original, uint32 point) {
        if (Unicode.is_assigned (point)) {
            if (!append_case_variants (result, original, point) ||
                !append_scalar (result, original, Icu.mirror (point))) {
                return false;
            }
            var scratch = new uint32[32];
            var count = Icu.canonical_decomposition (point, scratch);
            for (size_t index = 0; index < count; index++) {
                if (!append_scalar (result, original, scratch[index])) {
                    return false;
                }
            }
            if (!append_accent_variants (result, original, point, scratch)) {
                return false;
            }
        }
        var confusables = Unicode.confusables (point);
        if (confusables == null) {
            return false;
        }
        foreach (var candidate in confusables) {
            if (!append_scalar (result, original, candidate)) {
                return false;
            }
        }
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
            if (!append_scalar_relationships (collected, original, points[0])) {
                return null;
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
