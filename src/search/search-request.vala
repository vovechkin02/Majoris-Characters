namespace Characters {
    /** Shares one native request between its worker and main-context consumers. */
    internal sealed class SearchRequest : Object {
        /** Completion status; read on the main context only after publication. */
        private Search.Status status = Search.Status.INVALID;

        /** Owned native job, immutable except for its worker and atomic cancellation. */
        private Search.Job? job;

        /** Bounded input captured before worker execution; null denotes invalid input. */
        private string? query;
        /** Locale used by annotation search and language selection. */
        private string locale = "en";
        /** Configured XKB IDs and resolved IBus locales captured before worker execution. */
        private string input_sources = "";
        /** Scope and filter needed for language collection preparation. */
        private Search.Scope scope = Search.Scope.ALL;
        /** Immutable collection filter. */
        private int filter = -1;
        /** Whether the request traverses an explicit copied scalar repertoire. */
        private bool explicit_points = false;

        /** Creates an unprepared request; parsing and language selection run on the worker. */
        public SearchRequest (Search.Scope scope, int filter, int category, string query,
                              string input_sources = "") {
            this.scope = scope;
            this.filter = filter;
            this.query = query.length <= QueryParser.MAX_BYTES ? query : null;
            locale = Intl.get_language_names ()[0];
            this.input_sources = input_sources.length <= InputSourcePolicy.MAX_SNAPSHOT_BYTES ? input_sources : "";
            job = Search.allocate (scope, filter, category);
        }

        /** Copies a font or encoding repertoire into an independently owned native job. */
        public SearchRequest.for_points (uint32[] points, int category, string query) {
            this.query = query.length <= QueryParser.MAX_BYTES ? query : null;
            explicit_points = true;
            job = Search.allocate_points (points, category);
        }

        /** Interpret input and execute on the worker before main-context publication. */
        public Search.Status run () {
            if (job == null) {
                return Search.Status.FAILED;
            }
            SearchPlan.prepare (job, scope, filter, query, locale, input_sources,
                                explicit_points);
            return job.run ();
        }

        /** Publishes completion on the main context after the worker returns. */
        public void publish (Search.Status result) {
            status = result;
        }

        /** Requests atomic cancellation from the main context. */
        public void cancel () {
            if (job != null) {
                job.cancel ();
            }
        }

        /** Main-context completion status, invalid until the worker publishes its result. */
        public Search.Status get_status () {
            return status;
        }

        /** Number of results after successful publication; zero otherwise. */
        public uint count () {
            if (status != Search.Status.OK || job == null) {
                return 0;
            }
            return (uint) job.count ();
        }

        /** Returns a native result identity at a valid completed position. */
        public uint32 id (uint position) {
            if (status != Search.Status.OK || job == null) {
                return uint32.MAX;
            }
            return job.id (position);
        }
    }
}
