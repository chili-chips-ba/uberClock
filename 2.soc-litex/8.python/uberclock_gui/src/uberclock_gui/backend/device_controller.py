from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtProperty, pyqtSignal, pyqtSlot


class DeviceController(QObject):
    """Minimal backend object exposed to QML."""

    phaseNcoChanged = pyqtSignal()
    statusTextChanged = pyqtSignal()
    logTextChanged = pyqtSignal()

    def __init__(self) -> None:
        super().__init__()
        self._phase_nco = 10_324_440
        self._status_text = "Disconnected"
        self._log_lines = [
            "Starter backend ready.",
            "Hook serial or direct CSR access into DeviceController.",
        ]

    @pyqtProperty(int, notify=phaseNcoChanged)
    def phaseNco(self) -> int:
        return self._phase_nco

    @pyqtProperty(str, notify=statusTextChanged)
    def statusText(self) -> str:
        return self._status_text

    @pyqtProperty(str, notify=logTextChanged)
    def logText(self) -> str:
        return "\n".join(self._log_lines)

    @pyqtSlot()
    def connectDemo(self) -> None:
        self._status_text = "Demo connected"
        self._append_log("Connected to demo backend.")
        self.statusTextChanged.emit()

    @pyqtSlot(int)
    def setPhaseNco(self, value: int) -> None:
        if value < 0 or value >= (1 << 26):
            self._append_log(f"Rejected phase_nco={value}: valid range is 0..{(1 << 26) - 1}")
            return

        self._phase_nco = int(value)
        self._append_log(f"phase_nco {self._phase_nco}")
        self.phaseNcoChanged.emit()

    def _append_log(self, line: str) -> None:
        self._log_lines.append(line)
        self.logTextChanged.emit()

