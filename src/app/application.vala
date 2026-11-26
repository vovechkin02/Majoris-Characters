namespace Characters {
    /** Owns application actions and the single active main window on the GTK main context. */
    internal sealed class Application : Adw.Application {
        /** Search provider exported on the primary application connection. */
        private SearchProvider? provider;
        /** Registration ID used to unexport the provider exactly once. */
        private uint provider_id;
        /** Keeps clipboard ownership alive when the compact picker is closed. */
        private Gdk.Clipboard? picker_clipboard;
        /** Signal subscription owned while the application holds clipboard text. */
        private ulong clipboard_handler;
        /** Independent client process: no desktop bus ownership or clipboard commit. */
        private bool client_mode;
        /** Zero only after writing one exact selection to the client's stdout pipe. */
        private int client_status = 1;

        /** Creates the application with its stable desktop identity and local version option. */
        public Application (bool client_mode = false) {
            Object (application_id: client_mode ? null : APP_ID,
                flags: ApplicationFlags.HANDLES_COMMAND_LINE |
                (client_mode ? ApplicationFlags.NON_UNIQUE : ApplicationFlags.DEFAULT_FLAGS));
            this.client_mode = client_mode;
            add_main_option ("pick-text", 0, OptionFlags.NONE, OptionArg.NONE,
                _("Return one explicitly selected character to the requesting application"), null);
            add_main_option ("version", 'v', OptionFlags.NONE, OptionArg.NONE,
                _("Show the application version"), null);
            add_main_option ("unicode-version", 0, OptionFlags.NONE, OptionArg.NONE,
                _("Show the Unicode database versions"), null);
            add_main_option ("picker", 0, OptionFlags.NONE, OptionArg.NONE,
                _("Open the compact character picker"), null);
            add_main_option ("search", 0, OptionFlags.NONE, OptionArg.STRING,
                _("Search the full character catalog"), _("QUERY"));
            add_main_option ("input-search", 0, OptionFlags.NONE, OptionArg.STRING,
                _("Return bounded input-source results without opening a window"), _("QUERY"));
            add_main_option ("input-search-stdin", 0, OptionFlags.NONE, OptionArg.NONE,
                _("Read a bounded input-source query from standard input"), null);
            add_main_option ("record-insertion", 0, OptionFlags.NONE, OptionArg.STRING,
                _("Record an explicitly inserted character without opening a window"), _("TEXT"));
        }

        /** Answers version requests locally without requiring a graphical session. */
        protected override int handle_local_options (VariantDict options) {
            if (options.contains ("pick-text") && !client_mode) {
                printerr (_("Use --pick-text without other arguments.\n"));
                return 2;
            }
            if (options.contains ("version")) {
                print ("Characters %s\n", VERSION);
                return 0;
            }
            if (options.contains ("unicode-version")) {
                print ("Catalog: %s\nProperties: %s\nSupplement: %s\n",
                    Unicode.catalog_version (), Unicode.properties_version () ?? "unknown",
                    Unicode.supplement_version ());
                print ("Emoji annotations: %s\n", Emoji.annotation_source ());
                print ("Official Unihan: %s\n", Unicode.unihan_version ());
                return 0;
            }
            var input = options.lookup_value ("input-search", VariantType.STRING);
            if (options.contains ("input-search-stdin")) {
                if (input != null) {
                    return 2;
                }
                var query = DesktopSearch.read_input_query ();
                return query != null ? DesktopSearch.print_input_results (query) : 2;
            }
            if (input != null) {
                return DesktopSearch.print_input_results (input.get_string ());
            }
            return -1;
        }

        /** Registers application actions once on the primary instance. */
        protected override void startup () {
            base.startup ();
            var shortcuts_action = new SimpleAction ("shortcuts", null);
            shortcuts_action.activate.connect (show_shortcuts);
            add_action (shortcuts_action);
            set_accels_for_action ("app.shortcuts", { "<Primary>question" });
            if (client_mode) {
                return;
            }

            var picker_action = new SimpleAction ("picker", null);
            picker_action.activate.connect (() => {
                open_picker ();
            });
            add_action (picker_action);
            var about_action = new SimpleAction ("about", null);
            about_action.activate.connect (show_about);
            add_action (about_action);

            var quit_action = new SimpleAction ("quit", null);
            quit_action.activate.connect (close_window);
            add_action (quit_action);
            set_accels_for_action ("app.picker", { "<Primary><Shift>period" });
            set_accels_for_action ("app.quit", { "<Primary>q" });
            set_accels_for_action ("win.search", { "<Primary>f" });
            set_accels_for_action ("win.open-composer", { "<Primary>m" });
        }

        /** Exports SearchProvider2 alongside the standard GApplication interfaces. */
        protected override bool dbus_register (DBusConnection connection, string object_path)
            throws Error {
            if (!base.dbus_register (connection, object_path)) {
                return false;
            }
            if (client_mode) {
                return true;
            }
            provider = new SearchProvider (this);
            try {
                provider_id = connection.register_object<ShellSearchProvider> (
                    object_path + "/SearchProvider", provider);
            } catch (Error error) {
                provider = null;
                base.dbus_unregister (connection, object_path);
                throw error;
            }
            return true;
        }

        /** Stops outstanding searches and releases the exported object. */
        protected override void dbus_unregister (DBusConnection connection, string object_path) {
            if (provider_id != 0) {
                provider.shutdown ();
                connection.unregister_object (provider_id);
                provider_id = 0;
                provider = null;
            }
            base.dbus_unregister (connection, object_path);
        }

        /** Routes command-line activation through the primary graphical instance. */
        protected override int command_line (ApplicationCommandLine command) {
            if (client_mode) {
                open_picker ();
                return 0;
            }
            var options = command.get_options_dict ();
            var insertion = options.lookup_value ("record-insertion", VariantType.STRING);
            if (insertion != null) {
                var library = new LibraryStore (new GLib.Settings (APP_ID));
                var text = insertion.get_string ();
                if (!LibraryStore.is_valid (text)) {
                    return 2;
                }
                var saved = library.record_use (text);
                GLib.Settings.sync ();
                return saved ? 0 : 1;
            }
            var query = options.lookup_value ("search", VariantType.STRING);
            if (query != null) {
                if (!DesktopSearch.valid_query (query.get_string ())) {
                    command.printerr (_("Invalid character query\n"));
                    return 2;
                }
                open_search (query.get_string (), 0);
            } else if (options.contains ("picker")) {
                open_picker ();
            } else if (command.get_arguments ().length > 1) {
                command.printerr (_("Unexpected command-line argument\n"));
                return 2;
            } else {
                activate ();
            }
            return 0;
        }

        /** Presents the main browser even when the compact picker is active. */
        protected override void activate () {
            if (client_mode) {
                open_picker ();
            } else {
                main_window ().present ();
            }
        }

        /** Reuses one full browser; the compact picker has a separate lifetime. */
        private MainWindow main_window () {
            foreach (var window in get_windows ()) {
                if (window is MainWindow) {
                    return (MainWindow) window;
                }
            }
            return new MainWindow (this);
        }

        /** Opens or focuses the independent compact picker. */
        private void open_picker () {
            foreach (var window in get_windows ()) {
                if (window is CompactPicker) {
                    window.present ();
                    ((CompactPicker) window).focus_query ();
                    return;
                }
            }
            var picker = new CompactPicker (this);
            picker.present ();
            picker.focus_query ();
        }

        /** Activates the full browser using a query and the supplied desktop timestamp. */
        public void open_search (string query, uint timestamp) {
            var window = main_window ();
            window.search_catalog (query);
            window.present_with_time (timestamp);
        }

        /** Transfers the compact catalog context into the independent main window. */
        public void open_picker_context (Search.Scope scope, int filter, string query,
                                         uint position, uint32 id) {
            var window = main_window ();
            window.present ();
            window.restore_picker_context (scope, filter, query, position, id);
        }

        /** Transfers a saved collection into its full browser dialog. */
        public void open_library_context (LibraryMode mode, string query, string? text,
                                          uint position) {
            var window = main_window ();
            window.present ();
            window.restore_library_context (mode, query, text, position);
        }

        /** Returns whether the picker is serving a single private subprocess request. */
        public bool is_client_picker () {
            return client_mode;
        }

        /** Returns the exact selection without copying or recording speculative insertion. */
        public void finish_client_pick (string text) {
            if (!client_mode || client_status != 1 || !LibraryStore.is_valid (text)) {
                return;
            }
            client_status = stdout.printf ("%s", text) >= 0 && stdout.flush () == 0 ? 0 : 2;
        }

        /** Preserves framework errors and reports a canceled private request as exit status 1. */
        public int exit_status (int status) {
            return status == 0 && client_mode ? client_status : status;
        }

        /** Opens the selected Unicode identity in its own inspector. */
        public void open_identity (string code, uint timestamp) {
            var window = main_window ();
            window.present_with_time (timestamp);
            window.inspect_code (code);
        }

        /** Retains clipboard ownership for copies from a compact picker or its nested dialogs. */
        public static void copy_widget_text (Gtk.Widget widget, string text) {
            var picker = widget.get_root () as CompactPicker;
            var application = picker != null ? picker.application as Application : null;
            if (application != null && !application.is_client_picker ()) {
                application.copy_from_picker (widget.get_clipboard (), text);
            } else {
                widget.get_clipboard ().set_text (text);
            }
        }

        /** Retains the primary process until another owner replaces the picker clipboard. */
        public void copy_from_picker (Gdk.Clipboard clipboard, string text) {
            if (picker_clipboard != clipboard) {
                release_clipboard ();
                hold ();
                picker_clipboard = clipboard;
                clipboard_handler = clipboard.changed.connect (on_clipboard_changed);
            }
            clipboard.set_text (text);
        }

        /** Releases the clipboard hold once its contents are no longer locally owned. */
        private void on_clipboard_changed () {
            if (picker_clipboard != null && !picker_clipboard.is_local ()) {
                release_clipboard ();
            }
        }

        /** Disconnects the clipboard observer before dropping the process hold. */
        private void release_clipboard () {
            if (picker_clipboard != null) {
                picker_clipboard.disconnect (clipboard_handler);
                clipboard_handler = 0;
                picker_clipboard = null;
                release ();
            }
        }

        /** Presents version and license information using the system's native dialog. */
        private void show_about (Variant? parameter) {
            var window = active_window;
            if (window == null) {
                return;
            }
            var dialog = new Adw.AboutDialog () {
                application_name = _("Characters"),
                application_icon = APP_ID,
                developer_name = "Majoris",
                version = VERSION,
                license_type = Gtk.License.GPL_3_0,
                website = "https://github.com/vovechkin02/Majoris-Characters"
            };
            dialog.present (window);
        }

        /** Opens the shortcut reference for the current window, including private pickers. */
        private void show_shortcuts (Variant? parameter) {
            var window = active_window;
            if (window != null) {
                new ShortcutsDialog (this).present (window);
            }
        }

        /** Routes Quit through normal window closing so its settings are saved. */
        private void close_window (Variant? parameter) {
            release_clipboard ();
            Gtk.Window[] windows = {};
            foreach (var window in get_windows ()) {
                windows += window;
            }
            foreach (var window in windows) {
                window.close ();
            }
            quit ();
        }
    }
}
