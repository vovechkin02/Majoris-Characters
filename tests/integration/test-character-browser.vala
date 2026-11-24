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
    entry.text = "byte:00";
    wait_for_page (selection, 1, "0");
    selection.selected = 0;
    assert (!copy.sensitive && bytes.sensitive);
    entry.text = "byte:GG";
    assert (selection.model == null && !bytes.sensitive);
    assert (dialog.close ());
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
    preview.value = 36;
    fallback.active = true;
    assert (selection.selected == 0 && detail.label.contains ("DejaVu Sans"));
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
    assert (dialog.activate_action_variant ("inspect.related", new Variant.string ("U+1F469")));
    assert (identity.label == "U+1F469");
    assert (dialog.activate_action_variant ("inspect.back", null));
    assert (identity.label == "U+1F469 U+200D U+1F4BB");

    input.buffer.text = "U+D800";
    assert (dialog.activate_action_variant ("inspect.identify-code", null));
    assert (error_label.visible && identity.label == "U+1F469 U+200D U+1F4BB");
    input.buffer.text = "U+0000";
    assert (dialog.activate_action_variant ("inspect.identify-code", null));
    assert (!error_label.visible && identity.label == "U+0000" && !copy.sensitive);
    assert (dialog.activate_action_variant ("inspect.copy", new Variant.string ("00")));
    expect_clipboard (dialog, "00");
    dialog.force_close ();
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
    test_inspector (window);
    assert (selected_code.label == "U+20AC");
    test_legacy_browser (window);
    test_font_browser (window);
    assert (selected_code.label == "U+20AC");

    entry.text = "";
    scope_list.select_row (scope_list.get_row_at_index ((int) Search.Scope.BLOCK));
    collection.selected = 1;
    wait_for_page (selection, 128, "0");
    category.selected = 3;
    wait_for_page (selection, 10, "48");

    var reset = window.lookup_action ("reset-search");
    assert (reset != null);
    reset.activate (null);
    wait_for_page (selection, 512, "0");
    assert (entry.text == "" && category.selected == 0);
    assert (!inspect.sensitive);
    entry.text = "letter";
    window.close ();
}

/** Runs the browser integration test in an isolated graphical and D-Bus session. */
int main (string[] args) {
    Test.init (ref args);
    Test.add_func ("/characters/browser/interaction", test_browser);
    return Test.run ();
}
