namespace Characters {
    /** Local, bounded character collections using stable exact UTF-8 identities. */
    internal sealed class LibraryStore : Object {
        /** Maximum entries in either independent collection. */
        public const int MAX_ENTRIES = 512;
        /** Maximum scalars in an individually saved or recorded identity. */
        private const int MAX_POINTS = 32;
        /** Maximum attempts for one externally triggered cleanup. */
        private const uint POLICY_RETRY_LIMIT = 3;
        /** Delay between cleanup attempts, in milliseconds. */
        private const uint POLICY_RETRY_MS = 100;

        /** Emitted synchronously on the main context after a collection changes. */
        public signal void changed ();

        /** Shared window settings; all operations run on the owning main context. */
        private GLib.Settings settings;
        /** Subscription to external settings changes, released on disposal. */
        private ulong settings_handler;
        /** Coalesced cleanup after external policy changes, never nested inside a commit. */
        private uint policy_source = 0;
        /** Bounded retries for policy cleanup blocked by another writer. */
        private uint policy_attempts = 0;
        /** Suppresses this store's observer while it commits a coordinated update. */
        private bool committing = false;

        /** Stages one mutation on an isolated delayed settings object. */
        private delegate bool Mutation (GLib.Settings target);

        /** Opens local collections and enforces the currently configured history policy. */
        public LibraryStore (GLib.Settings settings) {
            this.settings = settings;
            settings_handler = settings.changed.connect (on_settings_changed);
            // Read the keys with the observer attached so external changes are delivered.
            settings.get_strv ("favorites");
            settings.get_strv ("usage-history");
            settings.get_boolean ("history-enabled");
            settings.get_int ("history-limit");
            if (!enforce_history_policy ()) {
                schedule_policy ();
            }
        }

        /** Disconnects the settings observer before releasing the repository. */
        public override void dispose () {
            if (policy_source != 0) {
                Source.remove (policy_source);
                policy_source = 0;
            }
            if (settings_handler != 0) {
                settings.disconnect (settings_handler);
                settings_handler = 0;
            }
            base.dispose ();
        }

        /** Accepts nonempty, valid, bounded text without normalization or trimming. */
        public static bool is_valid (string text) {
            return text.length > 0 && text.length <= MAX_POINTS * 4 &&
                text.validate () && text.char_count () <= MAX_POINTS;
        }

        /** Returns unique valid favorites in their manually selected order. */
        public string[] favorites () {
            var snapshot = Collections.snapshot (settings);
            return favorites_from (snapshot ?? settings);
        }

        /** Read and sanitize favorites from the current transaction or display snapshot. */
        private static string[] favorites_from (GLib.Settings source) {
            string[] result = {};
            var seen = new HashTable<string, bool> (str_hash, str_equal);
            foreach (var text in source.get_strv ("favorites")) {
                if (is_valid (text) && !seen.contains (text)) {
                    seen.insert (text, true);
                    result += text;
                }
                if (result.length == MAX_ENTRIES) {
                    break;
                }
            }
            return result;
        }

        /** Saves one exact identity; duplicate additions succeed without changing order. */
        public bool add_favorite (string text) {
            if (!is_valid (text)) {
                return false;
            }
            return mutate ((target) => {
                var values = favorites_from (target);
                foreach (var value in values) {
                    if (value == text) {
                        return true;
                    }
                }
                if (values.length >= MAX_ENTRIES) {
                    return false;
                }
                values += text;
                return target.set_strv ("favorites", values);
            });
        }

        /** Removes one favorite without modifying the independent usage history. */
        public bool remove_favorite (string text) {
            return mutate ((target) => {
                string[] remaining = {};
                foreach (var value in favorites_from (target)) {
                    if (value != text) {
                        remaining += value;
                    }
                }
                return target.set_strv ("favorites", remaining);
            });
        }

        /** Moves a favorite by one position in the latest unfiltered manual ordering. */
        public bool move_favorite (string text, int delta) {
            if (delta != -1 && delta != 1) {
                return false;
            }
            return mutate ((target) => {
                var values = favorites_from (target);
                for (var index = 0; index < values.length; index++) {
                    if (values[index] == text) {
                        var position = index + delta;
                        if (position < 0 || position >= values.length) {
                            return false;
                        }
                        var neighbor = values[position];
                        values[position] = text;
                        values[index] = neighbor;
                        return target.set_strv ("favorites", values);
                    }
                }
                return false;
            });
        }

