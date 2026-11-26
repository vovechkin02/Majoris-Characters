using Characters;

/** Construct the exact GNOME input-source tuple type without accessing desktop settings. */
private Variant make_sources (string[] kinds, string[] ids) {
    assert (kinds.length == ids.length);
    var builder = new VariantBuilder (new VariantType ("a(ss)"));
    for (var index = 0; index < kinds.length; index++) {
        builder.add_value (new Variant ("(ss)", kinds[index], ids[index]));
    }
    return builder.end ();
}

/** Verify source-kind separation, exact engine lookup and canonical bounded locale operands. */
private void test_snapshot () {
    string[] kinds = { "xkb", "ibus", "ibus", "ibus", "ibus", "ibus", "xkb", "ibus",
                       "unknown", "xkb", "xkb", "ibus", "ibus" };
    string[] ids = { "us+intl", "mozc-jp", "hangul", "pinyin", "m17n:hi:itrans", "neutral",
                     "ru", "missing-ja", "mozc-jp", "ibus:el", "bad,ru", "MOZC-JP", "dupe" };
    var sources = make_sources (kinds, ids);
    var languages = new HashTable<string, string> (str_hash, str_equal);
    languages.insert ("mozc-jp", "ja");
    languages.insert ("hangul", "ko_KR.UTF-8");
    languages.insert ("pinyin", "zh-Hant");
    languages.insert ("m17n:hi:itrans", "hi");
    languages.insert ("neutral", "other");
    languages.insert ("dupe", "JA");
    var engines = InputSourcePolicy.engine_ids (sources);
    assert (engines.contains ("mozc-jp") && engines.contains ("missing-ja"));
    assert (!engines.contains ("us+intl") && !engines.contains ("bad,ru"));
    assert (InputSourcePolicy.snapshot (sources, languages) ==
        "us+intl,ibus:ja,ibus:ko_kr,ibus:zh_hant,ibus:hi,ru");
    languages.remove_all ();
    assert (InputSourcePolicy.snapshot (sources, languages) == "us+intl,ru");
    assert (InputSourcePolicy.snapshot (new Variant.string ("malformed"), languages) == "");
    assert (InputSourcePolicy.snapshot (null, languages) == "");
    assert (InputSourcePolicy.engine_ids (null).size () == 0);
}

/** Reject neutral languages, malformed tags, invalid identifiers and injected delimiters. */
private void test_validation () {
    assert (InputSourcePolicy.engine_language ("zh-Hant") == "zh_hant");
    assert (InputSourcePolicy.engine_language ("JA") == "ja");
    assert (InputSourcePolicy.engine_language ("sr-Latn") == "sr_latn");
    assert (InputSourcePolicy.engine_language ("ko_KR.UTF-8") == "ko_kr");
    assert (InputSourcePolicy.engine_language ("jpn") == "jpn");
    foreach (var invalid in new string[] { "", "C", "other", "und", "und-Latn", "mul",
        "mis", "zxx", "en_Zzzz", "en_Zyyy", "en_Zinh", "ja,,ru", "ja:ru", "ja__JP",
        "ja_", "ja@unknown", ".UTF-8", "Latin", "éé", string.nfill (64, 'a') }) {
        assert (InputSourcePolicy.engine_language (invalid) == null);
    }
    var languages = new HashTable<string, string> (str_hash, str_equal);
    languages.insert ("bad,engine", "ja");
    languages.insert ("bad engine", "ja");
    languages.insert ("good", "und");
    var sources = make_sources (new string[] { "ibus", "ibus", "ibus", "ibus" },
        new string[] { "bad,engine", "bad engine", "good", string.nfill (256, 'a') });
    assert (InputSourcePolicy.snapshot (sources, languages) == "");
    assert (InputSourcePolicy.engine_ids (sources).size () == 1);
}

/** Bound snapshots without truncating an operand or walking an unbounded configuration. */
private void test_bounds () {
    var builder = new VariantBuilder (new VariantType ("a(ss)"));
    var languages = new HashTable<string, string> (str_hash, str_equal);
    for (var index = 0; index < 300; index++) {
        var id = "engine-%d".printf (index);
        languages.insert (id, "ja_%03d".printf (index));
        builder.add_value (new Variant ("(ss)", "ibus", id));
    }
    var sources = builder.end ();
    var engines = InputSourcePolicy.engine_ids (sources);
    assert (engines.size () == 256 && !engines.contains ("engine-256"));
    var snapshot = InputSourcePolicy.snapshot (sources, languages);
    assert (snapshot.length > 0 && snapshot.length <= InputSourcePolicy.MAX_SNAPSHOT_BYTES);
    foreach (var operand in snapshot.split (",")) {
        assert (operand.has_prefix (InputSourcePolicy.IBUS_PREFIX) && operand.length == 11);
    }
}

