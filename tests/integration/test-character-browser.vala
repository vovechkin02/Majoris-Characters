using Characters;

/** Standard logical window width for application integration tests. */
private const int WINDOW_WIDTH_PX = 800;
/** Standard logical window height for application integration tests. */
private const int WINDOW_HEIGHT_PX = 600;
/** Maximum wait for an asynchronously published result, in microseconds. */
private const int64 WAIT_TIMEOUT_US = 10000000;
/** Poll delay while dispatching GTK events, in microseconds. */
private const ulong POLL_INTERVAL_US = 1000;

/** Finds a real XML-defined widget by its GtkBuildable identity. */
private Gtk.Widget? find_widget (Gtk.Widget root, string id) {
    var buildable = root as Gtk.Buildable;
    if (buildable != null && buildable.get_buildable_id () == id) {
        return root;
    }
    unowned Gtk.Widget? child = root.get_first_child ();
    while (child != null) {
        var result = find_widget (child, id);
        if (result != null) {
            return result;
        }
        child = child.get_next_sibling ();
    }
    return null;
}

/** Waits for a populated page with the expected size and optional first identity. */
private void wait_for_page (Gtk.SingleSelection selection, uint expected, string? first = null) {
    var deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    var context = MainContext.default ();
    while (get_monotonic_time () < deadline) {
        context.iteration (false);
        var model = selection.model as Gtk.StringList;
        if (model != null && model.get_n_items () == expected &&
            (first == null || model.get_string (0) == first)) {
            return;
        }
        Thread.usleep (POLL_INTERVAL_US);
    }
    assert_not_reached ();
}

/** Reads actual clipboard text with a bounded wait on the GTK main context. */
private void expect_clipboard (Gtk.Widget widget, string expected) {
    var clipboard = widget.get_clipboard ();
    string? copied = null;
    bool completed = false;
    clipboard.read_text_async.begin (null, (object, result) => {
        try {
            copied = clipboard.read_text_async.end (result);
        } catch (Error error) {
            Test.message ("Clipboard read failed: %s", error.message);
        }
        completed = true;
    });
    var deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    var context = MainContext.default ();
    while (!completed && get_monotonic_time () < deadline) {
        context.iteration (false);
        Thread.usleep (POLL_INTERVAL_US);
    }
    assert (completed && copied == expected);
}

/** Exercises the native legacy byte browser and exact copied Unicode and encoded text. */
private void test_legacy_browser (MainWindow window) {
    var action = window.lookup_action ("browse-encodings");
    assert (action != null);
    action.activate (null);
    var dialog = window.get_visible_dialog () as FontBrowser;
    assert (dialog != null);
    var encodings = find_widget (dialog, "encoding_filter") as Gtk.DropDown;
    var entry = find_widget (dialog, "font_query") as Gtk.SearchEntry;
    var grid = find_widget (dialog, "font_grid") as Gtk.GridView;
    var copy = find_widget (dialog, "font_copy") as Gtk.Button;
    var bytes = find_widget (dialog, "bytes_copy") as Gtk.Button;
    var detail = find_widget (dialog, "font_detail") as Gtk.Label;
    assert (encodings != null && entry != null && grid != null);
    assert (copy != null && bytes != null && detail != null);
    var selection = grid.model as Gtk.SingleSelection;
    assert (selection != null);
    for (uint index = 0; index < Legacy.count (); index++) {
        if (Legacy.name (index) == "Windows-1252") {
            encodings.selected = index;
            break;
        }
    }
    entry.text = "byte:80";
    wait_for_page (selection, 1, "8364");
    selection.selected = 0;
    assert (detail.label.contains ("80") && copy.sensitive && bytes.sensitive);
    copy.clicked ();
    expect_clipboard (dialog, "€");
    bytes.clicked ();
    expect_clipboard (dialog, "80");
    var add = find_widget (dialog, "font_add") as Gtk.Button;
    var editor = find_widget (window, "composer_input") as Gtk.TextView;
    assert (add != null && editor != null && add.sensitive);
    add.clicked ();
    assert (editor.buffer.text == "👩‍💻€");
    entry.text = "byte:00";
    wait_for_page (selection, 1, "0");
    selection.selected = 0;
    assert (!copy.sensitive && bytes.sensitive && !add.sensitive);
    entry.text = "byte:GG";
    assert (selection.model == null && !bytes.sensitive);
    assert (dialog.close ());
}

