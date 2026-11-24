using Characters;

/** Maximum main-context polling duration for a worker completion, in microseconds. */
private const int64 WAIT_TIMEOUT_US = 10000000;
/** Poll interval while allowing the main context to dispatch completion, in microseconds. */
private const ulong POLL_INTERVAL_US = 1000;

/** Native result ownership and catalog identifiers survive the Vala boundary. */
private void test_bindings () {
    var job = Search.create (Search.Scope.ALL, -1, -1, "U+20AC");
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
    Test.add_func ("/characters/search/latest-request", test_latest_request);
    return Test.run ();
}
