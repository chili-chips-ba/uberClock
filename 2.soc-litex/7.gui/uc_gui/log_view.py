"""A read-only log view that understands ANSI color codes.

Kept separate from ``ansi_text.py``'s parsing logic so the Qt-specific
rendering (QTextCharFormat, cursor management) doesn't need a Qt
application instance to unit-test the parsing itself.
"""

from __future__ import annotations

from PySide6.QtGui import QColor, QTextCharFormat, QTextCursor
from PySide6.QtWidgets import QTextEdit

from .ansi_text import TextStyle, split_ansi


class AnsiLogView(QTextEdit):
    """Appends text from a serial/litex_term stream with ANSI SGR codes
    rendered as color/bold instead of shown as raw escape bytes."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setReadOnly(True)
        self._style = TextStyle()
        self._pending = ""  # possible incomplete escape sequence from the last chunk

    def append_ansi(self, text: str) -> None:
        segments, self._style, self._pending = split_ansi(self._pending + text, self._style)

        cursor = self.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        for segment in segments:
            char_format = QTextCharFormat()
            if segment.style.color is not None:
                char_format.setForeground(QColor(segment.style.color))
            char_format.setFontWeight(700 if segment.style.bold else 400)
            cursor.insertText(segment.text, char_format)

        self.setTextCursor(cursor)
        self.ensureCursorVisible()
