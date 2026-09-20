import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from uc_gui.log_view import AnsiLogView

_app = QApplication.instance() or QApplication([])


class AnsiLogViewTests(unittest.TestCase):
    def test_full_escape_sequence_in_one_chunk_leaves_no_raw_bytes(self):
        view = AnsiLogView()
        view.append_ansi("\x1b[92;1muberClock\x1b[0m> ")
        self.assertNotIn("\x1b", view.toPlainText())
        self.assertEqual(view.toPlainText(), "uberClock> ")

    def test_escape_sequence_split_across_two_appends_still_renders_clean(self):
        # Mirrors real serial reads: an os.read() can return a chunk that
        # ends mid-escape-sequence. This used to leak a raw ESC byte into
        # the visible text (reported live from real hardware).
        view = AnsiLogView()
        view.append_ansi("before \x1b[92;1")
        view.append_ansi("muberClock\x1b[0m> ")
        self.assertNotIn("\x1b", view.toPlainText())
        self.assertEqual(view.toPlainText(), "before uberClock> ")

    def test_split_byte_by_byte_still_renders_clean(self):
        # Worst case: every byte arrives as its own chunk.
        view = AnsiLogView()
        text = "\x1b[92;1muberClock\x1b[0m> cap_start\n"
        for ch in text:
            view.append_ansi(ch)
        self.assertNotIn("\x1b", view.toPlainText())
        self.assertEqual(view.toPlainText(), "uberClock> cap_start\n")

    def test_doubled_prompt_from_the_field_renders_clean(self):
        view = AnsiLogView()
        view.append_ansi("\x1b[92;1m\x1b[92;1muberClock\x1b[0m>\x1b[0m > cap_start\n")
        self.assertNotIn("\x1b", view.toPlainText())
        self.assertEqual(view.toPlainText(), "uberClock> > cap_start\n")


if __name__ == "__main__":
    unittest.main()
