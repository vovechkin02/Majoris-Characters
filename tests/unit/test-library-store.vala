using Characters;

/** Resets the memory backend before each independent collection test. */
private GLib.Settings fresh_settings () {
    var settings = new GLib.Settings (APP_ID);
    foreach (var key in new string[] { "favorites", "usage-history", "history-enabled",
        "history-limit", "grid-preview-size", "font-preview-size", "font-fallback" }) {
        settings.reset (key);
    }
    return settings;
}

/** Exact Unicode identities remain distinct, ordered, deduplicated, and independently removable. */
private void test_favorites () {
    var settings = fresh_settings ();
    var library = new LibraryStore (settings);
    assert (library.add_favorite ("é"));
    assert (library.add_favorite ("e\u0301"));
    assert (library.add_favorite ("👩🏽‍💻"));
    assert (library.add_favorite ("é"));
    assert (!library.add_favorite (""));
    var values = library.favorites ();
    assert (values.length == 3 && values[0] == "é" && values[1] == "e\u0301");
    assert (library.move_favorite ("👩🏽‍💻", -1));
    assert (!library.move_favorite ("é", -1));
    assert (library.remove_favorite ("é"));
    var reopened = new LibraryStore (new GLib.Settings (APP_ID));
    values = reopened.favorites ();
    assert (values.length == 2 && values[0] == "👩🏽‍💻" && values[1] == "e\u0301");
}

/** Frequency uses saturating counts and recency as a tie breaker; removal affects both views. */
private void test_history_order () {
    var settings = fresh_settings ();
    var library = new LibraryStore (settings);
    assert (library.record_use ("€"));
    assert (library.record_use ("👍🏽"));
    assert (library.record_use ("€"));
    assert (library.record_use (":"));
    var recent = library.history_entries ();
    var frequent = library.history_entries (true);
    assert (recent.length == 3 && recent[0].text == ":" && recent[1].text == "€");
    assert (frequent[0].text == "€" && frequent[0].count == 2);
    assert (frequent[1].text == ":" && frequent[2].text == "👍🏽");
    assert (library.remove_history ("€"));
    assert (library.history_entries (true).length == 2);
    assert (library.add_favorite ("👍🏽"));
    assert (library.clear_history ());
    assert (library.history_entries ().length == 0 && library.favorites ().length == 1);
}

/** Opt-out clears history, limit reductions evict oldest entries, and favorites remain independent. */
private void test_privacy () {
    var settings = fresh_settings ();
    var library = new LibraryStore (settings);
    assert (library.add_favorite ("A"));
    assert (library.record_use ("A") && library.record_use ("B") && library.record_use ("C"));
    assert (library.set_history_limit (1));
    assert (library.history_entries ().length == 1);
    assert (settings.get_strv ("usage-history").length == 1);
    assert (library.set_history_enabled (false));
    assert (library.record_use ("D"));
    assert (library.history_entries ().length == 0);
    assert (settings.get_strv ("usage-history").length == 0);
    assert (library.set_history_enabled (true));
    assert (library.history_entries ().length == 0 && library.favorites ()[0] == "A");
}

/** Rejects malformed saved values, deduplicates external records, and prevents count overflow. */
private void test_invalid_records () {
    var settings = fresh_settings ();
    assert (settings.set_strv ("favorites", { "", "A", "A", "👩‍💻" }));
    assert (settings.set_strv ("usage-history", { "bad", "0:A", "4294967296:B",
        "4294967295:C", "1:C", "3::", "2:" }));
    var library = new LibraryStore (settings);
    assert (library.favorites ().length == 2);
    assert (library.history_entries ().length == 2);
    assert (library.record_use ("C"));
    assert (library.history_entries ()[0].count == uint.MAX);
    assert (!LibraryStore.is_valid (string.nfill (33, 'a')));
}

/** Wait for queued observer-driven cleanup after an external settings edit. */
private void test_external_policy () {
    var settings = fresh_settings ();
    var library = new LibraryStore (settings);
    assert (library.record_use ("A") && library.record_use ("B"));
    assert (settings.set_int ("history-limit", 1));
    while (MainContext.default ().pending ()) {
        MainContext.default ().iteration (false);
    }
    assert (settings.get_strv ("usage-history").length == 1);
    assert (settings.set_boolean ("history-enabled", false));
    assert (library.history_entries ().length == 0);
    while (MainContext.default ().pending ()) {
        MainContext.default ().iteration (false);
    }
    assert (settings.get_strv ("usage-history").length == 0);
}

/** Number of distinct writes and shared-use increments by each concurrent worker. */
private const int WORKER_WRITES = 8;
/** Number of independent worker processes started by the integration driver. */
private const int WORKER_COUNT = 4;

/** Signal that the store has read its original data, then wait for the start barrier. */
private void await_barrier (string ready, string start) {
    try {
        FileUtils.set_contents (ready, "ready");
    } catch (FileError error) {
        assert_not_reached ();
    }
    int64 deadline = get_monotonic_time () + 10000000;
    while (!FileUtils.test (start, FileTest.EXISTS) && get_monotonic_time () < deadline) {
        Thread.usleep (1000);
    }
    assert (FileUtils.test (start, FileTest.EXISTS));
}

