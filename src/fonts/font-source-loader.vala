namespace Characters {
    /** Runs at most one discovery or mapping worker and suppresses superseded publication. */
    internal sealed class FontSourceLoader : Object {
        /** Delivers only the latest completed source request on the default main context. */
        public signal void completed (FontSourceRequest request, bool success);

        /** Currently running native operation, retained until its idle completion. */
        private FontSourceRequest? active;
        /** Latest requested source, replacing obsolete queued operations. */
        private FontSourceRequest? pending;
        /** Blocks publication if active work is superseded without a replacement. */
        private bool discard_active = false;
        /** Permanently prevents submission or callbacks after dialog shutdown. */
        private bool closed = false;

        /** Queues the newest source request; the active native call is allowed to finish. */
        public void submit (FontSourceRequest request) {
            if (closed) {
                return;
            }
            pending = request;
            start_pending ();
        }

        /** Clears queued work and discards completion from any uninterruptible native call. */
        public void cancel () {
            pending = null;
            discard_active = true;
        }

        /** Prevents subsequent publication without joining a worker on GTK's main context. */
        public void shutdown () {
            closed = true;
            cancel ();
        }

        /** Returns true only when no worker or queued request retains source state. */
        public bool is_idle () {
            return active == null && pending == null;
        }

        /** Starts one worker, retaining its request until main-context delivery. */
        private void start_pending () {
            if (closed || active != null || pending == null) {
                return;
            }
            FontSourceRequest request = pending;
            pending = null;
            active = request;
            discard_active = false;
            try {
                new Thread<void*>.try ("font-source", () => {
                    var success = request.run ();
                    Idle.add (() => {
                        finish (request, success);
                        return Source.REMOVE;
                    });
                    return null;
                });
            } catch (Error error) {
                finish (request, false);
            }
        }

        /** Releases active ownership before publishing or starting a replacement source. */
        private void finish (FontSourceRequest request, bool success) {
            active = null;
            if (closed) {
                return;
            }
            if (pending != null) {
                start_pending ();
                return;
            }
            if (!discard_active) {
                completed (request, success);
            }
        }
    }
}
