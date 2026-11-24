namespace Characters {
    /** Owns application actions and the single active main window on the GTK main context. */
    internal sealed class Application : Adw.Application {
        /** Creates the application with its stable desktop identity and local version option. */
        public Application () {
            Object (application_id: APP_ID, flags: ApplicationFlags.DEFAULT_FLAGS);
            add_main_option ("version", 'v', OptionFlags.NONE, OptionArg.NONE,
                _("Show the application version"), null);
            add_main_option ("unicode-version", 0, OptionFlags.NONE, OptionArg.NONE,
                _("Show the Unicode database versions"), null);
        }

        /** Answers version requests locally without requiring a graphical session. */
        protected override int handle_local_options (VariantDict options) {
            if (options.contains ("version")) {
                print ("Characters %s\n", VERSION);
                return 0;
            }
            if (options.contains ("unicode-version")) {
                print ("Catalog: %s\nProperties: %s\nSupplement: %s\n",
                    Unicode.catalog_version (), Unicode.properties_version () ?? "unknown",
                    Unicode.supplement_version ());
                print ("Emoji annotations: %s\n", Emoji.annotation_source ());
                return 0;
            }
            return -1;
        }

        /** Registers application actions once on the primary instance. */
        protected override void startup () {
            base.startup ();

            var about_action = new SimpleAction ("about", null);
            about_action.activate.connect (show_about);
            add_action (about_action);

            var quit_action = new SimpleAction ("quit", null);
            quit_action.activate.connect (close_window);
            add_action (quit_action);
            set_accels_for_action ("app.quit", { "<Primary>q" });
            set_accels_for_action ("win.search", { "<Primary>f" });
            set_accels_for_action ("win.open-composer", { "<Primary>m" });
        }

        /** Presents the existing main window or constructs one for a new session. */
        protected override void activate () {
            Gtk.Window? window = active_window;
            if (window == null) {
                window = new MainWindow (this);
            }
            window.present ();
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

        /** Routes Quit through normal window closing so its settings are saved. */
        private void close_window (Variant? parameter) {
            var window = active_window;
            if (window != null) {
                window.close ();
            } else {
                quit ();
            }
        }
    }
}