/** Converts multi-scalar ISCII and GB18030 bytes with independent dialog lifetimes. */
private void test_legacy_sequence_dialog (MainWindow window) {
    var action = window.lookup_action ("browse-encodings");
    assert (action != null);
    action.activate (null);
    var browser = window.get_visible_dialog () as FontBrowser;
    assert (browser != null);
    var encodings = find_widget (browser, "encoding_filter") as Gtk.DropDown;
    var search = find_widget (browser, "font_query") as Gtk.SearchEntry;
    var open = find_widget (browser, "inspect_bytes") as Gtk.Button;
    assert (encodings != null && search != null && open != null);
    for (uint index = 0; index < Legacy.count (); index++) {
        if (Legacy.name (index) == "ISCII Devanagari") {
            encodings.selected = index;
            break;
        }
    }
    search.text = "byte:E8 E8";
    open.clicked ();
    var dialog = window.get_visible_dialog () as LegacySequenceDialog;
    assert (dialog != null);
    var hex = find_widget (dialog, "byte_input") as Gtk.Entry;
    var code = find_widget (dialog, "unicode_input") as Gtk.Entry;
    var copy = find_widget (dialog, "copy_sequence_bytes") as Gtk.Button;
    var decode = find_widget (dialog, "decode_bytes") as Gtk.Button;
    var encode = find_widget (dialog, "encode_codes") as Gtk.Button;
    var inspect = find_widget (dialog, "inspect_sequence") as Gtk.Button;
    assert (hex != null && code != null && copy != null && decode != null);
    assert (encode != null && inspect != null);
    assert (hex.text == "E8 E8" && code.text == "U+094D U+200C");
    assert (copy.sensitive && inspect.sensitive);
    copy.clicked ();
    expect_clipboard (dialog, "E8 E8");
    hex.text = "GG";
    assert (!copy.sensitive && !inspect.sensitive);
    decode.clicked ();
    assert (!copy.sensitive);
    code.text = "U+094D U+200C";
    encode.clicked ();
    assert (copy.sensitive && inspect.sensitive);
    inspect.clicked ();
    var inspector = window.get_visible_dialog () as CharacterInspector;
    assert (inspector != null && inspector.close ());
    assert (dialog.close ());
    for (uint index = 0; index < Legacy.count (); index++) {
        if (Legacy.name (index) == "GB18030-2022") {
            encodings.selected = index;
            break;
        }
    }
    search.text = "byte:94 39 FC 36";
    open.clicked ();
    dialog = window.get_visible_dialog () as LegacySequenceDialog;
    assert (dialog != null);
    code = find_widget (dialog, "unicode_input") as Gtk.Entry;
    assert (code != null && code.text == "U+1F600");
    assert (dialog.close ());
    assert (browser.close ());
}

