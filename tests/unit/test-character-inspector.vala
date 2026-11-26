using Characters;

/** Verify every numeric representation, supplementary scalars and invalid format rejection. */
private void test_encodings () {
    var snapshot = Inspector.from_text ("Aé😀");
    assert (snapshot != null && snapshot.count () == 3);
    assert (snapshot.point (0) == 0x41 && snapshot.point (2) == 0x1F600);
    assert (snapshot.point (3) == uint32.MAX);
    assert (snapshot.format (Inspector.Format.CODE) == "U+0041 U+00E9 U+1F600");
    assert (snapshot.format (Inspector.Format.DECIMAL) == "65 233 128512");
    assert (snapshot.format (Inspector.Format.UTF8) == "41 C3 A9 F0 9F 98 80");
    assert (snapshot.format (Inspector.Format.UTF16) == "0041 00E9 D83D DE00");
    assert (snapshot.format (Inspector.Format.UTF32) == "00000041 000000E9 0001F600");
    assert (snapshot.format (Inspector.Format.HTML_DECIMAL) == "&#65;&#233;&#128512;");
    assert (snapshot.format (Inspector.Format.HTML_HEX) == "&#x41;&#xE9;&#x1F600;");
    assert (snapshot.format (Inspector.Format.ESCAPE) == "\\u0041\\u00E9\\U0001F600");
    assert (snapshot.format (Inspector.Format.CSS) == "\\41 \\E9 \\1F600 ");
    assert (snapshot.text () == "Aé😀");
    assert (snapshot.format ((Inspector.Format) 99) == null);
}

/** Verify a retained snapshot and round-trip every catalog identity through scalar notation. */
private void test_catalog_identity () {
    var job = Search.create (Search.Scope.ALL, -1, -1, "U+1F469 U+200D U+1F4BB");
    assert (job != null && job.run () == Search.Status.OK && job.count () == 1);
    var snapshot = Inspector.from_id (job.id (0));
    job = null;
    assert (snapshot != null && snapshot.count () == 3);
    assert (snapshot.name () == "WOMAN TECHNOLOGIST");
    assert (snapshot.display () == "👩‍💻" && snapshot.text () == "👩‍💻");
    assert (snapshot.format (Inspector.Format.CODE) == "U+1F469 U+200D U+1F4BB");
    var related = snapshot.related ();
    assert (related != null);
    assert ("👩" in related && "💻" in related);
    var total = Unicode.emoji_count () + Unicode.named_count ();
    for (size_t index = 0; index < total; index++) {
        var text = index < Unicode.emoji_count () ? Unicode.emoji_text (index) :
            Unicode.named_text (index - Unicode.emoji_count ());
        snapshot = Inspector.from_text (text);
        assert (snapshot != null && snapshot.text () == text);
        var roundtrip = Inspector.from_code (snapshot.format (Inspector.Format.CODE));
        assert (roundtrip != null && roundtrip.text () == text);
    }
    snapshot = Inspector.from_text ("©︎");
    assert (snapshot != null && snapshot.display () == "©︎");
    snapshot = Inspector.from_text ("⌚︎");
    assert (snapshot != null);
    var name = snapshot.name ();
    assert (name != null && name.contains ("WATCH") && name.contains ("Text presentation"));
}

/** Preserve NUL and exact copy data while displaying controls and marks safely. */
private void test_nonprinting () {
    var snapshot = Inspector.from_id (0);
    assert (snapshot != null && snapshot.count () == 1);
    assert (snapshot.display () == "·" && snapshot.text () == null);
    assert (snapshot.format (Inspector.Format.UTF8) == "00");
    assert (snapshot.normalize ("NFC") == "U+0000");
    snapshot = Inspector.from_code ("U+0065 U+0301 U+0000 U+00E9");
    assert (snapshot != null && snapshot.text () == null);
    assert (snapshot.format (Inspector.Format.UTF8) == "65 CC 81 00 C3 A9");
    assert (snapshot.normalize ("NFC") == "U+00E9 U+0000 U+00E9");
    assert (snapshot.normalize ("NFD") == "U+0065 U+0301 U+0000 U+0065 U+0301");
    assert (snapshot.related () == null);
    snapshot = Inspector.from_text ("\n\t ");
    assert (snapshot != null && snapshot.display () == "···" && snapshot.text () == "\n\t ");
    snapshot = Inspector.from_text ("́");
    assert (snapshot != null && snapshot.display () == "◌́" && snapshot.text () == "́");
}

