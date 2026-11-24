#!/usr/bin/python3
"""Optional IBus engine for direct Unicode insertion into the current input context."""

import argparse
import gettext
import shutil

import gi

gi.require_version("IBus", "1.0")
from gi.repository import Gio, GLib, IBus

from input_protocol import MAX_QUERY_BYTES, decode_results

_ = gettext.gettext
BUS_NAME = "com.majoris.Characters.Input"
ENGINE_NAME = "majoris-characters"


class CharacterEngine(IBus.Engine):
    """Own one input-context query, canceling it on focus loss or supersession."""

    def __init__(self, **kwargs):
        """Initialize an idle engine which forwards ordinary keystrokes untouched."""
        super().__init__(**kwargs)
        self.active = False
        self.sensitive_context = False
        self.query = ""
        self.results = []
        self.table = None
        self.process = None
        self.generation = 0
        self.timer = 0

    def _cancel_search(self):
        """Discard pending callbacks and terminate the old local search subprocess."""
        self.generation += 1
        if self.timer:
            GLib.source_remove(self.timer)
            self.timer = 0
        if self.process is not None:
            self.process.force_exit()
            self.process = None

    def _reset_query(self):
        """Clear preedit and candidates without committing anything to the client."""
        self._cancel_search()
        self.active = False
        self.query = ""
        self.results = []
        self.table = None
        self.hide_preedit_text()
        self.hide_lookup_table()
        self.hide_auxiliary_text()

    def _refresh(self):
        """Render current query immediately and debounce the bounded catalog search."""
        self._cancel_search()
        self.results = []
        self.table = None
        self.hide_lookup_table()
        self.update_preedit_text_with_mode(
            IBus.Text.new_from_string(self.query), len(self.query), True,
            IBus.PreeditFocusMode.CLEAR,
        )
        self.update_auxiliary_text(IBus.Text.new_from_string(
            _("Type a character name or code point; Enter inserts, Escape cancels.")), True)
        if self.query.strip():
            self.timer = GLib.timeout_add(100, self._start_search)

    def _start_search(self):
        """Start the local executable without a shell; finish on the GLib main loop."""
        self.timer = 0
        executable = shutil.which("majoris-characters")
        if not self.active or executable is None:
            return GLib.SOURCE_REMOVE
        try:
            process = Gio.Subprocess.new(
                [executable, "--input-search=" + self.query],
                Gio.SubprocessFlags.STDOUT_PIPE | Gio.SubprocessFlags.STDERR_SILENCE,
            )
        except GLib.Error:
            self.update_auxiliary_text(IBus.Text.new_from_string(_("Search unavailable")), True)
            return GLib.SOURCE_REMOVE
        self.process = process
        generation = self.generation
        process.communicate_utf8_async(None, None, self._search_finished, generation)
        return GLib.SOURCE_REMOVE

    def _search_finished(self, process, result, generation):
        """Consume completion but never publish results for an obsolete input context."""
        try:
            _, stdout, _stderr = process.communicate_utf8_finish(result)
            values = decode_results(stdout or "") if process.get_successful() else []
        except (GLib.Error, ValueError):
            values = []
        if generation != self.generation or not self.active:
            return
        self.process = None
        self.results = values
        if not values:
            self.update_auxiliary_text(IBus.Text.new_from_string(_("No matching characters")), True)
            return
        self.table = IBus.LookupTable.new(8, 0, True, False)
        for text, name in values:
            self.table.append_candidate(IBus.Text.new_from_string(f"{text}  {name}"))
        self.update_lookup_table(self.table, True)

    def _commit(self, index):
        """Commit only a currently displayed exact identity through the IBus context."""
        if self.active and 0 <= index < len(self.results):
            text = self.results[index][0]
            self._reset_query()
            self.commit_text(IBus.Text.new_from_string(text))
            self._record_insertion(text)

    def _record_insertion(self, text):
        """Route explicit insertion history through the primary application, not a second store."""
        executable = shutil.which("majoris-characters")
        if executable is None:
            return
        try:
            process = Gio.Subprocess.new(
                [executable, "--record-insertion=" + text],
                Gio.SubprocessFlags.STDOUT_SILENCE | Gio.SubprocessFlags.STDERR_SILENCE,
            )
            process.wait_async(None, self._history_finished, None)
        except GLib.Error:
            # History failure must not undo a successfully dispatched input commit.
            return

    def _history_finished(self, process, result, _user_data):
        """Reap the history subprocess without changing the committed text."""
        try:
            process.wait_finish(result)
        except GLib.Error:
            return

    def do_process_key_event(self, keyval, keycode, state):
        """Consume the explicit picker shortcut and active query keys only."""
        if self.sensitive_context or state & IBus.ModifierType.RELEASE_MASK:
            return False
        control = IBus.ModifierType.CONTROL_MASK
        shift = IBus.ModifierType.SHIFT_MASK
        if not self.active:
            if keyval in (IBus.KEY_period, IBus.KEY_greater) and state & control and state & shift:
                self.active = True
                self._refresh()
                return True
            return False
        if keyval == IBus.KEY_Escape:
            self._reset_query()
            return True
        if keyval in (IBus.KEY_Return, IBus.KEY_KP_Enter):
            if self.table is not None:
                self._commit(self.table.get_cursor_pos())
            return True
        if self.table is not None and keyval in (IBus.KEY_Up, IBus.KEY_Down,
                                                  IBus.KEY_Page_Up, IBus.KEY_Page_Down):
            if keyval == IBus.KEY_Up:
                self.table.cursor_up()
            elif keyval == IBus.KEY_Down:
                self.table.cursor_down()
            elif keyval == IBus.KEY_Page_Up:
                self.table.page_up()
            else:
                self.table.page_down()
            self.update_lookup_table(self.table, True)
            return True
        if keyval == IBus.KEY_BackSpace:
            self.query = self.query[:-1]
            self._refresh()
            return True
        if state & (control | IBus.ModifierType.MOD1_MASK | IBus.ModifierType.SUPER_MASK):
            self._reset_query()
            return False
        scalar = IBus.keyval_to_unicode(keyval)
        if scalar and chr(scalar).isprintable():
            candidate = self.query + chr(scalar)
            if len(candidate.encode("utf-8")) <= MAX_QUERY_BYTES:
                self.query = candidate
                self._refresh()
            return True
        return False

    def do_candidate_clicked(self, index, button, state):
        """Accept a candidate on the current lookup-table page."""
        if button == 1 and self.table is not None:
            page_size = self.table.get_page_size()
            start = self.table.get_cursor_pos() // page_size * page_size
            if 0 <= index < page_size:
                self._commit(start + index)

    def do_set_content_type(self, purpose, hints):
        """Never search or record in clients declaring password, PIN or private input."""
        private_hint = getattr(IBus.InputHints, "PRIVATE", 0)
        self.sensitive_context = purpose in (IBus.InputPurpose.PASSWORD, IBus.InputPurpose.PIN)
        self.sensitive_context = self.sensitive_context or bool(hints & private_hint)
        if self.sensitive_context:
            self._reset_query()

    def do_focus_out(self):
        """Invalidate all candidates when the input context loses focus."""
        self._reset_query()

    def do_reset(self):
        """Handle input-client resets without committing a pending selection."""
        self._reset_query()

    def do_disable(self):
        """Cancel search when the user changes input sources."""
        self._reset_query()


def main() -> int:
    """Register the IBus factory on its own bus and follow its lifecycle."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ibus", action="store_true", required=True)
    parser.parse_args()
    gettext.textdomain("majoris-characters")
    IBus.init()
    bus = IBus.Bus.new()
    if not bus.is_connected():
        return 1
    loop = GLib.MainLoop()
    bus.connect("disconnected", lambda *_args: loop.quit())
    factory = IBus.Factory.new(bus.get_connection())
    factory.add_engine(ENGINE_NAME, CharacterEngine)
    if bus.request_name(BUS_NAME, 0) != 1:
        return 1
    loop.run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