/** Selects an installed fixture family and searches the exact face without fallback. */
private void test_font_browser (MainWindow window) {
    var action = window.lookup_action ("browse-fonts");
    assert (action != null);
    action.activate (null);
    var dialog = window.get_visible_dialog () as FontBrowser;
    assert (dialog != null);
    var families = find_widget (dialog, "family_filter") as Gtk.DropDown;
    var entry = find_widget (dialog, "font_query") as Gtk.SearchEntry;
    var grid = find_widget (dialog, "font_grid") as Gtk.GridView;
    var fallback = find_widget (dialog, "font_fallback") as Gtk.CheckButton;
    var preview = find_widget (dialog, "preview_size") as Gtk.SpinButton;
    var detail = find_widget (dialog, "font_detail") as Gtk.Label;
    assert (families != null && entry != null && grid != null && fallback != null);
    assert (preview != null && detail != null && !fallback.active);
    var context = MainContext.default ();
    var deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    while (families.model == null && get_monotonic_time () < deadline) {
        context.iteration (false);
        Thread.usleep (POLL_INTERVAL_US);
    }
    var family_names = families.model as Gtk.StringList;
    assert (family_names != null);
    bool found = false;
    for (uint index = 0; index < family_names.get_n_items (); index++) {
        if (family_names.get_string (index) == "DejaVu Sans") {
            families.selected = index;
            found = true;
            break;
        }
    }
    assert (found);
    var selection = grid.model as Gtk.SingleSelection;
    assert (selection != null);
    entry.text = "U+0041";
    wait_for_page (selection, 1, "65");
    selection.selected = 0;
    assert (detail.label.contains ("DejaVu Sans"));
    var inspect_face = find_widget (dialog, "inspect_face") as Gtk.Button;
    assert (inspect_face != null && inspect_face.sensitive);
    inspect_face.clicked ();
    var face_dialog = window.get_visible_dialog () as FontFaceDialog;
    assert (face_dialog != null);
    var picture = find_widget (face_dialog, "face_picture") as Gtk.Picture;
    var glyph = find_widget (face_dialog, "glyph_identity") as Gtk.Label;
    assert (picture != null && picture.paintable != null);
    assert (glyph != null && glyph.label.contains ("U+0041"));
    assert (face_dialog.close ());
    preview.value = 36;
    fallback.active = true;
    assert (selection.selected == 0 && detail.label.contains ("DejaVu Sans"));
    var add = find_widget (dialog, "font_add") as Gtk.Button;
    var editor = find_widget (window, "composer_input") as Gtk.TextView;
    assert (add != null && editor != null && add.sensitive);
    add.clicked ();
    assert (editor.buffer.text == "👩‍💻€A");
    entry.text = "U+10FFFF";
    assert (dialog.close ());
}

/** Exercises the mapped inspector, exact clipboard, invalid input and related navigation. */
private void test_inspector (MainWindow window) {
    var action = window.lookup_action ("inspect-character");
    assert (action != null);
    action.activate (null);
    var dialog = window.get_visible_dialog () as CharacterInspector;
    assert (dialog != null);
    var identity = find_widget (dialog, "identity_code") as Gtk.Label;
    var input = find_widget (dialog, "identify_input") as Gtk.TextView;
    var error_label = find_widget (dialog, "input_error") as Gtk.Label;
    var copy = find_widget (dialog, "copy_character") as Gtk.Button;
    var components = find_widget (dialog, "component_filter") as Gtk.DropDown;
    var expander = find_widget (dialog, "identify_expander") as Gtk.Expander;
    assert (identity != null && input != null && error_label != null);
    assert (copy != null && components != null && expander != null);
    assert (identity.label == "U+20AC" && copy.sensitive);
    assert (dialog.activate_action_variant ("inspect.copy-character", null));
    expect_clipboard (dialog, "€");

    expander.expanded = true;
    input.buffer.text = "👩‍💻";
    assert (dialog.activate_action_variant ("inspect.identify-text", null));
    assert (identity.label == "U+1F469 U+200D U+1F4BB");
    assert (components.visible && components.model.get_n_items () == 3);
    components.selected = 1;
    assert (dialog.activate_action_variant ("inspect.copy-character", null));
    expect_clipboard (dialog, "👩‍💻");
    var add = find_widget (dialog, "inspect_add") as Gtk.Button;
    var editor = find_widget (window, "composer_input") as Gtk.TextView;
    assert (add != null && editor != null && add.sensitive);
    add.clicked ();
    assert (editor.buffer.text == "👩‍💻");
    assert (dialog.activate_action_variant ("inspect.related", new Variant.string ("U+1F469")));
    assert (identity.label == "U+1F469");
    assert (dialog.activate_action_variant ("inspect.back", null));
    assert (identity.label == "U+1F469 U+200D U+1F4BB");

    input.buffer.text = "U+01FA";
    assert (dialog.activate_action_variant ("inspect.identify-code", null));
    assert (identity.label == "U+01FA");
    assert (dialog.activate_action_variant ("inspect.related", new Variant.string ("U+00C5 U+0301")));
    assert (identity.label == "U+00C5 U+0301");
    assert (dialog.activate_action_variant ("inspect.back", null));
    assert (identity.label == "U+01FA");
    input.buffer.text = "U+D800";
    assert (dialog.activate_action_variant ("inspect.identify-code", null));
    assert (error_label.visible && identity.label == "U+01FA");
    input.buffer.text = "U+0000";
    assert (dialog.activate_action_variant ("inspect.identify-code", null));
    assert (!error_label.visible && identity.label == "U+0000" && !copy.sensitive && !add.sensitive);
    assert (dialog.activate_action_variant ("inspect.copy", new Variant.string ("00")));
    expect_clipboard (dialog, "00");
    dialog.force_close ();
}

