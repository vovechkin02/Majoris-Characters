namespace Characters {
    /** Presents the actual application bindings and context-specific keyboard guidance. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/shortcuts-dialog.ui")]
    internal sealed class ShortcutsDialog : Adw.Dialog {
        /** Displays the registered full-browser search shortcut. */
        [GtkChild]
        private unowned Gtk.Label shortcut_search;
        /** Displays the registered composer shortcut. */
        [GtkChild]
        private unowned Gtk.Label shortcut_composer;
        /** Displays the registered application picker shortcut. */
        [GtkChild]
        private unowned Gtk.Label shortcut_picker;
        /** Displays the registered quit shortcut. */
        [GtkChild]
        private unowned Gtk.Label shortcut_quit;
        /** Displays the registered shortcuts reference accelerator. */
        [GtkChild]
        private unowned Gtk.Label shortcut_help;
        /** Displays configured Shell shortcuts without asserting the extension is enabled. */
        [GtkChild]
        private unowned Gtk.Label shortcut_global;
        /** Hides actions unavailable in a private client picker. */
        [GtkChild]
        private unowned Adw.PreferencesGroup shortcuts_application;

        /** Full browser keyboard guidance, hidden in private client mode. */
        [GtkChild]
        private unowned Adw.PreferencesGroup shortcuts_browser;
        /** Composer editing guidance, hidden in private client mode. */
        [GtkChild]
        private unowned Adw.PreferencesGroup shortcuts_composer_group;

        /** Captures actual bindings when opened; does not change any shortcut or setting. */
        public ShortcutsDialog (Application application) {
            var full_application = !application.is_client_picker ();
            shortcuts_application.visible = full_application;
            shortcuts_browser.visible = full_application;
            shortcuts_composer_group.visible = full_application;
            shortcut_search.label = format_accels (application.get_accels_for_action ("win.search"));
            shortcut_composer.label = format_accels (
                application.get_accels_for_action ("win.open-composer"));
            shortcut_picker.label = format_accels (application.get_accels_for_action ("app.picker"));
            shortcut_quit.label = format_accels (application.get_accels_for_action ("app.quit"));
            shortcut_help.label = format_accels (application.get_accels_for_action ("app.shortcuts"));
            var settings = new GLib.Settings (APP_ID);
            shortcut_global.label = format_accels (settings.get_strv ("picker-shortcut"));
        }

        /** Converts valid GTK accelerator strings to localized key names; skips invalid entries. */
        private static string format_accels (string[] accelerators) {
            string[] labels = {};
            foreach (var accelerator in accelerators) {
                uint key;
                Gdk.ModifierType modifiers;
                if (Gtk.accelerator_parse (accelerator, out key, out modifiers) && key != 0) {
                    labels += Gtk.accelerator_get_label (key, modifiers);
                }
            }
            return labels.length > 0 ? string.joinv (", ", labels) : _("Not assigned");
        }
    }
}
