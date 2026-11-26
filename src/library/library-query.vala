namespace Characters {
    /** Shared matching policy for the full and compact saved collection browsers. */
    internal sealed class LibraryQuery : Object {
        /** Matches exact text, case-folded names and canonical notation without normalization. */
        public static bool matches (string text, string? query) {
            if (query == null) {
                return false;
            }
            if (query == "" || (Unicode.casefold (text) ?? "").contains (query)) {
                return true;
            }
            var snapshot = Inspector.from_text (text);
            if (snapshot == null) {
                return false;
            }
            return (Unicode.casefold (snapshot.name () ?? "") ?? "").contains (query) ||
                (Unicode.casefold (snapshot.format (Inspector.Format.CODE) ?? "") ?? "").contains (query);
        }
    }
}
