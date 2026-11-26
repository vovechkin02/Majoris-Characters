namespace Characters {
    /** Immutable metadata and native identity for one usable installed face. */
    internal sealed class FontFaceRecord : Object {
        /** Index into the retained, unsorted native snapshot. */
        public size_t native_index { get; private set; }
        /** Valid UTF-8 primary family. */
        public string family { get; private set; }
        /** Valid UTF-8 style, or null if absent or malformed. */
        public string? style { get; private set; }
        /** Actual filesystem path; not necessarily UTF-8. */
        public string file { get; private set; }
        /** Exact face and named-instance index. */
        public int face_index { get; private set; }
        /** Whether Fontconfig advertises variable font support. */
        public bool variable { get; private set; }
        /** Whether Fontconfig advertises color glyph support. */
        public bool color { get; private set; }
        /** Original byte-level style key, preserving the previous sort order. */
        private string sort_style;

        /** Copies metadata from a pattern that has passed inventory validation. */
        private FontFaceRecord (Fonts.Catalog catalog, size_t index, string family,
                                string file, int face_index) {
            native_index = index;
            this.family = family;
            this.file = file;
            this.face_index = face_index;
            unowned string? raw_style = catalog.style (index);
            sort_style = raw_style ?? "";
            style = raw_style != null && raw_style.validate () ? raw_style : null;
            variable = catalog.is_variable (index);
            color = catalog.is_color (index);
        }

        /** Rejects unusable identities before publishing application-visible metadata. */
        public static FontFaceRecord? read (Fonts.Catalog catalog, size_t index) {
            unowned string? family = catalog.family (index);
            unowned string? file = catalog.file (index);
            var face_index = catalog.face_index (index);
            if (family == null || !family.validate () || file == null || face_index < 0) {
                return null;
            }
            return new FontFaceRecord (catalog, index, family, file, face_index);
        }

        /** Orders bytewise by family, raw style, path, and exact face index. */
        public static int compare (FontFaceRecord left, FontFaceRecord right) {
            var result = strcmp0 (left.family, right.family);
            if (result == 0) {
                result = strcmp0 (left.sort_style, right.sort_style);
            }
            if (result == 0) {
                result = strcmp0 (left.file, right.file);
            }
            if (result == 0) {
                result = (left.face_index > right.face_index ? 1 : 0) -
                    (left.face_index < right.face_index ? 1 : 0);
            }
            return result;
        }

        /** Builds a label using the caller's localized missing-style description. */
        public string display_name (string regular) {
            return "%s — %s".printf (family, style ?? regular);
        }
    }
}
