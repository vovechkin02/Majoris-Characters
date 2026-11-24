using Characters;

/** Main-context polling limit, in microseconds, for bounded worker tests. */
private const int64 WAIT_TIMEOUT_US = 10000000;
/** Poll interval, in microseconds, without blocking completion dispatch. */
private const ulong POLL_INTERVAL_US = 1000;

/** Finds an encoding by stable label without coupling tests to dropdown ordering. */
private size_t encoding_index (string name) {
    for (size_t index = 0; index < Legacy.count (); index++) {
        if (Legacy.name (index) == name) {
            return index;
        }
    }
    return (size_t) -1;
}

/** Checks returned-array ownership and the exact repertoire Vala binding. */
private void test_mapping_bindings () {
    var table = Legacy.load (encoding_index ("Windows-1252"));
    assert (table != null);
    assert (table.decode ("80") == 0x20AC);
    assert (table.bytes (0x20AC) == "80");
    var points = table.points ();
    assert (points != null && points.length >= 128);
    var job = Search.create_points (points, -1, "U+20AC");
    assert (job != null);
    assert (job.run () == Search.Status.OK);
    assert (job.count () == 1 && job.id (0) == 0x20AC);
}

/** Supersedes source workers and verifies only the last selected encoding is published. */
private void test_latest_source () {
    var loader = new FontSourceLoader ();
    uint publications = 0;
    FontSourceRequest? received = null;
    loader.completed.connect ((request, success) => {
        assert (success);
        publications++;
        received = request;
    });
    loader.submit (new FontSourceRequest.for_encoding (encoding_index ("GBK (Windows-936)")));
    loader.cancel ();
    loader.submit (new FontSourceRequest.for_encoding (encoding_index ("Windows-1252")));
    var context = MainContext.default ();
    var deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    while (received == null && get_monotonic_time () < deadline) {
        context.iteration (false);
        Thread.usleep (POLL_INTERVAL_US);
    }
    assert (received != null && publications == 1);
    var table = received.take_table ();
    var points = received.take_points ();
    assert (table != null && points != null && points.length >= 128);
    assert (table.decode ("80") == 0x20AC);
    assert (received.take_table () == null && received.take_points () == null);
    loader.shutdown ();
}

/** Registers focused bindings and source supersession tests without initializing GTK. */
int main (string[] args) {
    Test.init (ref args);
    Test.add_func ("/characters/fonts/mapping-bindings", test_mapping_bindings);
    Test.add_func ("/characters/fonts/latest-source", test_latest_source);
    return Test.run ();
}