/** Verify normalization forms, expanded output and Unicode 17 deltas. */
private void test_normalization () {
    var snapshot = Inspector.from_text ("é①");
    assert (snapshot != null);
    assert (snapshot.normalize ("NFC") == "U+00E9 U+2460");
    assert (snapshot.normalize ("NFD") == "U+0065 U+0301 U+2460");
    assert (snapshot.normalize ("NFKC") == "U+00E9 U+0031");
    assert (snapshot.normalize ("NFKD") == "U+0065 U+0301 U+0031");
    assert (snapshot.normalize ("invalid") == null && snapshot.normalize (null) == null);
    var code = new StringBuilder ();
    for (var index = 0; index < 32; index++) {
        code.append ("U+FB03 ");
    }
    snapshot = Inspector.from_code (code.str);
    assert (snapshot != null && snapshot.count () == 32);
    var normalized = snapshot.normalize ("NFKD");
    assert (normalized != null && normalized.length == 96 * 7 - 1);
    snapshot = Inspector.from_code ("U+0061 U+0301 U+1ADD U+0000 U+A7F1");
    assert (snapshot != null);
    assert (snapshot.normalize ("NFKD") == "U+0061 U+1ADD U+0301 U+0000 U+0053");
    snapshot = Inspector.from_code ("U+0378");
    assert (snapshot != null && snapshot.name () == null && snapshot.normalize ("NFC") == null);
    assert (snapshot.format (Inspector.Format.CODE) == "U+0378");
}

/** Reject malformed UTF-8 and numeric notation without discarding valid reserved scalars. */
private void test_invalid_input () {
    string[] invalid = { "", "U+", "U+D800", "U+110000", "U+0041 garbage", "U+-1",
        "dec:1114112", "U+FFFFFFFF", "U+0041, U+0042" };
    foreach (var code in invalid) {
        assert (Inspector.from_code (code) == null);
    }
    assert (Inspector.from_code (null) == null && Inspector.from_text (null) == null);
    assert (Inspector.from_text ("") == null);
    uint8[] surrogate = { 0xED, 0xA0, 0x80, 0 };
    uint8[] overlong = { 0xC0, 0x80, 0 };
    assert (Inspector.from_text ((string) surrogate) == null);
    assert (Inspector.from_text ((string) overlong) == null);
    assert (Inspector.from_text ("012345678901234567890123456789012") == null);
    assert (Inspector.from_id (uint32.MAX) == null && Inspector.from_id (0xD800) == null);
    var reserved = Inspector.from_id (0x0378);
    assert (reserved != null && reserved.count () == 1 && reserved.display () == "·");
    var snapshot = Inspector.from_code ("  u+0041\t0x42 dec:67\n");
    assert (snapshot != null && snapshot.text () == "ABC");
}

/** Exercise immediate mappings for every scalar with a Unicode decomposition type. */
private void test_direct_decomposition () {
    assert (Inspector.decomposition (0x00E9) == "U+0065 U+0301");
    assert (Inspector.decomposition (0x01FA) == "U+00C5 U+0301");
    assert (Inspector.decomposition (0xAC01) == "U+AC00 U+11A8");
    assert (Inspector.decomposition (0x00BD) == "U+0031 U+2044 U+0032");
    assert (Inspector.decomposition (0xA7F1) == "U+0053");
    assert (Inspector.decomposition (0x41) == null && Inspector.decomposition (0x0378) == null);
    assert (Inspector.decomposition (0xD800) == null && Inspector.decomposition (0x110000) == null);
    var mappings = 0;
    for (uint32 point = 0; point <= 0x10FFFF; point++) {
        if (Scalar.decomposition_type (point) == 0) {
            continue;
        }
        var code = Inspector.decomposition (point);
        assert (code != null);
        var mapping = Inspector.from_code (code);
        assert (mapping != null && mapping.count () > 0);
        mappings++;
    }
    assert (mappings > 10000);
    var snapshot = Inspector.from_id (0x10FFFF);
    assert (snapshot != null && snapshot.format (Inspector.Format.CODE) == "U+10FFFF");
    assert (snapshot.display () == "·");
}

/** Run all six migrated inspector contract groups. */
int main () {
    test_encodings ();
    test_catalog_identity ();
    test_nonprinting ();
    test_normalization ();
    test_invalid_input ();
    test_direct_decomposition ();
    return 0;
}
