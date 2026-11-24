using Characters;

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
    uint32[] letter = { 0x61 };
    var alternatives = Unicode.related (letter);
    assert (alternatives != null);
    bool found = false;
    foreach (var alternative in alternatives) {
        if (alternative == "A") {
            found = true;
        }
    }
    assert (found);
    return 0;
}
