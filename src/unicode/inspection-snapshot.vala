namespace Characters.Inspector {
    /** Maximum input scalars; normalized output may exceed this bound. */
    public const int MAX_POINTS = 32;

    /** Immutable exact scalar snapshot independent of search and GTK lifetimes. */
    public class Snapshot : Object {
        /** Owned exact scalar sequence, including embedded NUL. */
        private uint32[] points;

        /** Construct from an already validated, privately owned scalar sequence. */
        internal Snapshot (owned uint32[] points) {
            this.points = (owned) points;
        }

        /** Return the number of original scalars. */
        public size_t count () {
            return points.length;
        }

        /** Return an exact scalar or UINT32_MAX for an invalid index. */
        public uint32 point (size_t index) {
            return index < points.length ? points[index] : uint32.MAX;
        }

        /** Return the canonical scalar or catalog sequence name when available. */
        public string? name () {
            return Unicode.sequence_name (points);
        }

        /** Return exact UTF-8, or null for embedded NUL. */
        public string? text () {
            return Unicode.encode (points);
        }

        /** Return a safe visible preview; never use it as copy data. */
        public string? display () {
            var first = points[0];
            if (points.length == 1) {
                return Scalar.is_ignorable (first) ? "·" : Search.display (first);
            }
            var original = text ();
            if (original != null && Emoji.is_candidate (original)) {
                return original;
            }
            var output = new StringBuilder.sized ((size_t) (points.length + 1) * 4);
            var category = Scalar.category (first);
            if (category == Scalar.Category.NON_SPACING_MARK ||
                category == Scalar.Category.COMBINING_SPACING_MARK ||
                category == Scalar.Category.ENCLOSING_MARK) {
                output.append_unichar (0x25CC);
            }
            foreach (var point in points) {
                category = Scalar.category (point);
                if (category == Scalar.Category.CONTROL_CHAR ||
                    category == Scalar.Category.FORMAT_CHAR ||
                    category == Scalar.Category.LINE_SEPARATOR ||
                    category == Scalar.Category.PARAGRAPH_SEPARATOR ||
                    category == Scalar.Category.SPACE_SEPARATOR ||
                    category == Scalar.Category.UNASSIGNED || Scalar.is_ignorable (point)) {
                    output.append_unichar (0x00B7);
                } else {
                    output.append_unichar ((unichar) point);
                }
            }
            return output.str;
        }

        /** Return the requested exact scalar representation. */
        public string? format (Format format) {
            return format_points (points, format);
        }

        /** Normalize as explicit scalars, including NUL, without truncating expansion. */
        public string? normalize (string? form) {
            if (form == null) {
                return null;
            }
            var normalized = Icu.normalize_points (points, form);
            return normalized == null ? null : format_points (normalized, Format.CODE);
        }

        /** Return owned, sorted related text identities from the Unicode relationship service. */
        public string[]? related () {
            return Relationships.collect (points);
        }
    }

    /** Resolve an opaque search identity to an independent snapshot. */
    public static Snapshot? from_id (uint32 id) {
        return from_code (Search.code (id));
    }

    /** Decode one to 32 UTF-8 scalars; empty or malformed text is rejected. */
    public static Snapshot? from_text (string? text) {
        var points = ScalarSequence.decode (text, MAX_POINTS);
        if (points == null || points.length == 0) {
            return null;
        }
        return new Snapshot ((owned) points);
    }

    /** Parse exact numeric notation using the shared Vala search parser. */
    public static Snapshot? from_code (string? code) {
        var points = QueryParser.decode_code (code);
        if (points == null || points.length == 0 || points.length > MAX_POINTS) {
            return null;
        }
        return new Snapshot ((owned) points);
    }

    /** Format an immediate decomposition rather than recursive normalization. */
    public static string? decomposition (uint32 point) {
        var points = new uint32[MAX_POINTS];
        var count = Icu.decomposition (point, points);
        if (count == 0 || count > MAX_POINTS) {
            return null;
        }
        points.resize ((int) count);
        return format_points (points, Format.CODE);
    }
}