/** Execute a real worker request with an immutable resolved source snapshot. */
private SearchRequest search (string sources) {
    var request = new SearchRequest (Search.Scope.LANGUAGE, 1, -1, "", sources);
    var status = request.run ();
    request.publish (status);
    assert (status == Search.Status.OK);
    return request;
}

/** Test exact scalar membership in a completed repertoire. */
private bool contains (SearchRequest request, uint32 point) {
    for (uint index = 0; index < request.count (); index++) {
        if (request.id (index) == point) {
            return true;
        }
    }
    return false;
}

/** Exercise actual prepared search masks, without requiring an installed or running IBus daemon. */
private void test_repertoires () {
    var request = search ("ibus:ja");
    assert (contains (request, 0x3042) && contains (request, 0x30A2));
    assert (contains (request, 0x4E2D) && !contains (request, 0x0410));
    assert (!contains (request, 0x0041));
    request = search ("us,ru,ibus:ko,ibus:zh_hant,ibus:hi");
    foreach (var point in new uint32[] { 0x0041, 0x0410, 0xAC00, 0x4E2D, 0x3105, 0x0915 }) {
        assert (contains (request, point));
    }
    assert (!contains (request, 0x3042));
    request = search ("ibus:sr_latn");
    assert (contains (request, 0x0041) && !contains (request, 0x0410));
    request = search ("ibus:und,ibus:other,ibus:zz,unknown-layout");
    assert (request.count () == 0);
    request = search ("us,ru");
    assert (contains (request, 0x0041) && contains (request, 0x0410));
    assert (!contains (request, 0x3042));
    request = search (string.nfill (1024, 'a'));
    assert (request.count () == 0);
}

/** Wait for a bounded memory-backend settings notification on the main context. */
private void expect_snapshot (KeyboardSourceMonitor monitor, string expected) {
    var context = MainContext.default ();
    var deadline = get_monotonic_time () + 1000000;
    while (monitor.snapshot () != expected && get_monotonic_time () < deadline) {
        context.iteration (false);
        Thread.usleep (1000);
    }
    assert (monitor.snapshot () == expected);
}

/** Exercise live GSettings refresh and shutdown without touching the real desktop or IBus. */
private void test_monitor () {
    var settings = new GLib.Settings ("org.gnome.desktop.input-sources");
    assert (settings.set_value ("sources", make_sources (new string[] { "xkb" },
        new string[] { "us" })));
    var monitor = new KeyboardSourceMonitor ();
    assert (monitor.snapshot () == "us");
    uint publications = 0;
    monitor.changed.connect (() => {
        publications++;
    });
    monitor.set_active (true);
    assert (settings.set_value ("sources", make_sources (new string[] { "xkb" },
        new string[] { "ru" })));
    expect_snapshot (monitor, "ru");
    assert (publications == 1);
    monitor.set_active (false);
    assert (settings.set_value ("sources", make_sources (new string[] { "xkb" },
        new string[] { "us+intl" })));
    expect_snapshot (monitor, "us+intl");
    assert (publications == 1);
    monitor.set_active (true);
    monitor.shutdown ();
    monitor.shutdown ();
    assert (settings.set_value ("sources", make_sources (new string[] { "xkb" },
        new string[] { "us" })));
    var context = MainContext.default ();
    while (context.pending ()) {
        context.iteration (false);
    }
    assert (monitor.snapshot () == "us+intl" && publications == 1);
}

/** Register deterministic policy and end-to-end worker contracts. */
int main (string[] args) {
    // Also isolate manual runs of this test executable from the real settings backend.
    Environment.set_variable ("GSETTINGS_BACKEND", "memory", true);
    Test.init (ref args);
    Test.add_func ("/characters/input-sources/snapshot", test_snapshot);
    Test.add_func ("/characters/input-sources/validation", test_validation);
    Test.add_func ("/characters/input-sources/bounds", test_bounds);
    Test.add_func ("/characters/input-sources/repertoires", test_repertoires);
    Test.add_func ("/characters/input-sources/monitor", test_monitor);
    return Test.run ();
}