        /** Records one explicit character copy or composer insertion, newest first. */
        public bool record_use (string text) {
            return mutate ((target) => {
                if (!target.get_boolean ("history-enabled")) {
                    return sanitize_history (target);
                }
                if (!is_valid (text)) {
                    return false;
                }
                uint count = 1;
                string[] records = {};
                foreach (var entry in history_from (target)) {
                    if (entry.text == text) {
                        count = entry.count == uint.MAX ? uint.MAX : entry.count + 1;
                    } else {
                        records += encode (entry.text, entry.count);
                    }
                }
                string[] updated = { encode (text, count) };
                var limit = target.get_int ("history-limit");
                foreach (var record in records) {
                    if (updated.length >= limit) {
                        break;
                    }
                    updated += record;
                }
                return target.set_strv ("usage-history", updated);
            });
        }

        /** Returns independent history entries, ordered by recency or stable frequency. */
        public UsageEntry[] history_entries (bool frequent = false) {
            var snapshot = Collections.snapshot (settings);
            return snapshot != null ? history_from (snapshot, frequent) : new UsageEntry[0];
        }

        /** Decode the latest bounded history without altering the selected settings object. */
        private static UsageEntry[] history_from (GLib.Settings source, bool frequent = false) {
            UsageEntry[] entries = {};
            if (!source.get_boolean ("history-enabled")) {
                return entries;
            }
            var seen = new HashTable<string, bool> (str_hash, str_equal);
            var limit = source.get_int ("history-limit");
            foreach (var record in source.get_strv ("usage-history")) {
                var entry = decode (record);
                if (entry != null && !seen.contains (entry.text)) {
                    seen.insert (entry.text, true);
                    entries += entry;
                }
                if (entries.length >= limit) {
                    break;
                }
            }
            if (frequent) {
                // Stable insertion sort preserves recency for equal usage counts.
                for (var index = 1; index < entries.length; index++) {
                    var entry = entries[index];
                    var position = index;
                    while (position > 0 && entries[position - 1].count < entry.count) {
                        entries[position] = entries[position - 1];
                        position--;
                    }
                    entries[position] = entry;
                }
            }
            return entries;
        }

        /** Removes a history identity from both recency and frequency views. */
        public bool remove_history (string text) {
            return mutate ((target) => {
                string[] remaining = {};
                foreach (var entry in history_from (target)) {
                    if (entry.text != text) {
                        remaining += encode (entry.text, entry.count);
                    }
                }
                return target.set_strv ("usage-history", remaining);
            });
        }

        /** Clears both history views atomically, leaving favorites untouched. */
        public bool clear_history () {
            return mutate ((target) => target.set_strv ("usage-history", {}));
        }

        /** Returns the current recording policy, including another process's committed changes. */
        public bool history_enabled () {
            var snapshot = Collections.snapshot (settings);
            return snapshot != null && snapshot.get_boolean ("history-enabled");
        }

        /** Returns the current history retention limit. */
        public int history_limit () {
            var snapshot = Collections.snapshot (settings);
            return (snapshot ?? settings).get_int ("history-limit");
        }

        /** Changes recording and removes history in the same delayed settings transaction. */
        public bool set_history_enabled (bool enabled) {
            bool cleared = true;
            var saved = mutate ((target) => {
                // Never restore records left behind by a previously failed privacy clear.
                if ((!enabled || !target.get_boolean ("history-enabled")) &&
                    target.get_strv ("usage-history").length > 0) {
                    cleared = target.set_strv ("usage-history", {});
                    if (enabled && !cleared) {
                        return false;
                    }
                }
                // An administrator-locked history key must not prevent opting out of recording.
                return target.set_boolean ("history-enabled", enabled);
            });
            return saved && cleared;
        }

        /** Changes the retention limit and trims older identities in one transaction. */
        public bool set_history_limit (int limit) {
            if (limit < 1 || limit > MAX_ENTRIES) {
                return false;
            }
            return mutate ((target) => target.set_int ("history-limit", limit) &&
                sanitize_history (target));
        }

        /** Encodes a record using a bounded decimal prefix and the unchanged text suffix. */
        private static string encode (string text, uint count) {
            return "%u:%s".printf (count, text);
        }

