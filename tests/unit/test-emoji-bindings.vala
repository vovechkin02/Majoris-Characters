using Characters;

/** Test membership without converting or normalizing exact identities. */
private bool has_variant (Emoji.Variants variants, string expected) {
    for (size_t index = 0; index < variants.count (); index++) {
        if (variants.text (index) == expected) {
            return true;
        }
    }
    return false;
}

/** Require exact family membership, distinct results, valid labels and stable first entry. */
private void expect_family (string input, string expected, size_t minimum) {
    var variants = Emoji.create_variants (input);
    assert (variants != null && variants.count () >= minimum);
    assert (variants.text (0) == input && has_variant (variants, expected));
    for (size_t index = 0; index < variants.count (); index++) {
        var text = variants.text (index);
        assert (text != null && text != "" && Emoji.is_candidate (text));
        assert (variants.label (index) != null);
        for (size_t previous = 0; previous < index; previous++) {
            assert (text != variants.text (previous));
        }
    }
    assert (variants.text (variants.count ()) == null);
    assert (variants.label (variants.count ()) == null);
}

/** Check all tone, relationship, profession, hair, family and direction groups. */
private void test_families () {
    expect_family ("👍", "👍🏿", 6);
    expect_family ("👍🏽", "👍🏻", 6);
    expect_family ("🤝", "🫱🏻‍🫲🏿", 26);
    expect_family ("🫱🏻‍🫲🏿", "🤝🏾", 26);
    expect_family ("👩‍💻", "🧑🏽‍💻", 18);
    expect_family ("👨", "👩‍🦰", 12);
    expect_family ("🚶", "🚶🏽‍♀️‍➡️", 18);
    expect_family ("👪", "🧑‍🧑‍🧒", 20);
    expect_family ("💑", "👩🏻‍❤️‍👩🏿", 26);
    expect_family ("💏", "👨🏿‍❤️‍💋‍👨🏻", 26);
    expect_family ("👫", "🧑🏻‍🤝‍🧑🏿", 26);
    expect_family ("🇦🇺", "🇯🇵", 200);
    expect_family ("1️⃣", "#️⃣", 12);
    var variants = Emoji.create_variants ("👍");
    assert (variants != null && variants.count () == 8);
}

/** Check exact presentation pairs and minimally qualified aliases. */
private void test_presentations () {
    expect_family ("©️", "©︎", 3);
    expect_family ("©︎", "©️", 3);
    expect_family ("©", "©️", 3);
    expect_family ("👁‍🗨", "👁️‍🗨️", 4);
    expect_family ("🚶‍♀‍➡", "🚶‍♀️‍➡️", 18);
    expect_family ("1⃣", "1️⃣", 12);
    expect_family ("⌚︎", "⌚️", 3);
    expect_family ("#︎", "#️", 3);
    var bases = 0;
    for (size_t index = 0; index < Emoji.alias_count (); index++) {
        var text = Emoji.alias_text (index);
        if (text == null || !text.has_suffix ("\uFE0E")) {
            continue;
        }
        bases++;
        var base_text = text.substring (0, text.length - 3);
        var variants = Emoji.create_variants (text);
        assert (variants != null && variants.text (0) == text);
        assert (has_variant (variants, base_text));
        assert (has_variant (variants, base_text + "\uFE0F"));
    }
    assert (bases == 371);
}

/** Reject unknown and malformed sequences rather than inventing alternatives. */
private void test_invalid () {
    assert (!Emoji.is_candidate ("A"));
    assert (Emoji.create_variants ("A") == null);
    assert (Emoji.create_variants ("A️") == null);
    assert (Emoji.create_variants ("👍‍😀") == null);
    uint8[] malformed = { 0xFF, 0 };
    assert (Emoji.create_variants ((string) malformed) == null);
    assert (Emoji.create_variants (null) == null);
}

/** Verify locale normalization, parent selection and English fallback order. */
private void test_annotation_policy () {
    var english = Emoji.locale_index ("en");
    var french = Emoji.locale_index ("fr");
    var chinese = Emoji.locale_index ("zh");
    var locales = Emoji.annotation_locales ("FR-ca.UTF-8");
    assert (locales.length >= 2 && locales[locales.length - 2] == french &&
        locales[locales.length - 1] == english);
    locales = Emoji.annotation_locales ("zh_Hant_TW");
    assert (locales.length >= 2 && locales[locales.length - 2] == chinese &&
        locales[locales.length - 1] == english);
    locales = Emoji.annotation_locales ("zh-TW");
    assert (locales.length >= 2 && locales[locales.length - 1] == english);
    locales = Emoji.annotation_locales ("unknown");
    assert (locales.length == 1 && locales[0] == english);
    locales = Emoji.annotation_locales ("en_US@custom");
    assert (locales[locales.length - 1] == english);
    assert (Emoji.fold_term ("ÉMOTICÔNE") == "émoticône");
    assert (Emoji.fold_term ("ＦＵＬＬ") == "full");
    assert (Emoji.fold_term ("") == null);
    var text_alias = Emoji.alias_index ("⌚︎");
    assert (Emoji.alias_label (text_alias) == "Text presentation");
    assert (Emoji.alias_name (text_alias) == "WATCH (Text presentation)");
    assert (Unicode.sequence_name ({ 0x231Au, 0xFE0Eu }) == "WATCH (Text presentation)");
    assert (Emoji.alias_name (size_t.MAX) == null);
}

/** Exercise the migrated variation domain directly in Vala. */
int main () {
    test_families ();
    test_presentations ();
    test_invalid ();
    test_annotation_policy ();
    return 0;
}
