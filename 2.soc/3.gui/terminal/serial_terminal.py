from __future__ import annotations

from PySide6.QtCore import QIODeviceBase, Slot
from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QMessageBox
)
from PySide6.QtSerialPort import QSerialPort

from console import Console
from settingsdialog import SettingsDialog


def description(s):
    return (f"Connected to {s.name} : {s.string_baud_rate}, "
            f"{s.string_data_bits}, {s.string_parity}, {s.string_stop_bits}, "
            f"{s.string_flow_control}")


class SerialTerminal(QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.m_console = Console()
        self.m_console.setEnabled(False)
        self.m_settings = SettingsDialog(self)
        self.m_serial = QSerialPort(self)
        self.m_status = QLabel("Disconnected")

        self.btn_connect = QPushButton("Connect")
        self.btn_disconnect = QPushButton("Disconnect")
        self.btn_configure = QPushButton("Configure")
        self.btn_clear = QPushButton("Clear")
        self.btn_disconnect.setEnabled(False)

        toolbar = QHBoxLayout()
        for w in (self.btn_connect, self.btn_disconnect, self.btn_configure, self.btn_clear):
            toolbar.addWidget(w)
        toolbar.addStretch()
        toolbar.addWidget(self.m_status)

        layout = QVBoxLayout(self)
        layout.addLayout(toolbar)
        layout.addWidget(self.m_console)

        self.btn_connect.clicked.connect(self.open_serial_port)
        self.btn_disconnect.clicked.connect(self.close_serial_port)
        self.btn_configure.clicked.connect(self.m_settings.show)
        self.btn_clear.clicked.connect(self.m_console.clear)

        self.m_serial.errorOccurred.connect(self.handle_error)
        self.m_serial.readyRead.connect(self.read_data)
        self.m_console.get_data.connect(self.write_data)

    @Slot()
    def open_serial_port(self):
        s = self.m_settings.settings()
        self.m_serial.setPortName(s.name)
        self.m_serial.setBaudRate(s.baud_rate)
        self.m_serial.setDataBits(s.data_bits)
        self.m_serial.setParity(s.parity)
        self.m_serial.setStopBits(s.stop_bits)
        self.m_serial.setFlowControl(s.flow_control)
        if self.m_serial.open(QIODeviceBase.OpenModeFlag.ReadWrite):
            self.m_console.setEnabled(True)
            self.m_console.set_local_echo_enabled(s.local_echo_enabled)
            self.btn_connect.setEnabled(False)
            self.btn_disconnect.setEnabled(True)
            self.btn_configure.setEnabled(False)
            self.m_status.setText(description(s))
        else:
            QMessageBox.critical(self, "Error", self.m_serial.errorString())
            self.m_status.setText("Open error")

    @Slot()
    def close_serial_port(self):
        if self.m_serial.isOpen():
            self.m_serial.close()
        self.m_console.setEnabled(False)
        self.btn_connect.setEnabled(True)
        self.btn_disconnect.setEnabled(False)
        self.btn_configure.setEnabled(True)
        self.m_status.setText("Disconnected")

    @Slot(bytearray)
    def write_data(self, data):
        self.m_serial.write(data)

    @Slot()
    def read_data(self):
        data = self.m_serial.readAll()
        self.m_console.put_data(data.data())

    @Slot(QSerialPort.SerialPortError)
    def handle_error(self, error):
        if error == QSerialPort.SerialPortError.ResourceError:
            QMessageBox.critical(self, "Critical Error", self.m_serial.errorString())
            self.close_serial_port()