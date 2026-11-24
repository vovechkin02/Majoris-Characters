using Characters;

/** Verifies compact snapshot ownership, exact formats, NUL and related-vector bindings. */
int main () {
    var snapshot = Inspector.from_text ("é");
    assert (snapshot != null);
    assert (snapshot.name () == "LATIN SMALL LETTER E WITH ACUTE");
    assert (snapshot.count () == 1 && snapshot.point (0) == 0xE9);
    assert (snapshot.format (Inspector.Format.UTF8) == "C3 A9");
    assert (snapshot.normalize ("NFD") == "U+0065 U+0301");
    var related = snapshot.related ();
    assert (related != null);
    bool found = false;
    foreach (var text in related) {
        if (text == "e") {
            found = true;
        }
    }
    assert (found);
    snapshot = Inspector.from_id (0);
    assert (snapshot != null && snapshot.text () == null);
    assert (snapshot.format (Inspector.Format.CODE) == "U+0000");
    snapshot = Inspector.from_code ("U+1F469 U+200D U+1F4BB");
    assert (snapshot != null && snapshot.text () == "👩‍💻");
    assert (snapshot.count () == 3);
    assert (Inspector.from_code ("U+D800") == null);
    return 0;
}