/** Selects an exact skin-tone variation through the real dialog and checks its clipboard. */
private void test_emoji_variations (MainWindow window) {
    var action = window.lookup_action ("emoji-variations");
    assert (action != null);
    action.activate (null);
    var dialog = window.get_visible_dialog () as EmojiVariations;
    assert (dialog != null);
    var filter = find_widget (dialog, "variant_filter") as Gtk.DropDown;
    var preview = find_widget (dialog, "preview") as Gtk.Label;
    var copy = find_widget (dialog, "copy_button") as Gtk.Button;
    assert (filter != null && preview != null && copy != null);
    var model = filter.model as Gtk.StringList;
    assert (model != null && model.get_n_items () == 6);
    assert (preview.label == "👍");
    bool found = false;
    for (uint index = 0; index < model.get_n_items (); index++) {
        if (model.get_string (index).has_prefix ("👍🏿 —")) {
            filter.selected = index;
            found = true;
            break;
        }
    }
    assert (found && preview.label == "👍🏿");
    copy.clicked ();
    expect_clipboard (dialog, "👍🏿");
    var add = find_widget (dialog, "emoji_add") as Gtk.Button;
    var editor = find_widget (window, "composer_input") as Gtk.TextView;
    assert (add != null && editor != null && add.sensitive);
    add.clicked ();
    assert (editor.buffer.text == "👩‍💻€A👍🏿");
    assert (dialog.close ());
}

/** Exercises the shared composer with exact text, native edit history and conversions. */
private void test_composer (MainWindow window) {
    var open = window.lookup_action ("open-composer");
    assert (open != null);
    open.activate (null);
    var entry = find_widget (window, "composer_input") as Gtk.TextView;
    var expander = find_widget (window, "composer_expander") as Gtk.Expander;
    var add = find_widget (window, "browser_add") as Gtk.Button;
    var copy = find_widget (window, "browser_copy") as Gtk.Button;
    var undo = find_widget (window, "composer_undo") as Gtk.Button;
    var redo = find_widget (window, "composer_redo") as Gtk.Button;
    var clear = find_widget (window, "composer_clear") as Gtk.Button;
    var copy_all = find_widget (window, "composer_copy_all") as Gtk.Button;
    var normalize = find_widget (window, "composer_normalize") as Gtk.Button;
    var format = find_widget (window, "composer_copy_format") as Gtk.Button;
    var inspect = find_widget (window, "composer_inspect") as Gtk.Button;
    var counts = find_widget (window, "composer_counts") as Gtk.Label;
    var cut = find_widget (window, "composer_cut") as Gtk.Button;
    var paste = find_widget (window, "composer_paste") as Gtk.Button;
    assert (entry != null && expander != null && add != null && copy != null);
    assert (undo != null && redo != null && clear != null && copy_all != null);
    assert (normalize != null && format != null && inspect != null && counts != null);
    assert (cut != null && paste != null);
    assert (add.sensitive && copy.sensitive);
    copy.clicked ();
    expect_clipboard (window, "€");
    add.clicked ();
    assert (expander.expanded && entry.buffer.text == "€" && undo.sensitive);
    undo.clicked ();
    assert (entry.buffer.text == "" && redo.sensitive);
    redo.clicked ();
    assert (entry.buffer.text == "€");
    entry.buffer.text = "e\u0301👩‍💻";
    assert (counts.label.contains ("5 code points") && counts.label.contains ("2 graphemes"));
    Gtk.TextIter start;
    Gtk.TextIter end;
    entry.buffer.get_iter_at_offset (out start, 0);
    entry.buffer.get_iter_at_offset (out end, 2);
    entry.buffer.select_range (start, end);
    normalize.clicked ();
    assert (entry.buffer.text == "é👩‍💻");
    undo.clicked ();
    assert (entry.buffer.text == "e\u0301👩‍💻");
    entry.buffer.get_bounds (out start, out end);
    entry.buffer.place_cursor (end);
    format.clicked ();
    expect_clipboard (window, "U+0065 U+0301 U+1F469 U+200D U+1F4BB");
    copy_all.clicked ();
    expect_clipboard (window, "e\u0301👩‍💻");
    inspect.clicked ();
    var dialog = window.get_visible_dialog () as CharacterInspector;
    assert (dialog != null);
    var identity = find_widget (dialog, "identity_code") as Gtk.Label;
    assert (identity != null && identity.label == "U+0065 U+0301 U+1F469 U+200D U+1F4BB");
    dialog.force_close ();
    entry.buffer.get_iter_at_offset (out start, 0);
    entry.buffer.get_iter_at_offset (out end, 2);
    entry.buffer.select_range (start, end);
    assert (cut.sensitive);
    cut.clicked ();
    assert (entry.buffer.text == "👩‍💻");
    expect_clipboard (window, "e\u0301");
    paste.clicked ();
    var deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    var context = MainContext.default ();
    while (entry.buffer.text != "e\u0301👩‍💻" && get_monotonic_time () < deadline) {
        context.iteration (false);
        Thread.usleep (POLL_INTERVAL_US);
    }
    assert (entry.buffer.text == "e\u0301👩‍💻");
    clear.clicked ();
    assert (entry.buffer.text == "");
    undo.clicked ();
    assert (entry.buffer.text == "e\u0301👩‍💻");
    clear.clicked ();
    expander.expanded = false;
}

