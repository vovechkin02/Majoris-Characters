namespace Characters {
    /** Local, bounded character collections using stable exact UTF-8 identities. */
    internal sealed class LibraryStore : Object {
        /** Maximum entries in either independent collection. */
        public const int MAX_ENTRIES = 512;
        /** Maximum scalars in an individually saved or recorded identity. */
        private const int MAX_POINTS = 32;

        /** Emitted synchronously on the main context after a collection changes. */
        public signal void changed ();

        /** Shared window settings; all operations run on the owning main context. */
        private GLib.Settings settings;
        /** Subscription to external settings changes, released on disposal. */
        private ulong settings_handler;

        /** Opens local collections and enforces the currently configured history policy. */
        public LibraryStore (GLib.Settings settings) {
            this.settings = settings;
            settings_handler = settings.changed.connect (on_settings_changed);
            enforce_history_policy ();
            // Read the keys with the observer attached so external changes are delivered.
            settings.get_strv ("favorites");
            settings.get_int ("history-limit");
        }

        /** Disconnects the settings observer before releasing the repository. */
        public override void dispose () {
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
            string[] result = {};
            var seen = new HashTable<string, bool> (str_hash, str_equal);
            foreach (var text in settings.get_strv ("favorites")) {
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
            var values = favorites ();
            foreach (var value in values) {
                if (value == text) {
                    return true;
                }
            }
            if (values.length >= MAX_ENTRIES) {
                return false;
            }
            values += text;
            return settings.set_strv ("favorites", values);
        }

        /** Removes one favorite without modifying the independent usage history. */
        public bool remove_favorite (string text) {
            string[] remaining = {};
            foreach (var value in favorites ()) {
                if (value != text) {
                    remaining += value;
                }
            }
            return settings.set_strv ("favorites", remaining);
        }

        /** Moves a favorite by one position in the unfiltered manual ordering. */
        public bool move_favorite (string text, int delta) {
            if (delta != -1 && delta != 1) {
                return false;
            }
            var values = favorites ();
            for (var index = 0; index < values.length; index++) {
                if (values[index] == text) {
                    var target = index + delta;
                    if (target < 0 || target >= values.length) {
                        return false;
                    }
                    var neighbor = values[target];
                    values[target] = text;
                    values[index] = neighbor;
                    return settings.set_strv ("favorites", values);
                }
            }
            return false;
        }

        /** Records one explicit character copy or composer insertion, newest first. */
        public bool record_use (string text) {
            if (!settings.get_boolean ("history-enabled")) {
                return true;
            }
            if (!is_valid (text)) {
                return false;
            }
            var entries = history_entries ();
            uint count = 1;
            string[] records = {};
            foreach (var entry in entries) {
                if (entry.text == text) {
                    count = entry.count == uint.MAX ? uint.MAX : entry.count + 1;
                } else {
                    records += encode (entry.text, entry.count);
                }
            }
            string[] updated = { encode (text, count) };
            var limit = settings.get_int ("history-limit");
            foreach (var record in records) {
                if (updated.length >= limit) {
                    break;
                }
                updated += record;
            }
            return settings.set_strv ("usage-history", updated);
        }

        /** Returns independent history entries, ordered by recency or stable frequency. */
        public UsageEntry[] history_entries (bool frequent = false) {
            UsageEntry[] entries = {};
            if (!settings.get_boolean ("history-enabled")) {
                return entries;
            }
            var seen = new HashTable<string, bool> (str_hash, str_equal);
            var limit = settings.get_int ("history-limit");
            foreach (var record in settings.get_strv ("usage-history")) {
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
            string[] remaining = {};
            foreach (var entry in history_entries ()) {
                if (entry.text != text) {
                    remaining += encode (entry.text, entry.count);
                }
            }
            return settings.set_strv ("usage-history", remaining);
        }

        /** Clears both history views atomically, leaving favorites untouched. */
        public bool clear_history () {
            return settings.set_strv ("usage-history", {});
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

        /** Drops history on opt-out and trims persisted records when the limit decreases. */
        private void enforce_history_policy () {
            if (!settings.get_boolean ("history-enabled")) {
                if (settings.get_strv ("usage-history").length > 0) {
                    clear_history ();
                }
                return;
            }
            string[] sanitized = {};
            foreach (var entry in history_entries ()) {
                sanitized += encode (entry.text, entry.count);
            }
            var original = settings.get_strv ("usage-history");
            if (original.length != sanitized.length) {
                settings.set_strv ("usage-history", sanitized);
            }
        }

        /** Reflects local and external writes; policy changes never restore discarded history. */
        private void on_settings_changed (string key) {
            if (key == "history-enabled" || key == "history-limit") {
                enforce_history_policy ();
                changed ();
            } else if (key == "usage-history") {
                enforce_history_policy ();
                changed ();
            } else if (key == "favorites") {
                changed ();
            }
        }
    }
}
