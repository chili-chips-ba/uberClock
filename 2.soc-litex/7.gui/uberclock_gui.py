#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import glob
import shutil
import subprocess
import threading
import queue
import select
import pty
from dataclasses import dataclass

from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QFormLayout,
    QPushButton, QTextEdit, QLineEdit, QComboBox, QLabel, QFileDialog, QMessageBox,
    QGroupBox, QFrame, QScrollArea, QSpinBox, QDoubleSpinBox
)
from PySide6.QtCore import QTimer, Qt

import serial.tools.list_ports


# ----------------------------
# Config
# ----------------------------
DEFAULT_BIN_DIR = "/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/2.soc-litex/2.sw"

BAUD_NOTE = "litex_term handles baud; GUI uses litex_term exactly like make term."


def list_ports():
    return [p.device for p in serial.tools.list_ports.comports()]


def newest_bin(path: str) -> str:
    bins = glob.glob(os.path.join(path, "*.bin"))
    if not bins:
        return ""
    return max(bins, key=os.path.getmtime)


# ----------------------------
# Backend: litex_term in PTY
# ----------------------------
class LitexTermWorker:
    def __init__(self):
        self.proc: subprocess.Popen | None = None
        self.rxq = queue.Queue()
        self._master_fd: int | None = None
        self._reader_th: threading.Thread | None = None

    def start(self, cmd: list[str], env: dict | None = None):
        self.stop()

        master_fd, slave_fd = pty.openpty()
        self._master_fd = master_fd

        self.proc = subprocess.Popen(
            cmd,
            stdin=slave_fd,
            stdout=slave_fd,
            stderr=slave_fd,
            env=env,
            close_fds=True,
            text=False,
        )
        os.close(slave_fd)

        self._reader_th = threading.Thread(target=self._reader, daemon=True)
        self._reader_th.start()

    def _reader(self):
        try:
            while self.proc and self.proc.poll() is None and self._master_fd is not None:
                r, _, _ = select.select([self._master_fd], [], [], 0.1)
                if not r:
                    continue
                data = os.read(self._master_fd, 4096)
                if not data:
                    break
                self.rxq.put(data.decode(errors="replace"))
        except Exception as e:
            self.rxq.put(f"\n[pty reader error] {e}\n")

        if self.proc:
            rc = self.proc.poll()
            if rc is not None:
                self.rxq.put(f"\n[litex_term exited] code={rc}\n")

    def send(self, s: str):
        if not self.proc or self.proc.poll() is not None:
            raise RuntimeError("litex_term is not running")
        if self._master_fd is None:
            raise RuntimeError("PTY not available")
        os.write(self._master_fd, (s.rstrip("\r\n") + "\n").encode())

    def stop(self):
        if self.proc:
            try:
                if self.proc.poll() is None:
                    self.proc.terminate()
                    try:
                        self.proc.wait(timeout=1.0)
                    except subprocess.TimeoutExpired:
                        self.proc.kill()
            except Exception:
                pass
            self.proc = None

        if self._master_fd is not None:
            try:
                os.close(self._master_fd)
            except Exception:
                pass
            self._master_fd = None

        self._reader_th = None


# ----------------------------
# UI helpers
# ----------------------------
def set_dark_palette(app: QApplication):
    # Simple “dark” stylesheet for a nice look without fighting palettes.
    app.setStyleSheet("""
        QWidget {
            background: #0e1118;
            color: #e6edf3;
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #2b3340;
            border-radius: 10px;
            margin-top: 12px;
            padding: 10px;
            background: #0f141c;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px 0 6px;
            color: #9ad1ff;
            font-weight: 600;
        }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit {
            background: #111827;
            border: 1px solid #273244;
            border-radius: 8px;
            padding: 6px;
            selection-background-color: #2563eb;
        }
        QTextEdit {
            font-family: "DejaVu Sans Mono", "Menlo", "Consolas", monospace;
            font-size: 12px;
        }
        QPushButton {
            background: #182233;
            border: 1px solid #2b3a55;
            border-radius: 10px;
            padding: 8px 10px;
        }
        QPushButton:hover { background: #1d2a3f; }
        QPushButton:pressed { background: #0f172a; }
        QPushButton#Primary {
            background: #2563eb;
            border: 1px solid #3b82f6;
            font-weight: 700;
        }
        QPushButton#Primary:hover { background: #1d4ed8; }
        QPushButton#Danger {
            background: #3a1920;
            border: 1px solid #7f1d1d;
        }
        QPushButton#Danger:hover { background: #4a1b24; }
        QScrollArea {
            border: none;
            background: transparent;
        }
        QFrame#Divider {
            background: #1f2a3a;
        }
        QLabel#Hint {
            color: #9aa4b2;
        }
    """)


