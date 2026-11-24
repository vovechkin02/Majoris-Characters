namespace Characters {
    /** Shares one native request between its worker and main-context consumers. */
    internal sealed class SearchRequest : Object {
        /** Completion status; read on the main context only after publication. */
        private Search.Status status = Search.Status.INVALID;

        /** Owned native job, immutable except for its worker and atomic cancellation. */
        private Search.Job? job;

        /** Creates a request with a copied query and immutable filter parameters. */
        public SearchRequest (Search.Scope scope, int filter, int category, string query) {
            job = Search.create (scope, filter, category, query);
        }

        /** Copies a font or encoding repertoire into an independently owned native job. */
        public SearchRequest.for_points (uint32[] points, int category, string query) {
            job = Search.create_points (points, category, query);
        }

        /** Runs on the worker exactly once and returns the status for main-context publication. */
        public Search.Status run () {
            if (job == null) {
                return Search.Status.FAILED;
            }
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
