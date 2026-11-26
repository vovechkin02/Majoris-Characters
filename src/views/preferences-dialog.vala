namespace Characters {
    /** Local privacy, category visibility, and preview preferences backed by GSettings. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/preferences-dialog.ui")]
    internal sealed class PreferencesDialog : Adw.Dialog {
        /** Confirms history clearing or reports a persistence failure. */
        [GtkChild]
        private unowned Adw.ToastOverlay preferences_toasts;
        /** Enables recording; disabling also discards retained history. */
        [GtkChild]
        private unowned Adw.SwitchRow history_enabled;
        /** Bounds retained distinct history identities. */
        [GtkChild]
        private unowned Adw.SpinRow history_limit;
        /** Main browser glyph size in points. */
        [GtkChild]
        private unowned Adw.SpinRow grid_size;
        /** Font browser glyph size in points. */
        [GtkChild]
        private unowned Adw.SpinRow font_size;
        /** Font browser rendering fallback, separate from coverage. */
        [GtkChild]
        private unowned Adw.SwitchRow font_fallback;
        /** Controls visibility of the emoji sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_emoji;
        /** Controls visibility of the symbol sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_symbols;
        /** Controls visibility of the script sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_scripts;
        /** Controls visibility of the block sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_blocks;
        /** Controls visibility of the named-sequence sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_named;
        /** Controls visibility of the language sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_languages;
        /** Controls visibility of the Han radical sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_radicals;
        /** Controls visibility of the Han stroke sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_strokes;
        /** Controls visibility of the Unicode plane sidebar category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_planes;
        /** Controls visibility of the unassigned scalar inspection category. */
        [GtkChild]
        private unowned Adw.SwitchRow show_unassigned;
        /** Controls visibility of the font sidebar entry. */
        [GtkChild]
        private unowned Adw.SwitchRow show_fonts;
        /** Controls visibility of the legacy-encoding sidebar entry. */
        [GtkChild]
        private unowned Adw.SwitchRow show_encodings;
        /** Opens confirmation for destructive history clearing. */
        [GtkChild]
        private unowned Gtk.Button clear_history;

        /** Shared settings retained for the lifetime of automatic widget bindings. */
        private GLib.Settings settings;
        /** Owns history mutation policy; does not own this dialog. */
        private LibraryStore library;

        /** Binds persisted preferences bidirectionally using native GSettings bindings. */
        public PreferencesDialog (GLib.Settings settings, LibraryStore library) {
            this.settings = settings;
            this.library = library;
            settings.bind ("history-enabled", history_enabled, "active", SettingsBindFlags.GET);
            settings.bind ("history-limit", history_limit, "value", SettingsBindFlags.GET);
            settings.bind ("grid-preview-size", grid_size, "value", SettingsBindFlags.DEFAULT);
            settings.bind ("font-preview-size", font_size, "value", SettingsBindFlags.DEFAULT);
            settings.bind ("font-fallback", font_fallback, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-emoji", show_emoji, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-symbols", show_symbols, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-scripts", show_scripts, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-blocks", show_blocks, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-named", show_named, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-languages", show_languages, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-radicals", show_radicals, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-strokes", show_strokes, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-planes", show_planes, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-unassigned", show_unassigned, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-fonts", show_fonts, "active", SettingsBindFlags.DEFAULT);
            settings.bind ("show-encodings", show_encodings, "active", SettingsBindFlags.DEFAULT);
            clear_history.clicked.connect (confirm_clear);
            history_enabled.notify["active"].connect (change_history_enabled);
            history_limit.notify["value"].connect (change_history_limit);
        }

        /** Commit opt-out and clearing together; restore the actual policy on rejection. */
        private void change_history_enabled () {
            if (history_enabled.active == library.history_enabled ()) {
                return;
            }
            if (!library.set_history_enabled (history_enabled.active)) {
                history_enabled.active = library.history_enabled ();
                var message = !history_enabled.active ?
                    _("Recording Disabled; Unable to Clear History. Check Settings Permissions.") :
                    _("Unable to Update History Settings. Try Again or Check Settings Permissions.");
                preferences_toasts.add_toast (new Adw.Toast (message));
            }
        }

        /** Commit retention and trimming together rather than writing the policy separately. */
        private void change_history_limit () {
            var limit = (int) history_limit.value;
            if (limit == library.history_limit ()) {
                return;
            }
            if (!library.set_history_limit (limit)) {
                history_limit.value = library.history_limit ();
                preferences_toasts.add_toast (new Adw.Toast (
                    _("Unable to Update History Settings. Try Again or Check Settings Permissions.")));
            }
        }

        /** Prompts before removing usage records while keeping favorites intact. */
        private void confirm_clear () {
            var dialog = new Adw.AlertDialog (_("Clear Character History?"),
                _("Recently and frequently used characters will be removed. Favorites are kept."));
            dialog.add_response ("cancel", _("Cancel"));
            dialog.add_response ("clear", _("Clear History"));
            dialog.set_response_appearance ("clear", Adw.ResponseAppearance.DESTRUCTIVE);
            dialog.set_default_response ("cancel");
            dialog.set_close_response ("cancel");
            dialog.response.connect ((response) => {
                if (response == "clear") {
                    var message = library.clear_history () ? _("History Cleared") :
                        _("Unable to Clear History. Check Settings Permissions.");
                    preferences_toasts.add_toast (new Adw.Toast (message));
                }
            });
            dialog.present (this);
        }
    }
}