/** Verifies browser favorites, usage recording, history views and live preference changes. */
private void test_library (MainWindow window) {
    var settings = new GLib.Settings (APP_ID);
    var favorite = find_widget (window, "browser_favorite") as Gtk.Button;
    var copy = find_widget (window, "browser_copy") as Gtk.Button;
    assert (favorite != null && favorite.sensitive && copy != null && copy.sensitive);
    favorite.clicked ();
    favorite.clicked ();
    copy.clicked ();
    expect_clipboard (window, "€");
    assert (settings.get_strv ("favorites").length == 1);
    assert (settings.get_strv ("favorites")[0] == "€");
    assert (settings.get_strv ("usage-history")[0] == "1:€");
    foreach (var name in new string[] { "favorites", "recent", "frequent" }) {
        var action = window.lookup_action (name);
        assert (action != null);
        action.activate (null);
        var dialog = window.get_visible_dialog () as LibraryDialog;
        assert (dialog != null);
        var list = find_widget (dialog, "library_list") as Gtk.ListView;
        var query = find_widget (dialog, "library_query") as Gtk.SearchEntry;
        assert (list != null && query != null);
        var selection = list.model as Gtk.SingleSelection;
        assert (selection != null);
        wait_for_page (selection, 1, "€");
        query.text = "U+20AC";
        wait_for_page (selection, 1, "€");
        query.text = "missing character";
        wait_for_page (selection, 0);
        query.text = "";
        wait_for_page (selection, 1, "€");
        selection.selected = 0;
        if (name == "frequent") {
            var remove = find_widget (dialog, "library_remove") as Gtk.Button;
            assert (remove != null && remove.sensitive);
            remove.clicked ();
            wait_for_page (selection, 0);
        }
        assert (dialog.close ());
    }
    assert (settings.get_strv ("usage-history").length == 0);
    assert (settings.get_strv ("favorites").length == 1);
    var action = window.lookup_action ("preferences");
    assert (action != null);
    action.activate (null);
    var preferences = window.get_visible_dialog () as PreferencesDialog;
    assert (preferences != null);
    var enabled = find_widget (preferences, "history_enabled") as Adw.SwitchRow;
    var symbols = find_widget (preferences, "show_symbols") as Adw.SwitchRow;
    var size = find_widget (preferences, "grid_size") as Adw.SpinRow;
    var scopes = find_widget (window, "scope_list") as Gtk.ListBox;
    assert (enabled != null && symbols != null && size != null && scopes != null);
    enabled.active = false;
    symbols.active = false;
    size.value = 40;
    assert (!settings.get_boolean ("history-enabled"));
    assert (!scopes.get_row_at_index ((int) Search.Scope.SYMBOLS).visible);
    assert (settings.get_int ("grid-preview-size") == 40);
    assert (preferences.close ());
    copy.clicked ();
    assert (settings.get_strv ("usage-history").length == 0);
    settings.reset ("history-enabled");
    settings.reset ("show-symbols");
    settings.reset ("grid-preview-size");
}

