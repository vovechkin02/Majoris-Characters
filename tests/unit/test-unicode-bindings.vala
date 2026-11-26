using Characters;

/** Assert exact membership, uniqueness, binary ordering and original exclusion. */
private void expect_related (uint32 point, string expected) {
    uint32[] points = { point };
    var original = Unicode.encode (points);
    var alternatives = Relationships.collect (points);
    assert (alternatives != null);
    bool found = false;
    for (var index = 0; index < alternatives.length; index++) {
        assert (alternatives[index] != original);
        if (index > 0) {
            assert (GLib.strcmp (alternatives[index - 1], alternatives[index]) < 0);
        }
        found |= alternatives[index] == expected;
    }
    assert (found);
}

/** Compare the complete ordered scalar relationship set with the pre-migration baseline. */
private void expect_exact_relationships (uint32 point, uint32[] expected) {
    uint32[] points = { point };
    var alternatives = Relationships.collect (points);
    assert (alternatives != null && alternatives.length == expected.length);
    for (var index = 0; index < expected.length; index++) {
        uint32[] single = { expected[index] };
        assert (alternatives[index] == Unicode.encode (single));
    }
}

/** Verify accent, titlecase, new case pairs, Hangul, and empty related sets. */
private void test_relationship_policy () {
    uint32[] latin = {
        0x0041, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x0100,
        0x0102, 0x0104, 0x01CD, 0x01DE, 0x01E0, 0x01FB, 0x0200, 0x0202,
        0x0226, 0x0301, 0x030A, 0x1E00, 0x1EA0, 0x1EA2, 0x1EA4, 0x1EA6,
        0x1EA8, 0x1EAA, 0x1EAC, 0x1EAE, 0x1EB0, 0x1EB2, 0x1EB4, 0x1EB6
    };
    expect_exact_relationships (0x01FA, latin);
    expect_exact_relationships (0x304B, new uint32[] { 0x304C });
    expect_exact_relationships (0x30AB, new uint32[] { 0x2F12, 0x30AC, 0x529B, 0xF98A });
    expect_exact_relationships (0xAC01, new uint32[] { 0x1100, 0x1161, 0x11A8 });
    expect_exact_relationships (0x01C5, new uint32[] { 0x01C4, 0x01C6 });
    expect_exact_relationships (0xA7D3, new uint32[] { 0xA7D2 });
    expect_exact_relationships (0x16EA0, new uint32[] { 0x16EBB });
    expect_exact_relationships (0x16EBB, new uint32[] { 0x16EA0 });
    expect_exact_relationships (0x0378, new uint32[0]);
    expect_exact_relationships (0xFFFF, new uint32[0]);
}

