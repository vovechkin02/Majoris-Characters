import Gio from 'gi://Gio';
import Meta from 'gi://Meta';
import Shell from 'gi://Shell';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';

/** Registers only the opt-in picker shortcut; never intercepts text input. */
export default class CharactersPickerExtension extends Extension {
    /** Registers the configurable binding in the normal desktop and overview. */
    enable() {
        this._settings = new Gio.Settings({schema_id: 'com.majoris.characters'});
        Main.wm.addKeybinding('picker-shortcut', this._settings,
            Meta.KeyBindingFlags.NONE, Shell.ActionMode.NORMAL | Shell.ActionMode.OVERVIEW,
            () => this._openPicker());
    }

    /** Delegates activation to the registered desktop application action. */
    _openPicker() {
        const app = Gio.DesktopAppInfo.new('com.majoris.characters.desktop');
        if (app !== null) {
            app.launch_action('picker', global.create_app_launch_context(0, -1));
        }
    }

    /** Removes only this extension's binding and releases its settings. */
    disable() {
        Main.wm.removeKeybinding('picker-shortcut');
        this._settings = null;
    }
}