/** Exercise multiple writers that opened their settings before any concurrent writes. */
private void concurrent_worker (string id, string ready, string start) {
    var library = new LibraryStore (new GLib.Settings (APP_ID));
    assert (library.favorites ().length == 2);
    assert (library.history_entries ().length == 1);
    await_barrier (ready, start);
    for (var index = 0; index < WORKER_WRITES; index++) {
        assert (library.add_favorite ("worker-%s-%d".printf (id, index)));
        assert (library.record_use ("shared"));
    }
    if (id == "0") {
        assert (library.remove_favorite ("seed-remove"));
    }
}

/** Confirm every successful addition and increment survived, in per-worker order. */
private void verify_concurrent (LibraryStore library) {
    var values = library.favorites ();
    assert (values.length == 1 + WORKER_COUNT * WORKER_WRITES);
    assert (values[0] == "seed-keep");
    for (var worker = 0; worker < WORKER_COUNT; worker++) {
        int previous = 0;
        for (var index = 0; index < WORKER_WRITES; index++) {
            var identity = "worker-%d-%d".printf (worker, index);
            int found = -1;
            for (var position = 1; position < values.length; position++) {
                if (values[position] == identity) {
                    found = position;
                    break;
                }
            }
            assert (found > previous);
            previous = found;
        }
    }
    var entries = library.history_entries ();
    assert (entries.length == 2 && entries[0].text == "shared");
    assert (entries[0].count == WORKER_COUNT * WORKER_WRITES);
    assert (entries[1].text == "old" && entries[1].count == 1);
}

/** Dispatch subprocess operations used by the isolated persistence integration driver. */
private int process_mode (string[] args) {
    if (args[1] == "--worker" && args.length == 5) {
        concurrent_worker (args[2], args[3], args[4]);
        return 0;
    }
    if (args[1] == "--seed" || args[1] == "--prepare-concurrent") {
        var settings = fresh_settings ();
        var library = new LibraryStore (settings);
        if (args[1] == "--seed") {
            assert (library.add_favorite ("👩🏽‍💻") && library.record_use ("e\u0301"));
            assert (library.record_use ("e\u0301"));
            // Use a fresh snapshot so the keyfile test never writes stale unrelated keys.
            var fresh = Collections.snapshot (settings);
            assert (fresh != null && fresh.set_int ("grid-preview-size", 40));
        } else {
            assert (library.add_favorite ("seed-remove") && library.add_favorite ("seed-keep"));
            assert (library.record_use ("old"));
        }
        GLib.Settings.sync ();
        return 0;
    }
    var settings = new GLib.Settings (APP_ID);
    var library = new LibraryStore (settings);
    switch (args[1]) {
        case "--verify":
            assert (library.favorites ()[0] == "👩🏽‍💻");
            assert (library.history_entries ()[0].text == "e\u0301");
            assert (library.history_entries ()[0].count == 2);
            assert (settings.get_int ("grid-preview-size") == 40);
            return 0;
        case "--verify-concurrent":
            verify_concurrent (library);
            return 0;
        case "--stale-writer":
            assert (args.length == 4 && library.history_enabled ());
            assert (library.history_entries ().length == 2);
            await_barrier (args[2], args[3]);
            assert (library.record_use ("should-not-return"));
            return 0;
        case "--disable":
            assert (library.set_history_enabled (false));
            return 0;
        case "--verify-disabled":
            assert (!library.history_enabled () && library.history_entries ().length == 0);
            assert (settings.get_strv ("usage-history").length == 0);
            assert (library.favorites ().length == 1 + WORKER_COUNT * WORKER_WRITES);
            return 0;
        case "--reenable":
            assert (library.set_history_enabled (true));
            assert (library.history_entries ().length == 0);
            assert (library.record_use ("new") && library.record_use ("newer"));
            assert (library.set_history_limit (1));
            return 0;
        case "--verify-trimmed":
            assert (library.history_enabled () && library.history_limit () == 1);
            assert (library.history_entries ().length == 1);
            assert (library.history_entries ()[0].text == "newer");
            return 0;
        case "--expect-lock-failure":
            assert (!library.add_favorite ("locked-write"));
            return 0;
        default:
            return 2;
    }
}

/** Register isolated unit tests or delegate to a subprocess operation. */
int main (string[] args) {
    if (args.length > 1) {
        switch (args[1]) {
            case "--seed":
            case "--verify":
            case "--prepare-concurrent":
            case "--worker":
            case "--verify-concurrent":
            case "--stale-writer":
            case "--disable":
            case "--verify-disabled":
            case "--reenable":
            case "--verify-trimmed":
            case "--expect-lock-failure":
                return process_mode (args);
        }
    }
    Environment.set_variable ("GSETTINGS_BACKEND", "memory", true);
    Test.init (ref args);
    Test.add_func ("/characters/library/favorites", test_favorites);
    Test.add_func ("/characters/library/history-order", test_history_order);
    Test.add_func ("/characters/library/privacy", test_privacy);
    Test.add_func ("/characters/library/invalid-records", test_invalid_records);
    Test.add_func ("/characters/library/external-policy", test_external_policy);
    return Test.run ();
}
