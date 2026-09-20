#!/usr/bin/env python3
"""Entry point for the uberClock control panel GUI.

Run from a LiteX venv so `litex_term` is on PATH, e.g.:
    source ~/.venv/litex/bin/activate
    python3 main.py
"""

import sys

from PySide6.QtWidgets import QApplication

from uc_gui.main_window import MainWindow


def main() -> int:
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
