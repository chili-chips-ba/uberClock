from __future__ import annotations

from PySide6.QtCore import Qt, Signal, Slot
from PySide6.QtGui import QPalette
from PySide6.QtWidgets import QPlainTextEdit


UNHANDLED_KEYS = [Qt.Key.Key_Backspace, Qt.Key.Key_Left, Qt.Key.Key_Right, Qt.Key.Key_Up,
                  Qt.Key.Key_Down]


class Console(QPlainTextEdit):

    get_data = Signal(bytearray)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.m_localEchoEnabled = False
        self.document().setMaximumBlockCount(100)
        p = self.palette()
        p.setColor(QPalette.ColorRole.Base, Qt.GlobalColor.black)
        p.setColor(QPalette.ColorRole.Text, Qt.GlobalColor.green)
        self.setPalette(p)

    @Slot(bytearray)
    def put_data(self, data):
        self.insertPlainText(data.decode("utf8"))
        bar = self.verticalScrollBar()
        bar.setValue(bar.maximum())

    def set_local_echo_enabled(self, e):
        self.m_localEchoEnabled = e

    def keyPressEvent(self, e):
        key = e.key()
        if key not in UNHANDLED_KEYS:
            if self.m_localEchoEnabled:
                super().keyPressEvent(e)
            self.get_data.emit(e.text().encode())

    def mousePressEvent(self, e):
        self.setFocus()

    def mouseDoubleClickEvent(self, e):
        pass

    def contextMenuEvent(self, e):
        pass