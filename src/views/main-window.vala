namespace Characters {
    /** Restores and saves the main window geometry and displays the foundation interface. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/main-window.ui")]
    internal sealed class MainWindow : Adw.ApplicationWindow {
        /** Owns the settings object for this window's lifetime. */
        private GLib.Settings settings;

        /** Constructs the XML-backed window and restores the last normal logical size. */
        public MainWindow (Application application) {
            Object (application: application);
            settings = new GLib.Settings (APP_ID);
            set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
            if (settings.get_boolean ("window-maximized")) {
                maximize ();
            }
        }

        /** Persists requested default dimensions rather than mapped allocation, plus maximized state. */
        public override bool close_request () {
            int width_px;
            int height_px;
            get_default_size (out width_px, out height_px);
            settings.set_int ("window-width", width_px.clamp (320, 8192));
            settings.set_int ("window-height", height_px.clamp (320, 8192));
            settings.set_boolean ("window-maximized", is_maximized ());
            GLib.Settings.sync ();
            return false;
        }
    }
}
