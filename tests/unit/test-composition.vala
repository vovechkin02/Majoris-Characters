using Characters;

/** Count graphemes independently of scalar and byte length. */
private void test_graphemes () {
    assert (Composition.graphemes ("") == 0);
    assert (Composition.graphemes ("A\r\n") == 2);
    assert (Composition.graphemes ("é") == 1);
    assert (Composition.graphemes ("👩🏽‍💻🇦🇺1️⃣©️") == 4);
    assert (Composition.graphemes ("שלום عربي") == 9);
}

/** Preserve exact selectors, supplementary scalars and separators in every format. */
private void test_formats () {
    var text = "A😀́\n";
    assert (Composition.format (text, Inspector.Format.CODE) == "U+0041 U+1F600 U+0301 U+000A");
    assert (Composition.format (text, Inspector.Format.DECIMAL) == "65 128512 769 10");
    assert (Composition.format (text, Inspector.Format.UTF8) == "41 F0 9F 98 80 CC 81 0A");
    assert (Composition.format (text, Inspector.Format.UTF16) == "0041 D83D DE00 0301 000A");
    assert (Composition.format (text, Inspector.Format.UTF32) ==
        "00000041 0001F600 00000301 0000000A");
    assert (Composition.format (text, Inspector.Format.HTML_DECIMAL) == "&#65;&#128512;&#769;&#10;");
    assert (Composition.format (text, Inspector.Format.HTML_HEX) == "&#x41;&#x1F600;&#x301;&#xA;");
    assert (Composition.format (text, Inspector.Format.ESCAPE) == "\\u0041\\U0001F600\\u0301\\u000A");
    assert (Composition.format (text, Inspector.Format.CSS) == "\\41 \\1F600 \\301 \\A ");
    assert (Composition.format ("©️", Inspector.Format.CODE) == "U+00A9 U+FE0F");
}

/** Normalize across the entire input only when explicitly requested. */
private void test_normalization () {
    assert (Composition.normalize ("é👩‍💻", "NFC") == "é👩‍💻");
    assert (Composition.normalize ("é", "NFD") == "é");
    assert (Composition.normalize ("①ﬁ", "NFKC") == "1fi");
    assert (Composition.normalize ("①é", "NFKD") == "1é");
    assert (Composition.normalize ("", "NFC") == "");
    assert (Composition.normalize ("\uA7F1\U000323B0", "NFKC") == "S\U000323B0");
    assert (Composition.normalize ("a\u0301\u1ADD", "NFD") == "a\u1ADD\u0301");
    assert (Composition.normalize ("\u0378", "NFC") == null);
    assert (Composition.normalize ("A", "invalid") == null);
}

/** Reject malformed and oversized input and output without truncating valid long text. */
private void test_limits () {
    uint8[] invalid_surrogate = { 0xED, 0xA0, 0x80, 0 };
    uint8[] overlong = { 0xC0, 0xAF, 0 };
    uint8[] out_of_range = { 0xF4, 0x90, 0x80, 0x80, 0 };
    uint8[] truncated = { 0xE2, 0 };
    string?[] invalid = { null, (string) invalid_surrogate, (string) overlong,
        (string) out_of_range, (string) truncated };
    foreach (var text in invalid) {
        assert (Composition.graphemes (text) == -1);
        assert (Composition.format (text, Inspector.Format.CODE) == null);
        assert (Composition.normalize (text, "NFC") == null);
    }
    assert (Composition.format ("", Inspector.Format.CODE) == null);
    assert (Composition.format ("A", (Inspector.Format) 999) == null);
    var text = string.nfill (Composition.MAX_POINTS, 'A');
    assert (Composition.graphemes (text) == Composition.MAX_POINTS);
    var output = Composition.format (text, Inspector.Format.CODE);
    assert (output != null && output.length == Composition.MAX_POINTS * 7 - 1);
    assert (Composition.normalize (text, "NFC") == text);
    text += "A";
    assert (Composition.graphemes (text) == -1);
    assert (Composition.format (text, Inspector.Format.CODE) == null);
    assert (Composition.normalize (text, "NFC") == null);
    var accents = new StringBuilder ();
    for (var index = 0; index < Composition.MAX_POINTS; index++) {
        accents.append ("é");
    }
    assert (Composition.normalize (accents.str, "NFD") == null);
    uint32[] surrogate = { 0xD800 };
    assert (Inspector.format_points (surrogate, Inspector.Format.UTF8) == null);
}

/** Run the four migrated headless composition contract groups. */
int main () {
    test_graphemes ();
    test_formats ();
    test_normalization ();
    test_limits ();
    return 0;
}