/** Waits for a compact page without assuming the native worker has completed. */
private void wait_for_picker_page (Gtk.FlowBox results, int count) {
    var deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    var context = MainContext.default ();
    while (get_monotonic_time () < deadline) {
        context.iteration (false);
        if ((count == 0 || results.get_child_at_index (count - 1) != null) &&
            results.get_child_at_index (count) == null) {
            return;
        }
        Thread.usleep (POLL_INTERVAL_US);
    }
    assert_not_reached ();
}

/** Verifies compact picker activation, supersession, exact copy and independent closure. */
private void test_compact_picker (Characters.Application application, MainWindow window) {
    var action = application.lookup_action ("picker");
    assert (action != null);
    action.activate (null);
    CompactPicker? picker = null;
    foreach (var candidate in application.get_windows ()) {
        if (candidate is CompactPicker) {
            picker = (CompactPicker) candidate;
        }
    }
    assert (picker != null);
    picker.set_default_size (400, 480);
    var scope = find_widget (picker, "picker_scope") as Gtk.DropDown;
    var query = find_widget (picker, "picker_query") as Gtk.SearchEntry;
    var results = find_widget (picker, "picker_results") as Gtk.FlowBox;
    assert (scope != null && query != null && results != null);
    var category = find_widget (picker, "picker_category") as Gtk.DropDown;
    var next = find_widget (picker, "picker_next") as Gtk.Button;
    var previous = find_widget (picker, "picker_previous") as Gtk.Button;
    var commit = find_widget (picker, "picker_commit") as Gtk.Button;
    var favorite = find_widget (picker, "picker_favorite") as Gtk.Button;
    var variations = find_widget (picker, "picker_variations") as Gtk.Button;
    var inspect = find_widget (picker, "picker_inspect") as Gtk.Button;
    var expand = find_widget (picker, "expand_button") as Gtk.Button;
    assert (category != null && next != null && previous != null && commit != null);
    assert (favorite != null && variations != null && inspect != null && expand != null);
    assert (scope.model.get_n_items () == 6 && category.model.get_n_items () == 10);
    category.selected = 1;
    wait_for_picker_page (results, 80);
    assert (next.sensitive && !previous.sensitive);
    next.clicked ();
    assert (previous.sensitive);
    previous.clicked ();
    assert (!previous.sensitive);
    scope.selected = 2;
    assert (category.model.get_n_items () == 28);
    scope.selected = 0;
    query.text = "letter";
    query.text = "U+1F469 U+200D U+1F4BB";
    var deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    var context = MainContext.default ();
    while (get_monotonic_time () < deadline && results.get_child_at_index (0) == null) {
        context.iteration (false);
        Thread.usleep (POLL_INTERVAL_US);
    }
    var first = results.get_child_at_index (0);
    assert (first != null && results.get_child_at_index (1) == null);
    var settings = new GLib.Settings (Characters.APP_ID);
    assert (settings.set_int ("grid-preview-size", 40));
    deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    while (get_monotonic_time () < deadline) {
        context.iteration (false);
        first = results.get_child_at_index (0);
        if (first != null && first.get_child ().width_request == 80) {
            break;
        }
        Thread.usleep (POLL_INTERVAL_US);
    }
    assert (first != null && first.get_child ().width_request == 80);
    results.select_child (first);
    assert (commit.sensitive && variations.sensitive && inspect.sensitive);
    settings.set_strv ("favorites", {});
    favorite.clicked ();
    assert (settings.get_strv ("favorites").length == 1);
    assert (settings.get_strv ("favorites")[0] == "👩‍💻");
    variations.clicked ();
    var variant_dialog = picker.get_visible_dialog () as EmojiVariations;
    assert (variant_dialog != null);
    var variant_copy = find_widget (variant_dialog, "copy_button") as Gtk.Button;
    assert (variant_copy != null);
    variant_copy.clicked ();
    expect_clipboard (picker, "👩‍💻");
    inspect.clicked ();
    var inspector = picker.get_visible_dialog () as CharacterInspector;
    assert (inspector != null && inspector.close ());
    commit.clicked ();
    expect_clipboard (picker, "👩‍💻");
    // Saved collection matches use the same literal-text filter as the full dialog.
    query.text = "👩‍💻";
    scope.selected = 3;
    wait_for_picker_page (results, 1);
    results.select_child (results.get_child_at_index (0));
    favorite.clicked ();
    assert (settings.get_strv ("favorites").length == 0);
    assert (results.get_child_at_index (0) == null);
    scope.selected = 5;
    wait_for_picker_page (results, 1);
    results.select_child (results.get_child_at_index (0));
    expand.clicked ();
    var library = window.get_visible_dialog () as LibraryDialog;
    assert (library != null && library.title == "Frequently Used");
    var list = find_widget (library, "library_list") as Gtk.ListView;
    assert (list != null);
    var saved_selection = list.model as Gtk.SingleSelection;
    assert (saved_selection != null && saved_selection.selected == 0);
    assert (library.close ());
    settings.reset ("grid-preview-size");
    assert (window.visible);

    // A second-page selection must survive expansion into the full catalog.
    action.activate (null);
    foreach (var candidate in application.get_windows ()) {
        if (candidate is CompactPicker && candidate.visible) {
            picker = (CompactPicker) candidate;
        }
    }
    results = find_widget (picker, "picker_results") as Gtk.FlowBox;
    category = find_widget (picker, "picker_category") as Gtk.DropDown;
    next = find_widget (picker, "picker_next") as Gtk.Button;
    expand = find_widget (picker, "expand_button") as Gtk.Button;
    assert (results != null && category != null && next != null && expand != null);
    category.selected = 1;
    wait_for_picker_page (results, 80);
    next.clicked ();
    results.select_child (results.get_child_at_index (0));
    expand.clicked ();
    var full_grid = find_widget (window, "character_grid") as Gtk.GridView;
    var full_scope = find_widget (window, "scope_list") as Gtk.ListBox;
    var full_filter = find_widget (window, "collection_filter") as Gtk.DropDown;
    assert (full_grid != null && full_scope != null && full_filter != null);
    var full_selection = full_grid.model as Gtk.SingleSelection;
    assert (full_selection != null);
    deadline = get_monotonic_time () + WAIT_TIMEOUT_US;
    while (get_monotonic_time () < deadline && full_selection.selected != 80) {
        context.iteration (false);
        Thread.usleep (POLL_INTERVAL_US);
    }
    assert (full_selection.selected == 80);
    assert (full_scope.get_selected_row ().get_index () == 1 && full_filter.selected == 1);
}

