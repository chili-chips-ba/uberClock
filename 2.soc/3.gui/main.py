import sys

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QApplication, QMainWindow, QDockWidget

sys.path.insert(0, "terminal")  # so console.py / settingsdialog.py imports resolve
from serial_terminal import SerialTerminal

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("uberClock")
        self.resize(1024, 800)

        self.terminal_panel = SerialTerminal()
        dock = QDockWidget("Terminal", self)
        dock.setWidget(self.terminal_panel)
        dock.setAllowedAreas(Qt.DockWidgetArea.AllDockWidgetAreas)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, dock)


app = QApplication(sys.argv)
window = MainWindow()
window.show()
app.exec()