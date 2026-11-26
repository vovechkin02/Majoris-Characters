namespace Characters {
    /** Shared bounds and exact identities for desktop search and input-source requests. */
    internal sealed class DesktopSearch : Object {
        /** Maximum results returned to a desktop consumer. */
        public const uint LIMIT = 20;
        /** Maximum UTF-8 query length accepted by the shared Vala parser. */
        public const int MAX_QUERY_BYTES = QueryParser.MAX_BYTES;

        /** Rejects empty, oversized, or invalid queries before allocating a worker. */
        public static bool valid_query (string query) {
            return query.length <= MAX_QUERY_BYTES && query.validate () && query.strip () != "";
        }

        /** Joins desktop terms without allocating an unbounded concatenated query. */
        public static string? join_terms (string[] terms) {
            var joined = new StringBuilder ();
            foreach (var term in terms) {
                if (joined.len + term.length + (joined.len > 0 ? 1 : 0) > MAX_QUERY_BYTES) {
                    return null;
                }
                if (joined.len > 0) {
                    joined.append_c (' ');
                }
                joined.append (term);
            }
            var query = joined.str;
            return valid_query (query) ? query : null;
        }

        /** Returns bounded, copyable results as stable Unicode code-point notation. */
        public static string[] identities (SearchRequest request) {
            string[] values = {};
            for (uint index = 0; index < request.count () && values.length < LIMIT; index++) {
                var id = request.id (index);
                var code = Search.code (id);
                if (code != null && Search.text (id) != null) {
                    values += code;
                }
            }
            return values;
        }

        /** Reads an EOF-terminated query without exposing text in process arguments.
         * Rejects NUL, read errors, invalid UTF-8, and inputs exceeding the byte limit.
         * Called only by the local headless CLI; never on the graphical main context.
         */
        public static string? read_input_query () {
            var query = new StringBuilder ();
            while (true) {
                int byte = stdin.getc ();
                if (byte < 0) {
                    if (!stdin.eof ()) {
                        return null;
                    }
                    break;
                }
                if (byte == 0 || query.len >= MAX_QUERY_BYTES) {
                    return null;
                }
                query.append_c ((char) byte);
            }
            return valid_query (query.str) ? query.str.dup () : null;
        }

        /** Produces a line-oriented, base64-encoded text/name protocol for the input engine. */
        public static int print_input_results (string query) {
            if (!valid_query (query)) {
                return 2;
            }
            var request = new SearchRequest (Search.Scope.ALL, -1, -1, query);
            request.publish (request.run ());
            if (request.get_status () != Search.Status.OK) {
                return 1;
            }
            foreach (var code in identities (request)) {
                var snapshot = Inspector.from_code (code);
                if (snapshot == null) {
                    continue;
                }
                var text = snapshot.text ();
                var name = snapshot.name () ?? code;
                if (text != null) {
                    print ("%s\t%s\n", Base64.encode (text.data), Base64.encode (name.data));
                }
            }
            return 0;
        }
    }
}
