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
            assert (strcmp0 (alternatives[index - 1], alternatives[index]) < 0);
        }
        found |= alternatives[index] == expected;
    }
    assert (found);
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
    expect_related (0x61, "A");
    expect_related (0x28, ")");
    expect_related (0x65, "é");
    expect_related (0xE9, "e");
    expect_related (0xAC00, "ᄀ");
    expect_related (0x41, "Α");
    expect_related (0xA9, "©️");
    expect_related (0xA7D3, "\uA7D2");
    expect_related (0x16EA0, "\U00016EBB");
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
