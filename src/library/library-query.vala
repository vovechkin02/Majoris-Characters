namespace Characters {
    /** Shared matching policy for the full and compact saved collection browsers. */
    internal sealed class LibraryQuery : Object {
        /** Matches exact text, case-folded names and canonical notation without normalization. */
        public static bool matches (string text, string query) {
            if (query == "" || text.casefold ().contains (query)) {
                return true;
            }
            var snapshot = Inspector.from_text (text);
            if (snapshot == null) {
                return false;
            }
            return (snapshot.name () ?? "").casefold ().contains (query) ||
                (snapshot.format (Inspector.Format.CODE) ?? "").casefold ().contains (query);
        }
    }
}
