namespace Characters {
    /** Serializes worker jobs, retaining only the newest pending request. */
    internal sealed class SearchController : Object {
        /** Emitted on the main context only for the latest non-superseded request. */
        public signal void completed (SearchRequest request);

        /** Request being executed; retained until its main-context completion. */
        private SearchRequest? active;
        /** Latest pending request; replacing it discards obsolete work before execution. */
        private SearchRequest? pending;
        /** Permanent shutdown flag, set before the owning window is closed. */
        private bool closed = false;

        /** Replaces any pending search and cancels the active job without blocking GTK. */
        public void submit (Search.Scope scope, int filter, int category, string query) {
            if (closed) {
                return;
            }
            pending = new SearchRequest (scope, filter, category, query);
            if (active != null) {
                active.cancel ();
                return;
            }
            start_pending ();
        }

        /** Prevents further publication and releases queued work; never waits for a worker. */
        public void shutdown () {
            closed = true;
            pending = null;
            if (active != null) {
                active.cancel ();
            }
        }

        /** Starts at most one worker; both closure and controller retain its request safely. */
        private void start_pending () {
            if (closed || pending == null || active != null) {
                return;
            }
            SearchRequest request = pending;
            pending = null;
            active = request;
            try {
                new Thread<void*>.try ("character-search", () => {
                    var result = request.run ();
                    Idle.add (() => {
                        finish (request, result);
                        return Source.REMOVE;
                    });
                    return null;
                });
            } catch (Error error) {
                finish (request, Search.Status.FAILED);
            }
        }

        /** Releases the active slot, suppresses obsolete results and starts the newest request. */
        private void finish (SearchRequest request, Search.Status status) {
            active = null;
            if (closed) {
                return;
            }
            if (pending != null) {
                start_pending ();
                return;
            }
            request.publish (status);
            completed (request);
        }
    }
}