/** Exercises actual widget models, search supersession, filters, reset and selection. */
private void test_browser () {
    var application = new Characters.Application ();
    try {
        assert (application.register (null));
    } catch (Error error) {
        Test.message ("Application registration failed: %s", error.message);
        assert_not_reached ();
    }
    var window = new MainWindow (application);
    window.set_default_size (WINDOW_WIDTH_PX, WINDOW_HEIGHT_PX);
    window.present ();
    var grid = find_widget (window, "character_grid") as Gtk.GridView;
    var entry = find_widget (window, "search_entry") as Gtk.SearchEntry;
    var scope_list = find_widget (window, "scope_list") as Gtk.ListBox;
    var collection = find_widget (window, "collection_filter") as Gtk.DropDown;
    var category = find_widget (window, "category_filter") as Gtk.DropDown;
    var next = find_widget (window, "next_button") as Gtk.Button;
    var previous = find_widget (window, "previous_button") as Gtk.Button;
    var selected_name = find_widget (window, "selected_name") as Gtk.Label;
    var selected_code = find_widget (window, "selected_code") as Gtk.Label;
    assert (grid != null && entry != null && scope_list != null && collection != null);
    assert (category != null && next != null && previous != null);
    assert (selected_name != null && selected_code != null);
    var selection = grid.model as Gtk.SingleSelection;
    assert (selection != null);
    wait_for_page (selection, 512, "0");
    assert (next.sensitive && !previous.sensitive);
    next.clicked ();
    assert (previous.sensitive);
    previous.clicked ();
    wait_for_page (selection, 512, "0");

    entry.text = "letter";
    entry.text = "U+20AC";
    wait_for_page (selection, 1, "8364");
    selection.selected = 0;
    assert (selected_name.label == "EURO SIGN");
    assert (selected_code.label == "U+20AC");
    var inspect = find_widget (window, "inspect_button") as Gtk.Button;
    assert (inspect != null && inspect.sensitive);
    test_library (window);
    test_composer (window);
    test_inspector (window);
    assert (selected_code.label == "U+20AC");
    test_legacy_browser (window);
    test_legacy_sequence_dialog (window);
    test_font_browser (window);
    assert (selected_code.label == "U+20AC");

    entry.text = "U+1F44D";
    scope_list.select_row (scope_list.get_row_at_index ((int) Search.Scope.EMOJI));
    wait_for_page (selection, 1);
    selection.selected = 0;
    assert (selected_code.label == "U+1F44D");
    test_emoji_variations (window);
    assert (selected_code.label == "U+1F44D");

    entry.text = "";
    scope_list.select_row (scope_list.get_row_at_index ((int) Search.Scope.BLOCK));
    collection.selected = 1;
    wait_for_page (selection, 128, "0");
    category.selected = 3;
    wait_for_page (selection, 10, "48");

    entry.text = "U+1F600";
    scope_list.select_row (scope_list.get_row_at_index ((int) Search.Scope.PLANE));
    collection.selected = 2;
    wait_for_page (selection, 1, "128512");
    entry.text = "U+0378";
    scope_list.select_row (scope_list.get_row_at_index ((int) Search.Scope.UNASSIGNED));
    collection.selected = 1;
    wait_for_page (selection, 1, "888");
    selection.selected = 0;
    assert (selected_name.label == "Unassigned Character" && inspect.sensitive);
    assert (!category.visible);
    inspect.clicked ();
    var reserved_dialog = window.get_visible_dialog () as CharacterInspector;
    assert (reserved_dialog != null);
    var reserved_name = find_widget (reserved_dialog, "identity_name") as Gtk.Label;
    assert (reserved_name != null && reserved_name.label == "Unassigned Character");
    assert (reserved_dialog.close ());

    var reset = window.lookup_action ("reset-search");
    assert (reset != null);
    reset.activate (null);
    wait_for_page (selection, 512, "0");
    assert (entry.text == "" && category.selected == 0);
    assert (!inspect.sensitive);
    test_compact_picker (application, window);
    entry.text = "letter";
    window.close ();
}

