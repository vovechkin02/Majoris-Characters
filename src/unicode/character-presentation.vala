namespace Characters.Search {
    /** Maximum scalar value, also the boundary between scalar and catalog identities. */
    private const uint32 SCALAR_LIMIT = 0x110000u;
    /** Bound shared with the native catalog's exact sequences. */
    private const int MAX_IDENTITY_POINTS = 32;

    /** Decode a valid identity directly, preserving NUL without a text round trip. */
    public static uint32[]? points (uint32 id) {
        if (id < SCALAR_LIMIT) {
            return Unicode.is_scalar (id) ? new uint32[] { id } : null;
        }
        return ScalarSequence.decode (text (id), MAX_IDENTITY_POINTS);
    }

    /** Return a safe tile representation; never use it as exact clipboard text. */
    public static string? display (uint32 id) {
        if (id < SCALAR_LIMIT && Unicode.is_scalar (id)) {
            var category = Scalar.category (id);
            if (!Unicode.is_assigned (id) || category == Scalar.Category.SPACE_SEPARATOR ||
                category == Scalar.Category.LINE_SEPARATOR ||
                category == Scalar.Category.PARAGRAPH_SEPARATOR ||
                category == Scalar.Category.CONTROL_CHAR ||
                category == Scalar.Category.FORMAT_CHAR || id == 0x034Fu ||
                id == 0x115Fu || id == 0x1160u || id == 0x3164u || id == 0xFFA0u) {
                return "·";
            }
            if (category == Scalar.Category.NON_SPACING_MARK ||
                category == Scalar.Category.ENCLOSING_MARK ||
                category == Scalar.Category.COMBINING_SPACING_MARK) {
                return Unicode.encode (new uint32[] { 0x25CCu, id });
            }
        }
        return text (id);
    }

    /** Resolve the canonical name, aliases or descriptive fallback without altering identity. */
    public static string? name (uint32 id) {
        if (id >= SCALAR_LIMIT) {
            if (id >= 0x180000u && id < 0x200000u) {
                return Emoji.alias_name (id - 0x180000u);
            }
            return catalog_sequence_label (id);
        }
        if (!Unicode.is_scalar (id)) {
            return null;
        }
        if (Unicode.is_assigned (id)) {
            var canonical = Unicode.name (id);
            if (canonical != null) {
                return canonical;
            }
            var aliases = Unicode.aliases (id);
            if (aliases != null) {
                return aliases;
            }
        }
        return fallback_name (id);
    }

    /** Format all exact scalar values through the shared inspector formatter. */
    public static string? code (uint32 id) {
        var values = points (id);
        return values == null ? null : Inspector.format_points (values, Inspector.Format.CODE);
    }
}