        /** Rejects malformed counts and oversized or invalid UTF-8 record identities. */
        private static UsageEntry? decode (string record) {
            var separator = record.index_of (":");
            if (separator < 1 || separator > 10 || record.length > MAX_POINTS * 4 + 11) {
                return null;
            }
            uint64 count = 0;
            for (var index = 0; index < separator; index++) {
                var digit = record[index];
                if (digit < '0' || digit > '9') {
                    return null;
                }
                count = count * 10 + (uint64) (digit - '0');
            }
            var text = record.substring (separator + 1);
            if (count == 0 || count > uint.MAX || !is_valid (text)) {
                return null;
            }
            return new UsageEntry (text, (uint) count);
        }

        /** Encode the bounded, deduplicated history under the selected privacy policy. */
        private static string[] canonical_history (GLib.Settings target) {
            string[] sanitized = {};
            foreach (var entry in history_from (target)) {
                sanitized += encode (entry.text, entry.count);
            }
            return sanitized;
        }

        /** Test exact serialized equality without normalizing stored text. */
        private static bool records_equal (string[] first, string[] second) {
            if (first.length != second.length) {
                return false;
            }
            for (var index = 0; index < first.length; index++) {
                if (first[index] != second[index]) {
                    return false;
                }
            }
            return true;
        }

        /** Remove disabled, invalid or excess history without writing an unchanged array. */
        private static bool sanitize_history (GLib.Settings target) {
            var sanitized = canonical_history (target);
            return records_equal (target.get_strv ("usage-history"), sanitized) ||
                target.set_strv ("usage-history", sanitized);
        }

        /** Serialize the complete read/modify/apply/sync/verify operation across pickers. */
        private bool mutate (Mutation operation) {
            if (committing) {
                return false;
            }
            Collections.Lock? guard = Collections.acquire (settings);
            if (guard == null) {
                return false;
            }
            GLib.Settings.sync ();
            var target = Collections.snapshot (settings);
            if (target == null) {
                return false;
            }
            committing = true;
            target.delay ();
            var success = operation (target);
            bool modified = success && target.get_has_unapplied ();
            if (modified) {
                // Verify committed values because GSettings.apply/sync return no write status.
                string[] keys = { "favorites", "usage-history", "history-enabled", "history-limit" };
                var expected = new Variant[keys.length];
                for (var index = 0; index < keys.length; index++) {
                    expected[index] = target.get_value (keys[index]);
                }
                target.apply ();
                GLib.Settings.sync ();
                var committed = Collections.snapshot (settings);
                success = committed != null;
                if (committed != null) {
                    for (var index = 0; index < keys.length; index++) {
                        if (!expected[index].equal (committed.get_value (keys[index]))) {
                            success = false;
                        }
                    }
                }
            } else {
                target.revert ();
            }
            committing = false;
            guard = null;
            if (modified) {
                changed ();
            }
            return success;
        }

        /** Drops history on opt-out and trims persisted records when the limit decreases. */
        private bool enforce_history_policy () {
            var snapshot = Collections.snapshot (settings);
            if (snapshot == null) {
                return false;
            }
            if (records_equal (snapshot.get_strv ("usage-history"),
                canonical_history (snapshot))) {
                return true;
            }
            // A dirty snapshot is only a hint; recompute after acquiring the lock.
            return mutate ((target) => sanitize_history (target));
        }

        /** Defer observer repair outside the commit stack, retrying bounded failures. */
        private void schedule_policy () {
            if (policy_source == 0) {
                if (policy_attempts == 0) {
                    policy_source = Idle.add (repair_policy);
                } else {
                    policy_source = Timeout.add (POLICY_RETRY_MS, repair_policy);
                }
            }
        }

        /** Perform one coalesced repair attempt and retry only within the configured bound. */
        private bool repair_policy () {
            policy_source = 0;
            if (enforce_history_policy ()) {
                policy_attempts = 0;
            } else if (++policy_attempts < POLICY_RETRY_LIMIT) {
                schedule_policy ();
            } else {
                policy_attempts = 0;
            }
            return Source.REMOVE;
        }

        /** Reflect local and external changes without acquiring a nested transaction lock. */
        private void on_settings_changed (string key) {
            if (committing) {
                return;
            }
            if (key == "history-enabled" || key == "history-limit" || key == "usage-history") {
                schedule_policy ();
                changed ();
            } else if (key == "favorites") {
                changed ();
            }
        }
    }
}
