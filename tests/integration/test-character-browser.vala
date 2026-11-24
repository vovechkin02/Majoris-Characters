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
    entry.text = "letter";
    window.close ();
}

/** Runs the browser integration test in an isolated graphical and D-Bus session. */
int main (string[] args) {
    Test.init (ref args);
    Test.add_func ("/characters/browser/interaction", test_browser);
    return Test.run ();
}
