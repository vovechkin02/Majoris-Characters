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
    assert (settings.set_int ("history-limit", 1));
    assert (library.history_entries ().length == 1);
    assert (settings.get_strv ("usage-history").length == 1);
    assert (settings.set_boolean ("history-enabled", false));
    assert (library.record_use ("D"));
    assert (library.history_entries ().length == 0);
    assert (settings.get_strv ("usage-history").length == 0);
    assert (settings.set_boolean ("history-enabled", true));
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

/** Registers isolated headless tests or verifies actual keyfile persistence across processes. */
int main (string[] args) {
    if (args.length == 2 && args[1] == "--seed") {
        var settings = fresh_settings ();
        var library = new LibraryStore (settings);
        assert (library.add_favorite ("👩🏽‍💻") && library.record_use ("e\u0301"));
        assert (library.record_use ("e\u0301"));
        assert (settings.set_int ("grid-preview-size", 40));
        GLib.Settings.sync ();
        return 0;
    }
    if (args.length == 2 && args[1] == "--verify") {
        var settings = new GLib.Settings (APP_ID);
        var library = new LibraryStore (settings);
        assert (library.favorites ()[0] == "👩🏽‍💻");
        assert (library.history_entries ()[0].text == "e\u0301");
        assert (library.history_entries ()[0].count == 2);
        assert (settings.get_int ("grid-preview-size") == 40);
        return 0;
    }
    Environment.set_variable ("GSETTINGS_BACKEND", "memory", true);
    Test.init (ref args);
    Test.add_func ("/characters/library/favorites", test_favorites);
    Test.add_func ("/characters/library/history-order", test_history_order);
    Test.add_func ("/characters/library/privacy", test_privacy);
    Test.add_func ("/characters/library/invalid-records", test_invalid_records);
    return Test.run ();
}
