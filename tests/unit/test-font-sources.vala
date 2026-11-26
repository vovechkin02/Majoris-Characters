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
    assert (Legacy.decode (table, "80") == 0x20AC);
    assert (Legacy.bytes (table, 0x20AC) == "80");
    var points = table.points ();
    assert (points != null && points.length >= 128);
    var job = QueryParser.create_points (points, -1, "U+20AC");
    assert (job != null);
    assert (job.run () == Search.Status.OK);
    assert (job.count () == 1 && job.id (0) == 0x20AC);
}

/** Exercises sequence array ownership and exact contextual re-encoding through Vala. */
private void test_sequence_bindings () {
    var encoding = encoding_index ("ISCII Devanagari");
    assert (Legacy.is_contextual (encoding));
    var decoded = Legacy.decode_sequence (encoding, "E8 E8");
    assert (decoded != null && decoded.length == 2);
    assert (decoded[0] == 0x094D && decoded[1] == 0x200C);
    var bytes = Legacy.encode_sequence (encoding, decoded);
    assert (bytes != null);
    var round_trip = Legacy.decode_sequence (encoding, bytes);
    assert (round_trip != null && round_trip.length == 2);
    assert (round_trip[0] == decoded[0] && round_trip[1] == decoded[1]);
    encoding = encoding_index ("GB18030-2022");
    uint32[] points = { 0, 0x1F600 };
    bytes = Legacy.encode_sequence (encoding, points);
    assert (bytes == "00 94 39 FC 36");
    decoded = Legacy.decode_sequence (encoding, bytes);
    assert (decoded != null && decoded.length == 2 && decoded[0] == 0);
    assert (decoded[1] == 0x1F600);
    assert (Legacy.decode_sequence (encoding, "94 39 FC") == null);
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
    assert (Legacy.decode (table, "80") == 0x20AC);
    assert (received.take_table () == null && received.take_points () == null);
    loader.shutdown ();
}

/** Exercises ownership, exact raster bytes, axis output parameters and symbol-code separation. */
private void test_exact_face_bindings () {
    var directory = Environment.get_variable ("CHARACTERS_FONT_FIXTURES");
    assert (directory != null);
    var face = Fonts.open_file (Path.build_filename (directory, "variable.ttf"), 0x10000);
    assert (face != null && face.axis_count () == 1 && face.axis_tag (0) == "wght");
    double minimum, maximum, default_value, current;
    assert (face.axis_range (0, out minimum, out maximum, out default_value, out current));
    assert (minimum == 100 && maximum == 900 && current == 900);
    var heavy = face.render (0x41, false, 48, 0, 0, 0);
    assert (heavy != null && heavy.get_size () == 96 * 96 * 4);
    assert (face.set_axis (0, 100));
    var light = face.render (0x41, false, 48, 0, 0, 0);
    assert (light != null && !heavy.equal (light));
    assert (face.reset ());
    assert (face.axis_range (0, out minimum, out maximum, out default_value, out current));
    assert (current == 900);
    var symbol = Fonts.open_file (Path.build_filename (directory, "symbol.ttf"), 0);
    assert (symbol != null && symbol.symbol_count () == 2);
    assert (symbol.symbol_code (0) == 0xF041);
    assert (symbol.glyph (0xF041, true) == 1 && symbol.glyph (0xF041, false) == 0);
}

/** Registers focused bindings and source supersession tests without initializing GTK. */
int main (string[] args) {
    Test.init (ref args);
    Test.add_func ("/characters/fonts/mapping-bindings", test_mapping_bindings);
    Test.add_func ("/characters/fonts/latest-source", test_latest_source);
    Test.add_func ("/characters/fonts/sequence-bindings", test_sequence_bindings);
    Test.add_func ("/characters/fonts/exact-face-bindings", test_exact_face_bindings);
    return Test.run ();
}