@dataclass
class CmdWidget:
    label: str
    build_cmd: callable  # () -> str
    send_btn: QPushButton


# ----------------------------
# Main App
# ----------------------------
class App(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("UberClock Control Panel (litex_term)")
        self.resize(1200, 720)

        self.worker = LitexTermWorker()
        self._cmd_widgets: list[CmdWidget] = []

        root = QVBoxLayout(self)
        root.setContentsMargins(14, 14, 14, 14)
        root.setSpacing(10)

        # ---------- Top bar ----------
        top = QHBoxLayout()
        top.setSpacing(10)

        self.port = QComboBox()
        self.port.addItems(list_ports())
        self.kernel = QLineEdit()

        if os.path.isdir(DEFAULT_BIN_DIR):
            k = newest_bin(DEFAULT_BIN_DIR)
            if k:
                self.kernel.setText(k)

        btn_refresh = QPushButton("Refresh ports")
        btn_browse = QPushButton("Browse .bin")
        self.btn_start = QPushButton("Start")
        self.btn_start.setObjectName("Primary")

        btn_refresh.clicked.connect(self.refresh_ports)
        btn_browse.clicked.connect(self.browse_bin)
        self.btn_start.clicked.connect(self.toggle)

        top.addWidget(QLabel("Port:"))
        top.addWidget(self.port, 1)
        top.addWidget(btn_refresh)
        top.addWidget(QLabel("Kernel:"))
        top.addWidget(self.kernel, 3)
        top.addWidget(btn_browse)
        top.addWidget(self.btn_start)

        root.addLayout(top)

        hint = QLabel("Tip: run this from your LiteX venv so `litex_term` is on PATH.  " + BAUD_NOTE)
        hint.setObjectName("Hint")
        root.addWidget(hint)

        # ---------- Middle: left control panel + right log ----------
        mid = QHBoxLayout()
        mid.setSpacing(12)

        # Left panel in a scroll area
        left_scroll = QScrollArea()
        left_scroll.setWidgetResizable(True)
        left_scroll.setMinimumWidth(420)

        left_host = QWidget()
        left_layout = QVBoxLayout(left_host)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(10)

        left_scroll.setWidget(left_host)

        # Right: log
        self.log = QTextEdit()
        self.log.setReadOnly(True)

        mid.addWidget(left_scroll, 0)
        mid.addWidget(self.log, 1)
        root.addLayout(mid, 1)

        # Divider line
        div = QFrame()
        div.setObjectName("Divider")
        div.setFixedHeight(2)
        root.addWidget(div)

        # ---------- Bottom command line ----------
        bottom = QHBoxLayout()
        bottom.setSpacing(10)

        self.cmdline = QLineEdit()
        self.cmdline.setPlaceholderText("Type any command (e.g. help_uc, phase_inc_nco 2581836, gain1 0x40000000) ...")
        btn_send = QPushButton("Send")
        btn_send.setObjectName("Primary")
        btn_send.clicked.connect(self.send_cmdline)
        self.cmdline.returnPressed.connect(self.send_cmdline)

        btn_clear = QPushButton("Clear log")
        btn_clear.clicked.connect(lambda: self.log.clear())

        bottom.addWidget(self.cmdline, 1)
        bottom.addWidget(btn_send)
        bottom.addWidget(btn_clear)

        root.addLayout(bottom)

        # ---------- Build left control groups ----------
        left_layout.addWidget(self.build_group_quick())
        left_layout.addWidget(self.build_group_uberclock())
        left_layout.addWidget(self.build_group_routing())
        left_layout.addWidget(self.build_group_gains())
        left_layout.addWidget(self.build_group_debug())
        left_layout.addStretch(1)

        # ---------- Poller ----------
        self.timer = QTimer()
        self.timer.timeout.connect(self.poll_rx)
        self.timer.start(30)

        self.set_running_ui(False)

    # ----------------------------
    # Groups / widgets
    # ----------------------------
    def build_group_quick(self) -> QGroupBox:
        g = QGroupBox("Quick")
        lay = QGridLayout(g)
        lay.setHorizontalSpacing(8)
        lay.setVerticalSpacing(8)

        def quick_btn(text, cmd, col, row):
            b = QPushButton(text)
            b.clicked.connect(lambda: self.send_line(cmd))
            lay.addWidget(b, row, col)

        quick_btn("help", "help", 0, 0)
        quick_btn("help_uc", "help_uc", 1, 0)
        quick_btn("help_ddr", "help_ddr", 2, 0)
        quick_btn("ub_info", "ub_info", 0, 1)
        quick_btn("ddrinfo", "ddrinfo", 1, 1)
        quick_btn("ddrwait", "ddrwait", 2, 1)

        return g

    def build_group_uberclock(self) -> QGroupBox:
        g = QGroupBox("UberClock core")
        form = QFormLayout(g)
        form.setLabelAlignment(Qt.AlignRight)
        form.setFormAlignment(Qt.AlignTop)

        # Common defaults you mentioned in your firmware
        self.phase_inc_nco = self._add_int_cmd(
            form, "phase_inc_nco", "phase_nco", 0, 20_000_000, 2_581_836
        )
        self.nco_mag = self._add_int_cmd(
            form, "nco_mag", "nco_mag", 0, 1_000_000, 22
        )
        self.final_shift = self._add_int_cmd(
            form, "final_shift", "final_shift", 0, 63, 0
        )

        # Down phases
        self.phase_inc_down_1 = self._add_int_cmd(form, "phase_inc_down_1", "down_1", 0, 20_000_000, 2_581_110)
        self.phase_inc_down_3 = self._add_int_cmd(form, "phase_inc_down_3", "down_3", 0, 20_000_000, 80_648)
        self.phase_inc_down_4 = self._add_int_cmd(form, "phase_inc_down_4", "down_4", 0, 20_000_000, 80_644)
        self.phase_inc_down_5 = self._add_int_cmd(form, "phase_inc_down_5", "down_5", 0, 20_000_000, 80_640)

        self.phase_inc_cpu = self._add_int_cmd(form, "phase_inc_cpu", "phase_cpu", 0, 20_000_000, 52_429)

        # Snapshot / readouts
        row = QHBoxLayout()
        b1 = QPushButton("Read magnitude/phase")
        b1.clicked.connect(lambda: self.send_line("phase"))
        b2 = QPushButton("Read magnitude only")
        b2.clicked.connect(lambda: self.send_line("magnitude"))
        row.addWidget(b1)
        row.addWidget(b2)
        wrap = QWidget()
        wrap.setLayout(row)
        form.addRow(QLabel("Readouts:"), wrap)

        return g

    def build_group_routing(self) -> QGroupBox:
        g = QGroupBox("Routing / selects")
        form = QFormLayout(g)
        form.setLabelAlignment(Qt.AlignRight)

        self.input_select = self._add_int_cmd(form, "input_select", "input_select", 0, 32, 0)
        self.upsampler_input_mux = self._add_int_cmd(form, "upsampler_input_mux", "upsampler_input_mux", 0, 32, 0)

        self.output_select_ch1 = self._add_int_cmd(form, "output_select_ch1", "output_select_ch1", 0, 64, 10)
        self.output_select_ch2 = self._add_int_cmd(form, "output_select_ch2", "output_select_ch2", 0, 64, 11)

        return g

    def build_group_gains(self) -> QGroupBox:
        g = QGroupBox("Gains")
        grid = QGridLayout(g)
        grid.setHorizontalSpacing(8)
        grid.setVerticalSpacing(8)

        # Gains as hex (common in LiteX CSR tooling)
        # We'll provide an input box that accepts 0x... or decimal.
        self.gain_boxes = []
        for i in range(1, 6):
            lbl = QLabel(f"gain{i}")
            box = QLineEdit("0x40000000")
            box.setPlaceholderText("hex or decimal")
            btn = QPushButton("Send")
            btn.clicked.connect(lambda _, ch=i, w=box: self.send_gain(ch, w))
            grid.addWidget(lbl, i-1, 0)
            grid.addWidget(box, i-1, 1)
            grid.addWidget(btn, i-1, 2)
            self.gain_boxes.append(box)

        return g

    def build_group_debug(self) -> QGroupBox:
        g = QGroupBox("Debug / convenience")
        lay = QVBoxLayout(g)
        lay.setSpacing(8)

        b_ping = QPushButton("Send newline (wake console)")
        b_ping.clicked.connect(lambda: self.send_line(""))
        lay.addWidget(b_ping)

        b_abort = QPushButton("Send 'Q' (abort serial boot)")
        b_abort.setObjectName("Danger")
        b_abort.clicked.connect(lambda: self.send_line("Q"))
        lay.addWidget(b_abort)

        return g

    def _add_int_cmd(self, form: QFormLayout, label: str, cmd: str, mn: int, mx: int, default: int):
        row = QHBoxLayout()
        sb = QSpinBox()
        sb.setRange(mn, mx)
        sb.setValue(default)
        sb.setSingleStep(1)
        sb.setMinimumWidth(160)

        btn = QPushButton("Send")
        btn.clicked.connect(lambda: self.send_line(f"{cmd} {sb.value()}"))

        row.addWidget(sb)
        row.addWidget(btn)

        wrap = QWidget()
        wrap.setLayout(row)
        form.addRow(QLabel(label + ":"), wrap)
        return sb

    # ----------------------------
    # Actions
    # ----------------------------
    def append_log(self, s: str):
        self.log.insertPlainText(s)
        self.log.ensureCursorVisible()

    def refresh_ports(self):
        cur = self.port.currentText()
        self.port.clear()
        self.port.addItems(list_ports())
        idx = self.port.findText(cur)
        if idx >= 0:
            self.port.setCurrentIndex(idx)

    def browse_bin(self):
        start_dir = DEFAULT_BIN_DIR if os.path.isdir(DEFAULT_BIN_DIR) else os.getcwd()
        p, _ = QFileDialog.getOpenFileName(self, "Select .bin", start_dir, "BIN (*.bin)")
        if p:
            self.kernel.setText(p)

    def set_running_ui(self, running: bool):
        self.btn_start.setText("Stop" if running else "Start")
        self.btn_start.setObjectName("Danger" if running else "Primary")
        # refresh style for objectName changes
        self.btn_start.style().unpolish(self.btn_start)
        self.btn_start.style().polish(self.btn_start)

    def toggle(self):
        # Stop if running
        if self.worker.proc and self.worker.proc.poll() is None:
            self.worker.stop()
            self.set_running_ui(False)
            self.append_log("\n[stopped]\n")
            return

        port = self.port.currentText().strip()
        kernel = self.kernel.text().strip()

        if not port:
            QMessageBox.critical(self, "Error", "Select a serial port.")
            return
        if not kernel or not os.path.isfile(kernel):
            QMessageBox.critical(self, "Error", "Select a valid .bin kernel.")
            return

        litex_term = shutil.which("litex_term")
        if not litex_term:
            QMessageBox.critical(
                self, "Error",
                "litex_term not found in PATH.\n\n"
                "Run this GUI from your LiteX venv shell, e.g.:\n"
                "  source ~/FPGA/Tools/litex-hub/litex/litex-venv/bin/activate\n"
                "  python3 uberclock_gui.py"
            )
            return

        cmd = [litex_term, port, f"--kernel={kernel}"]
        env = os.environ.copy()

        self.append_log("[starting litex_term]\n")
        self.append_log(" ".join(cmd) + "\n\n")

        try:
            self.worker.start(cmd, env=env)
        except Exception as e:
            self.append_log(f"[start error] {e}\n")
            return

        self.set_running_ui(True)
        self.append_log("[litex_term started]\n")

    def send_line(self, line: str):
        # still log even if empty line
        if line.strip():
            self.append_log(f"> {line.strip()}\n")
        else:
            self.append_log("> \n")
        try:
            self.worker.send(line)
        except Exception as e:
            self.append_log(f"[send error] {e}\n")

    def send_cmdline(self):
        s = self.cmdline.text().strip()
        if not s:
            return
        self.send_line(s)
        self.cmdline.clear()

    def send_gain(self, ch: int, box: QLineEdit):
        raw = box.text().strip()
        if not raw:
            return
        try:
            # allow 0x..., 0b..., or decimal
            val = int(raw, 0)
        except ValueError:
            self.append_log(f"[gain{ch}] invalid number: {raw}\n")
            return
        self.send_line(f"gain{ch} {val}")

    def poll_rx(self):
        while not self.worker.rxq.empty():
            self.append_log(self.worker.rxq.get())

        if self.worker.proc and self.worker.proc.poll() is not None:
            self.set_running_ui(False)

    def closeEvent(self, event):
        self.worker.stop()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    set_dark_palette(app)
    w = App()
    w.show()
    sys.exit(app.exec())