/** Verify application formatting, exact property aliases and independent Han provenance. */
private void test_property_metadata () {
    assert (Unicode.get_property_value (0x41, "General_Category") == "Lu");
    assert (Unicode.get_property_value (0x41, "gc") == "Lu");
    assert (Unicode.get_property_value (0x41, "sC") == "Latn");
    assert (Unicode.get_property_value (0x41, "Age") == "1.1");
    assert (Unicode.get_property_value (0x16EA0, "Age") == "17.0");
    assert (Unicode.get_property_value (0x16EA0, "Script") == "Berf");
    assert (Unicode.get_property_value (0x301, "Canonical_Combining_Class") == "230");
    assert (Unicode.get_property_value (0x41, "Canonical_Combining_Class") == "0");
    assert (Unicode.get_property_value (0x301, "ccc") == null);
    assert (Unicode.get_property_value (0x41, "age") == null);
    assert (Unicode.get_property_value (0x41, "East_Asian_Width") == "Na");
    assert (Unicode.get_property_value (0x41, "Decomposition_Type") == "None");
    assert (Unicode.get_property_value (0x5D0, "bc") == "R");
    assert (Unicode.get_property_value (0x28, "Bidi_Mirrored") == "Y");
    assert (Unicode.get_property_value (0x41, "Emoji") == "N");
    assert (Unicode.get_property_value (0x1F600, "Emoji_Presentation") == "Y");
    assert (Unicode.get_property_value (0x1F3FB, "Emoji_Modifier") == "Y");
    var extensions = Unicode.get_property_value (0x30FC, "Script_Extensions");
    assert (extensions != null && extensions.contains ("Hira") && extensions.contains ("Kana"));
    assert (Unicode.get_property_value (0, "Plane") == "0");
    assert (Unicode.get_property_value (0x0378, "Plane") == "0");
    assert (Unicode.get_property_value (0x10FFFF, "Plane") == "16");
    assert (Unicode.get_property_value (0x0378, "Script") == null);
    assert (Unicode.get_property_value (0xFFFF, "Age") == null);
    assert (Unicode.get_property_value (0xD800, "Plane") == null);
    assert (Unicode.get_property_value (0x110000, "Plane") == null);
    assert (Unicode.get_property_value (0x41, "invalid") == null);
    assert (Unicode.get_property_value (0x41, null) == null);
    var mandarin = Unicode.get_property_value (0x4E2D, "Mandarin_Fallback");
    assert (mandarin != null && mandarin.contains ("zhōng"));
    assert (Unicode.get_property_value (0x4E2D, "Hangul_Fallback") == "중");
    assert (Unicode.get_property_value (0x4E2D, "Radical_Collation") == "2");
    assert (Unicode.get_property_value (0x4E2D, "Stroke_Collation") == "4");
    assert (Unicode.unihan_version () == "17.0.0");
    assert (Unicode.get_property_value (0x4E2D, "kMandarin") == "zhōng");
    assert (Unicode.get_property_value (0x6C34, "kCantonese") == "seoi2");
    assert (Unicode.get_property_value (0x6C34, "kJapaneseKun") == "MIZU");
    assert (Unicode.get_property_value (0x6C34, "kDefinition") == "water, liquid, lotion, juice");
    assert (Unicode.get_property_value (0x6C34, "kRSUnicode") == "85.0");
    assert (Unicode.get_property_value (0x6C34, "kTotalStrokes") == "4");
    assert (Unicode.get_property_value (0x41, "Mandarin_Fallback") == null);
    assert (Unicode.get_property_value (0x41, "Radical_Collation") == null);
}

/** Exercise native binding ownership, UTF-8 sequence lengths, and borrowed catalog data. */
int main () {
    assert (Unicode.is_scalar (0x10FFFF));
    assert (!Unicode.is_scalar (0xD800));
    assert (Unicode.name (0x41) == "LATIN CAPITAL LETTER A");
    assert (Unicode.get_property_value (0x41, "Script") == "Latn");
    assert (Unicode.find_block (0x41) == "Basic Latin");
    assert (Unicode.block_count () > 300);
    uint32[] points = { 0x1F469, 0x200D, 0x1F4BB };
    assert (Unicode.encode (points) == "👩‍💻");
    assert (Unicode.sequence_name (points) == "WOMAN TECHNOLOGIST");
    assert (Unicode.normalize ("é", "NFD") == "e\u0301");
    test_property_metadata ();
    test_relationship_policy ();
    expect_related (0x61, "A");
    expect_related (0x28, ")");
    expect_related (0x65, "é");
    expect_related (0xE9, "e");
    expect_related (0xAC00, "ᄀ");
    expect_related (0x41, "Α");
    expect_related (0xA9, "©️");
    expect_related (0xA7D3, "\uA7D2");
    expect_related (0x16EA0, "𖺻");
    expect_related (0x231A, "⌚︎");
    expect_related (0x231A, "⌚️");
    uint32[] invalid = { 0x110000 };
    assert (Relationships.collect (invalid) == null);
    uint32[] embedded = { 0x61, 0 };
    assert (Relationships.collect (embedded) == null);
    uint32[] nul = { 0 };
    var empty = Relationships.collect (nul);
    assert (empty != null && empty.length == 0);
    var parts = Relationships.collect (points);
    assert (parts != null && "👩" in parts && "💻" in parts);
    return 0;
}