/** Drives the actual private picker window through the non-unique application lifecycle. */
private int test_client_picker (string executable, bool cancel) {
    var application = new Characters.Application (true);
    Timeout.add (10, () => {
        foreach (var window in application.get_windows ()) {
            var picker = window as CompactPicker;
            if (picker == null) {
                continue;
            }
            var settings = new GLib.Settings (Characters.APP_ID);
            settings.set_strv ("usage-history", {});
            if (cancel) {
                picker.close ();
                return Source.REMOVE;
            }
            var query = find_widget (picker, "picker_query") as Gtk.SearchEntry;
            var scope = find_widget (picker, "picker_scope") as Gtk.DropDown;
            var results = find_widget (picker, "picker_results") as Gtk.FlowBox;
            var commit = find_widget (picker, "picker_commit") as Gtk.Button;
            assert (query != null && scope != null && results != null && commit != null);
            picker.get_clipboard ().set_text ("unchanged");
            scope.selected = 0;
            query.text = "U+1F469 U+200D U+1F4BB";
            wait_for_picker_page (results, 1);
            results.select_child (results.get_child_at_index (0));
            assert (commit.label == "Insert Character");
            commit.clicked ();
            assert (settings.get_strv ("usage-history").length == 0);
            expect_clipboard (picker, "unchanged");
            return Source.REMOVE;
        }
        return Source.CONTINUE;
    });
    return application.exit_status (application.run ({ executable, "--pick-text" }));
}

/** Runs the browser integration test in an isolated graphical and D-Bus session. */
int main (string[] args) {
    if (args.length == 2 && (args[1] == "--client-pick-test" ||
        args[1] == "--client-cancel-test")) {
        return test_client_picker (args[0], args[1] == "--client-cancel-test");
    }
    Test.init (ref args);
    Test.add_func ("/characters/browser/interaction", test_browser);
    return Test.run ();
}
