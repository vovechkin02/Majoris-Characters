using Characters;

/** Exercises nullable native collections and the owned lifetime of exact alternatives. */
int main (string[] args) {
    assert (!Emoji.is_candidate ("A"));
    assert (Emoji.create_variants ("A") == null);
    var variants = Emoji.create_variants ("👍");
    assert (variants != null && variants.count () == 6);
    bool found = false;
    for (size_t index = 0; index < variants.count (); index++) {
        unowned string? text = variants.text (index);
        assert (text != null && variants.label (index) != null);
        if (text == "👍🏿") {
            found = true;
        }
    }
    assert (found);
    assert (variants.text (variants.count ()) == null);
    return 0;
}
