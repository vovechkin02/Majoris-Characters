namespace Characters {
    /** GNOME Shell SearchProvider2 contract; signatures match the desktop search protocol. */
    [DBus (name = "org.gnome.Shell.SearchProvider2")]
    internal interface ShellSearchProvider : Object {
        /** Starts a bounded search from the supplied terms. */
        public abstract async string[] get_initial_result_set (string[] terms) throws Error;
        /** Searches again, rather than incorrectly limiting refinement to truncated results. */
        public abstract async string[] get_subsearch_result_set (string[] previous_results,
            string[] terms) throws Error;
        /** Returns metadata in the requested order for valid result identities. */
        public abstract HashTable<string, Variant>[] get_result_metas (string[] identifiers)
            throws Error;
        /** Opens the selected identity for explicit copying or inspection. */
        public abstract void activate_result (string identifier, string[] terms, uint timestamp)
            throws Error;
        /** Opens the full browser with the supplied search terms. */
        public abstract void launch_search (string[] terms, uint timestamp) throws Error;
    }

    /** Exports desktop search without constructing a window or recording passive queries. */
    internal sealed class SearchProvider : Object, ShellSearchProvider {
        /** Application owns this provider, not the reverse. */
        private weak Application application;
        /** Newest request, canceled when superseded or unexported. */
        private SearchRequest? active;
        /** Bounds concurrent canceled workers during rapid input. */
        private uint running = 0;
        /** Monotonic request generation; pending invocations exit when superseded. */
        private uint64 generation = 0;
        /** Invalidates late completions during teardown. */
        private bool closed = false;

        /** Creates the provider on the application's owning main context. */
        public SearchProvider (Application application) {
            this.application = application;
        }

        /** Cancels work without blocking the main context. */
        public void shutdown () {
            closed = true;
            generation++;
            active?.cancel ();
            active = null;
        }

        /** Executes a fresh query, even if previous results were truncated. */
        public async string[] get_initial_result_set (string[] terms) throws Error {
            return yield search (terms);
        }

        /** Uses the full repertoire on refinement and cancellation of obsolete work. */
        public async string[] get_subsearch_result_set (string[] previous_results,
                                                        string[] terms) throws Error {
            return yield search (terms);
        }

        /** Returns safe metadata without rendering glyphs or modifying the clipboard. */
        public HashTable<string, Variant>[] get_result_metas (string[] identifiers) throws Error {
            HashTable<string, Variant>[] result = {};
            for (var index = 0; index < identifiers.length && index < DesktopSearch.LIMIT;
                 index++) {
                var identifier = identifiers[index];
                var snapshot = resolve (identifier);
                if (snapshot == null) {
                    continue;
                }
                var metadata = new HashTable<string, Variant> (str_hash, str_equal);
                metadata.insert ("id", new Variant.string (identifier));
                metadata.insert ("name", new Variant.string (snapshot.name () ?? identifier));
                metadata.insert ("description", new Variant.string (identifier));
                metadata.insert ("gicon", new Variant.string (APP_ID));
                result += metadata;
            }
            return result;
        }

        /** Activates a valid character without silently replacing the user's clipboard. */
        public void activate_result (string identifier, string[] terms, uint timestamp)
            throws Error {
            if (!closed && resolve (identifier) != null) {
                application.open_identity (identifier, timestamp);
            }
        }

        /** Presents a browser query with no collection constraints. */
        public void launch_search (string[] terms, uint timestamp) throws Error {
            var query = DesktopSearch.join_terms (terms);
            if (!closed && query != null) {
                application.open_search (query, timestamp);
            }
        }

        /** Accepts only canonical, bounded, copyable code-point identities. */
        private static Inspector.Snapshot? resolve (string identifier) {
            if (identifier.length > DesktopSearch.MAX_QUERY_BYTES) {
                return null;
            }
            var snapshot = Inspector.from_code (identifier);
            if (snapshot == null || snapshot.text () == null ||
                snapshot.format (Inspector.Format.CODE) != identifier) {
                return null;
            }
            return (owned) snapshot;
        }

        /** Runs native work off-thread and publishes only the current request. */
        private async string[] search (string[] terms) {
            active?.cancel ();
            active = null;
            var ticket = ++generation;
            var query = DesktopSearch.join_terms (terms);
            if (closed || query == null) {
                return {};
            }
            var owner = application;
            owner.hold ();
            while (!closed && ticket == generation && running >= 2) {
                yield wait_for_worker ();
            }
            if (closed || ticket != generation) {
                owner.release ();
                return {};
            }
            var request = new SearchRequest (Search.Scope.ALL, -1, -1, query);
            active = request;
            running++;
            Search.Status status = Search.Status.FAILED;
            SourceFunc resume = search.callback;
            try {
                new Thread<void*>.try ("desktop-search", () => {
                    status = request.run ();
                    Idle.add (() => {
                        resume ();
                        return Source.REMOVE;
                    });
                    return null;
                });
                yield;
            } catch (Error error) {
                warning ("Desktop search worker failed: %s", error.message);
            }
            running--;
            string[] results = {};
            if (!closed && active == request) {
                active = null;
                request.publish (status);
                results = DesktopSearch.identities (request);
            }
            owner.release ();
            return results;
        }

        /** Yields the main context while canceled workers finish their native cleanup. */
        private async void wait_for_worker () {
            Timeout.add (10, () => {
                wait_for_worker.callback ();
                return Source.REMOVE;
            });
            yield;
        }
    }
}
