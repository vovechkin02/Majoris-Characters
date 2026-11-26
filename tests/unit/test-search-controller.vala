using Characters;

/** Maximum main-context polling duration for a worker completion, in microseconds. */
private const int64 WAIT_TIMEOUT_US = 10000000;
/** Poll interval while allowing the main context to dispatch completion, in microseconds. */
private const ulong POLL_INTERVAL_US = 1000;

/** Native result ownership and catalog identifiers survive the Vala boundary. */
private void test_bindings () {
    var job = QueryParser.create (Search.Scope.ALL, -1, -1, "U+20AC");
    assert (job != null);
    assert (job.run () == Search.Status.OK);
    assert (job.count () == 1);
    assert (job.id (0) == 0x20AC);
    assert (Search.text (job.id (0)) == "€");
    assert (Search.name (job.id (0)) == "EURO SIGN");
    assert (Search.code (job.id (0)) == "U+20AC");
    assert (Search.display (0x0301) == "◌́");
    assert (Search.text (0) == null);
    assert (Search.filter_count (Search.Scope.BLOCK) == Unicode.block_count ());
}

/** Exercise the Vala collection catalog and exact presentation boundary. */
private void test_presentation () {
    assert (Search.filter_count (Search.Scope.SYMBOLS) == 27);
    assert (Search.filter_name (Search.Scope.SYMBOLS, 26) == "Block Elements");
    assert (Search.filter_name (Search.Scope.SYMBOLS, 27) == null);
    assert (Search.filter_count (Search.Scope.PLANE) == 17);
    assert (Search.filter_name (Search.Scope.PLANE, 16) == "16: Supplementary Private Use Area-B");
    assert (Search.category_count () == 8);
    assert (Search.category_name (8) == null);
    var nul = Search.points (0);
    assert (nul != null && nul.length == 1 && nul[0] == 0);
    assert (Search.code (0) == "U+0000" && Search.display (0) == "·");
    assert (Search.points (0xD800) == null && Search.points (uint32.MAX) == null);
    assert (Search.name (0x0378) == "Unassigned Character");
    assert (Search.name (0x10FFFF) == "Noncharacter");
    assert (Search.name (0xE000) == "Private Use Character");
    assert (Search.display (0x3164) == "·" && Search.display (0x0301) == "◌́");
    var job = QueryParser.create (Search.Scope.EMOJI, -1, -1, "U+2764 U+FE0E");
    assert (job != null && job.run () == Search.Status.OK && job.count () == 1);
    assert (Search.code (job.id (0)) == "U+2764 U+FE0E");
    assert (Search.display (job.id (0)) == "❤︎");
}

/** Exercise shared numeric syntax and invalid input directly through the Vala parser. */
private void test_query_parser () {
    var points = QueryParser.decode_code (" \tU+0000 0x20ac dec:128512\r\n");
    assert (points != null && points.length == 3);
    assert (points[0] == 0 && points[1] == 0x20AC && points[2] == 0x1F600);
    foreach (var invalid in new string[] { "", "U+D800", "U+110000", "dec:-1", "U+41 42" }) {
        assert (QueryParser.decode_code (invalid) == null);
    }
    var job = QueryParser.create (Search.Scope.ALL, -1, -1, "face");
    assert (job != null && job.run () == Search.Status.OK);
    bool found = false;
    for (size_t index = 0; index < job.count (); index++) {
        if (job.id (index) == 0xFACE) {
            found = true;
            break;
        }
    }
    assert (found);
    job = QueryParser.create (Search.Scope.ALL, -1, -1, string.nfill (513, 'a'));
    assert (job != null && job.run () == Search.Status.INVALID);
}

/** Exercise the production Vala traversal plan and its empty/rejected request behavior. */
private void test_search_plan () {
    var job = QueryParser.create (Search.Scope.ALL, -1, -1, "");
    assert (job != null && job.run () == Search.Status.OK && job.count () == 300543);
    job = QueryParser.create (Search.Scope.PLANE, 15, -1, "");
    assert (job != null && job.run () == Search.Status.OK && job.count () == 65534);
    assert (job.id (0) == 0xF0000 && job.id (65533) == 0xFFFFD);
    job = QueryParser.create (Search.Scope.BLOCK, 0, -1, "U+20AC");
    assert (job != null && job.run () == Search.Status.OK && job.count () == 0);
    job = QueryParser.create (Search.Scope.NAMED, -1, -1, "");
    assert (job != null && job.run () == Search.Status.OK && job.count () == Unicode.named_count ());
    job = QueryParser.create (Search.Scope.EMOJI, -1, -1, "");
    assert (job != null && job.run () == Search.Status.OK && job.count () == Unicode.emoji_count ());
    job = QueryParser.create_points (new uint32[0], -1, "");
    assert (job != null && job.run () == Search.Status.OK && job.count () == 0);
    job = QueryParser.create (Search.Scope.PLANE, 17, -1, "");
    assert (job != null && job.run () == Search.Status.INVALID && job.count () == 0);
}

/** A newer request must replace pending work and suppress completion of the older request. */
private void test_latest_request () {
    var controller = new SearchController ();
    uint publications = 0;
    SearchRequest? result = null;
    controller.completed.connect ((request) => {
        publications++;
        result = request;
    });
    controller.submit (Search.Scope.ALL, -1, -1, "letter");
    controller.submit (Search.Scope.ALL, -1, -1, "U+0041");
    controller.submit (Search.Scope.ALL, -1, -1, "U+221E");
    var context = MainContext.default ();
    var deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    while (result == null && get_monotonic_time () < deadline) {
        context.iteration (false);
        Thread.usleep (POLL_INTERVAL_US);
    }
    assert (result != null);
    assert (publications == 1);
    assert (result.get_status () == Search.Status.OK);
    assert (result.count () == 1 && result.id (0) == 0x221E);
    controller.shutdown ();
}

/** Registers headless Vala binding and request-publication tests. */
int main (string[] args) {
    Test.init (ref args);
    Test.add_func ("/characters/search/bindings", test_bindings);
    Test.add_func ("/characters/search/presentation", test_presentation);
    Test.add_func ("/characters/search/query-parser", test_query_parser);
    Test.add_func ("/characters/search/scan-plan", test_search_plan);
    Test.add_func ("/characters/search/latest-request", test_latest_request);
    return Test.run ();
}
