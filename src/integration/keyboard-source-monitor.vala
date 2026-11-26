namespace Characters {
    /** Main-context adapter for GNOME configuration and read-only IBus engine metadata. */
    internal sealed class KeyboardSourceMonitor : Object {
        /** Maximum registry call duration, in milliseconds. */
        private const int REGISTRY_TIMEOUT_MS = 1500;
        /** Optional GNOME configuration, never written by this adapter. */
        private GLib.Settings? settings;
        /** Subscription to configured input-source changes. */
        private ulong settings_handler;
        /** Latest typed configuration from the optional GNOME schema. */
        private Variant? sources;
        /** IBus connection, created lazily for configured engines only. */
        private IBus.Bus? bus;
        /** Connection subscription, disconnected during shutdown. */
        private ulong connected_handler;
        /** Disconnection subscription, disconnected during shutdown. */
        private ulong disconnected_handler;
        /** Currently outstanding registry call, or null. */
        private Cancellable? pending;
        /** Languages declared by the registry for exact configured engine identifiers. */
        private HashTable<string, string> languages = new HashTable<string, string> (
            str_hash, str_equal);
        /** Last published immutable worker snapshot. */
        private string current = "";
        /** Whether the configured-language filter is currently selected. */
        private bool active;
        /** Prevent callbacks from publishing after the owner closes. */
        private bool closed;

        /** Emitted only when the resolved writing-system operands change. */
        public signal void changed ();

        /** Read the optional schema and subscribe to changes without connecting to IBus yet. */
        public KeyboardSourceMonitor () {
            var schemas = SettingsSchemaSource.get_default ();
            if (schemas == null) {
                return;
            }
            var schema = schemas.lookup ("org.gnome.desktop.input-sources", true);
            if (schema == null || schema.get_path () == null || !schema.has_key ("sources") ||
                !schema.get_key ("sources").get_value_type ().equal (new VariantType ("a(ss)"))) {
                return;
            }
            settings = new GLib.Settings.full (schema, null, null);
            sources = settings.get_value ("sources");
            current = InputSourcePolicy.snapshot (sources, languages);
            settings_handler = settings.changed["sources"].connect (() => {
                on_sources_changed ();
            });
        }

        /** Return the latest immutable worker snapshot without performing IPC. */
        public string snapshot () {
            return current;
        }

        /** Activate registry discovery only while the configured-language filter is selected. */
        public void set_active (bool selected) {
            if (closed || active == selected) {
                return;
            }
            active = selected;
            if (active) {
                refresh_registry ();
            } else {
                cancel_pending ();
                languages.remove_all ();
                publish ();
            }
        }

        /** Cancel obsolete registry requests; completions must still release their results. */
        private void cancel_pending () {
            if (pending != null) {
                pending.cancel ();
                pending = null;
            }
        }

        /** Publish changed operands, leaving unrelated searches untouched. */
        private void publish () {
            var next = InputSourcePolicy.snapshot (sources, languages);
            if (next != current) {
                current = next;
                if (active && !closed) {
                    changed ();
                }
            }
        }

        /** Invalidate the old request and refresh only the latest configured sources. */
        private void on_sources_changed () {
            if (closed || settings == null) {
                return;
            }
            sources = settings.get_value ("sources");
            cancel_pending ();
            languages.remove_all ();
            publish ();
            if (active) {
                refresh_registry ();
            }
        }

        /** Re-resolve configured engine descriptors after a daemon reconnect. */
        private void on_connected () {
            if (active && !closed) {
                refresh_registry ();
            }
        }

        /** Discard disconnected engine metadata, preserving registered XKB operands. */
        private void on_disconnected () {
            cancel_pending ();
            languages.remove_all ();
            publish ();
        }

        /** Query the live registry with a bounded asynchronous call, never on the search worker. */
        private void refresh_registry () {
            if (closed || !active || pending != null) {
                return;
            }
            var ids = InputSourcePolicy.engine_ids (sources);
            if (ids.size () == 0) {
                return;
            }
            if (bus == null) {
                IBus.init ();
                bus = new IBus.Bus.async ();
                connected_handler = bus.connected.connect (on_connected);
                disconnected_handler = bus.disconnected.connect (on_disconnected);
            }
            if (!bus.is_connected ()) {
                return;
            }
            var request = new Cancellable ();
            pending = request;
            var registry = bus;
            registry.list_engines_async.begin (REGISTRY_TIMEOUT_MS, request, (object, result) => {
                var resolved = new HashTable<string, string> (str_hash, str_equal);
                try {
                    // IBus returns an allocated list of referenced descriptors, despite the
                    // historical transfer-container annotation. Own both through the binding.
                    var engines = IBusRegistry.finish_engines (registry, result);
                    foreach (unowned IBus.EngineDesc engine in engines) {
                        unowned string name = engine.get_name ();
                        if (!ids.contains (name)) {
                            continue;
                        }
                        var language = InputSourcePolicy.engine_language (engine.get_language ());
                        if (language != null) {
                            resolved.insert (name, language);
                        }
                    }
                } catch (Error error) {
                    // A missing, stopped or unresponsive daemon is not an application error.
                    // Publish only XKB operands if this is still the active request.
                }
                if (closed || pending != request || request.is_cancelled ()) {
                    return;
                }
                pending = null;
                languages = resolved;
                publish ();
            });
        }

        /** Disconnect observers and cancel IPC before the window releases this adapter. */
        public void shutdown () {
            if (closed) {
                return;
            }
            closed = true;
            active = false;
            cancel_pending ();
            if (settings != null && settings_handler != 0) {
                settings.disconnect (settings_handler);
                settings_handler = 0;
            }
            if (bus != null) {
                if (connected_handler != 0) {
                    bus.disconnect (connected_handler);
                    connected_handler = 0;
                }
                if (disconnected_handler != 0) {
                    bus.disconnect (disconnected_handler);
                    disconnected_handler = 0;
                }
            }
            bus = null;
            settings = null;
        }
    }
}
