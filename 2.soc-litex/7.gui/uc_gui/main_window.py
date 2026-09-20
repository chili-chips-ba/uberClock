"""Main application window: wires litex_link, the ANSI log view, and the
auto-generated command panel together into a single runnable app.

Kept deliberately thin - it owns widget layout and the QTimer poll loop,
and delegates everything else to the modules that already have their own
tests (`litex_link`, `cmd_catalog`, `command_panel`, `log_view`).
"""

from __future__ import annotations

import os
from pathlib import Path

from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDockWidget,
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QSpinBox,
    QSplitter,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from .ansi_text import AnsiStripper
from .cmd_catalog import CmdListParseError, parse_cmdlist
from .command_meta import ENUM_OVERRIDES
from .command_panel import CommandPanel
from .hs_capture import HsCaptureController
from .hs_udp_receiver import HsUdpReceiver
from .litex_link import (
    GatewareReloadWorker,
    LitexTermWorker,
    build_command,
    build_reload_gateware_command,
    find_newest_bin,
    list_serial_ports,
)
from .log_view import AnsiLogView
from .ls_capture import LsCaptureController
from .plot_panel import PlotPanel

LS_POLL_INTERVAL_MS = 300

# 2.soc-litex/7.gui/uc_gui/main_window.py -> parents[2] == 2.soc-litex
PROJECT_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SW_DIR = PROJECT_ROOT / "2.sw"

CMDLIST_BEGIN = "--CMDLIST-BEGIN--"
CMDLIST_END = "--CMDLIST-END--"

# Must be kept in sync with FREQ/OPTIONS defaults in
# 2.soc-litex/3.build/Makefile's `load` target.
DEFAULT_SYS_CLK_FREQ = "100e6"
DEFAULT_BUILD_OPTIONS = ("--with-uberddr3", "--with-uberclock", "--with-ethernet")

# The LS debug capture RAM samples at a fixed 10 kHz tick.
LS_SAMPLE_RATE_HZ = 10_000.0

# The "uc" clock domain HS debug samples are captured on - see UC_CLK_HZ
# in 8.python/src/uberclock_soc/clocking.py (a separate PLL output from
# sys_clk, not the same 100 MHz used to build the gateware).
HS_SAMPLE_RATE_HZ = 65_000_000.0

# 128 beats * 16 samples/beat = 2048 samples - matches LS capture's fixed
# length, so the two are visually comparable by default.
HS_DEFAULT_BEATS = 128

# This PC's address on the FPGA's Ethernet link (enp8s0, same /24 as the
# board's static 192.168.0.123 - see UBD3_BOARD_IP in uberclock.c). Only
# valid on this machine/link; re-check with `ip addr show` if the GUI
# ever runs from a different host or network.
DEFAULT_HS_HOST_IP = "192.168.0.5"
DEFAULT_HS_PORT = 5000


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("uberClock Control Panel")
        self.resize(1200, 720)

        self.worker = LitexTermWorker()
        self.reload_worker = GatewareReloadWorker()
        self._cmdlist_buffer: str | None = None  # None => not currently discovering
        self._ansi_stripper = AnsiStripper()  # feeds clean text to the capture controllers
        self.last_ls_capture = None  # most recent numpy array from ls_capture, if any
        self.last_hs_capture = None  # most recent numpy array from hs_capture, if any

        self.log = AnsiLogView()

        self._build_top_toolbar()
        self._build_bottom_bar()
        self._build_central_widget()

        self.ls_capture = LsCaptureController(
            send_command=self._send_command,
            on_samples=self._on_ls_samples,
            on_status=self._on_capture_status,
        )
        self.hs_capture = HsCaptureController(
            send_command=self._send_command,
            on_samples=self._on_hs_samples,
            on_status=self._on_capture_status,
        )
        self.hs_receiver = HsUdpReceiver()

        self.timer = QTimer(self)
        self.timer.timeout.connect(self._poll_worker)
        self.timer.start(30)

        # cap_status has to be polled (the firmware doesn't push a
        # completion notice) and the HS UDP stream/timeout need polling
        # too - a no-op tick while both are idle costs nothing.
        self.capture_poll_timer = QTimer(self)
        self.capture_poll_timer.timeout.connect(self._poll_captures)
        self.capture_poll_timer.start(LS_POLL_INTERVAL_MS)

        self._set_connected_ui(False)
        self._set_input_enabled(False)

    # ---------------------------------------------------------------- UI
    def _build_top_toolbar(self) -> None:
        bar = QWidget()
        layout = QHBoxLayout(bar)

        self.port_box = QComboBox()
        self.port_box.addItems(list_serial_ports())

        self.kernel_edit = QLineEdit()
        newest = find_newest_bin(DEFAULT_SW_DIR) if DEFAULT_SW_DIR.is_dir() else None
        if newest:
            self.kernel_edit.setText(newest)

        refresh_btn = QPushButton("Refresh ports")
        refresh_btn.clicked.connect(self._refresh_ports)

        browse_btn = QPushButton("Browse .bin")
        browse_btn.clicked.connect(self._browse_kernel)

        self.connect_btn = QPushButton("Start")
        self.connect_btn.clicked.connect(self._toggle_connection)

        self.reboot_btn = QPushButton("Soft reboot")
        self.reboot_btn.setToolTip(
            "Sends the BIOS 'reboot' command (soft CPU reset only). This "
            "does NOT force a fresh kernel upload - it just re-enters "
            "whatever program is already resident. To load a new kernel "
            "image, reprogram the FPGA (`make load`) first."
        )
        self.reboot_btn.clicked.connect(self._request_reboot)

        self.discover_btn = QPushButton("Discover commands")
        self.discover_btn.setToolTip("Runs `cmdlist` and rebuilds the command panel from the reply.")
        self.discover_btn.clicked.connect(self._discover_commands)

        self.reload_btn = QPushButton("Reload gateware")
        self.reload_btn.setToolTip(
            "Reprograms the FPGA over JTAG (same as `make load`) - the "
            "only thing that reliably forces a fresh kernel upload. "
            "Click Start first so litex_term is already attached and can "
            "catch the resulting reboot."
        )
        self.reload_btn.clicked.connect(self._reload_gateware)

        for widget in (
            QLabel("Port:"), self.port_box, refresh_btn,
            QLabel("Kernel:"),
        ):
            layout.addWidget(widget)
        layout.addWidget(self.kernel_edit, 1)
        layout.addWidget(browse_btn)
        layout.addWidget(self.connect_btn)
        layout.addWidget(self.reboot_btn)
        layout.addWidget(self.discover_btn)
        layout.addWidget(self.reload_btn)

        toolbar_dock = QDockWidget("Connection", self)
        toolbar_dock.setWidget(bar)
        toolbar_dock.setFeatures(QDockWidget.DockWidgetFeature.NoDockWidgetFeatures)
        self.addDockWidget(Qt.DockWidgetArea.TopDockWidgetArea, toolbar_dock)

    def _build_bottom_bar(self) -> None:
        bar = QWidget()
        layout = QHBoxLayout(bar)

        self.command_edit = QLineEdit()
        self.command_edit.setPlaceholderText("Type any command, e.g. help_uc, gain1 0x40000000 ...")
        self.command_edit.returnPressed.connect(self._send_typed_command)

        self.send_btn = QPushButton("Send")
        self.send_btn.clicked.connect(self._send_typed_command)

        layout.addWidget(self.command_edit, 1)
        layout.addWidget(self.send_btn)

        dock = QDockWidget("Command line", self)
        dock.setWidget(bar)
        dock.setFeatures(QDockWidget.DockWidgetFeature.NoDockWidgetFeatures)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, dock)

    def _build_central_widget(self) -> None:
        """Log on the left (always visible - it's what catches every bug
        we've hit so far), a tabbed Commands/Capture panel on the right
        (the two don't need to be visible at the same time)."""
        tabs = QTabWidget()
        tabs.addTab(self._build_commands_tab(), "Commands")
        tabs.addTab(self._build_capture_tab(), "Capture")

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(self.log)
        splitter.addWidget(tabs)
        splitter.setSizes([700, 500])
        self.setCentralWidget(splitter)

    def _build_commands_tab(self) -> QWidget:
        container = QWidget()
        self._commands_layout = QVBoxLayout(container)
        self._commands_container = container
        self._commands_layout.addWidget(QLabel("Connect, then click 'Discover commands'."))
        return container

    def _build_capture_tab(self) -> QWidget:
        """A slim control bar on top, the plot filling everything below
        it - the plot used to share width with a tall side column of
        controls and lost most of its room to it."""
        tab = QWidget()
        layout = QVBoxLayout(tab)

        self.ls_controls = QWidget()
        controls_layout = QHBoxLayout(self.ls_controls)
        controls_layout.setContentsMargins(0, 0, 0, 0)

        controls_layout.addWidget(QLabel("LS channel:"))
        self.ls_channel_box = QComboBox()
        for value, label in ENUM_OVERRIDES["lowspeed_dbg_select"]:
            self.ls_channel_box.addItem(f"{label} ({value})", userData=value)
        controls_layout.addWidget(self.ls_channel_box)

        self.ls_capture_btn = QPushButton("Capture LS")
        self.ls_capture_btn.clicked.connect(self._start_ls_capture)
        controls_layout.addWidget(self.ls_capture_btn)
        layout.addWidget(self.ls_controls)

        self.hs_controls = QWidget()
        hs_layout = QHBoxLayout(self.hs_controls)
        hs_layout.setContentsMargins(0, 0, 0, 0)

        hs_layout.addWidget(QLabel("HS channel:"))
        self.hs_channel_box = QComboBox()
        for value, label in ENUM_OVERRIDES["highspeed_dbg_select"]:
            self.hs_channel_box.addItem(f"{label} ({value})", userData=value)
        hs_layout.addWidget(self.hs_channel_box)

        hs_layout.addWidget(QLabel("Beats:"))
        self.hs_beats_box = QSpinBox()
        self.hs_beats_box.setRange(1, 1_000_000)
        self.hs_beats_box.setValue(HS_DEFAULT_BEATS)
        self.hs_beats_box.setToolTip(
            "Each beat is 16 samples (one 256-bit DDR write). The default "
            f"({HS_DEFAULT_BEATS}) gives {HS_DEFAULT_BEATS * 16} samples, "
            "matching LS capture's fixed length."
        )
        hs_layout.addWidget(self.hs_beats_box)

        hs_layout.addWidget(QLabel("Host IP:"))
        self.hs_ip_edit = QLineEdit(DEFAULT_HS_HOST_IP)
        self.hs_ip_edit.setToolTip(
            "This PC's IP address on the FPGA's Ethernet link - the board "
            "streams the capture back to this address over UDP."
        )
        hs_layout.addWidget(self.hs_ip_edit)

        hs_layout.addWidget(QLabel("Port:"))
        self.hs_port_box = QSpinBox()
        self.hs_port_box.setRange(1, 65535)
        self.hs_port_box.setValue(DEFAULT_HS_PORT)
        hs_layout.addWidget(self.hs_port_box)

        self.hs_ramp_check = QCheckBox("Ramp test pattern")
        self.hs_ramp_check.setToolTip(
            "Bypass the selected HS channel and capture the firmware's "
            "internal counting test pattern (ub_ramp) instead - a way to "
            "check the DMA/UDP/plot pipeline on its own, independent of "
            "whatever the HS channel mux is producing."
        )
        hs_layout.addWidget(self.hs_ramp_check)

        self.hs_capture_btn = QPushButton("Capture HS")
        self.hs_capture_btn.clicked.connect(self._start_hs_capture)
        hs_layout.addWidget(self.hs_capture_btn)
        layout.addWidget(self.hs_controls)

        self.capture_status_label = QLabel("Idle.")
        self.capture_status_label.setWordWrap(True)
        layout.addWidget(self.capture_status_label)

        self.plot_panel = PlotPanel(sample_rate=LS_SAMPLE_RATE_HZ)
        layout.addWidget(self.plot_panel, 1)
        return tab

    def _set_connected_ui(self, connected: bool) -> None:
        self.connect_btn.setText("Stop" if connected else "Start")

    def _set_input_enabled(self, enabled: bool) -> None:
        """Enable/disable everything that can send a command."""
        self.command_edit.setEnabled(enabled)
        self.send_btn.setEnabled(enabled)
        self.reboot_btn.setEnabled(enabled)
        self.discover_btn.setEnabled(enabled)
        self._commands_container.setEnabled(enabled)
        self.ls_controls.setEnabled(enabled)
        self.hs_controls.setEnabled(enabled)

    # ------------------------------------------------------------ actions
    def _refresh_ports(self) -> None:
        current = self.port_box.currentText()
        self.port_box.clear()
        self.port_box.addItems(list_serial_ports())
        index = self.port_box.findText(current)
        if index >= 0:
            self.port_box.setCurrentIndex(index)

    def _browse_kernel(self) -> None:
        start_dir = str(DEFAULT_SW_DIR) if DEFAULT_SW_DIR.is_dir() else os.getcwd()
        path, _ = QFileDialog.getOpenFileName(self, "Select kernel .bin", start_dir, "BIN (*.bin)")
        if path:
            self.kernel_edit.setText(path)

    def _toggle_connection(self) -> None:
        if self.worker.is_running:
            self.worker.stop()
            self._set_connected_ui(False)
            self._set_input_enabled(False)
            self.log.append_ansi("\n[stopped]\n")
            return

        port = self.port_box.currentText().strip()
        kernel = self.kernel_edit.text().strip()
        if not port:
            QMessageBox.critical(self, "Error", "Select a serial port.")
            return
        if not kernel or not os.path.isfile(kernel):
            QMessageBox.critical(self, "Error", "Select a valid kernel .bin file.")
            return

        try:
            command = build_command(port, kernel)
        except FileNotFoundError as exc:
            QMessageBox.critical(self, "Error", str(exc))
            return

        self.log.append_ansi(f"[starting] {' '.join(command)}\n")
        self.worker.start(command, env=os.environ.copy())
        self._set_connected_ui(True)
        self._set_input_enabled(True)
        self.log.append_ansi(
            "[note: litex_term may prepend a stray control byte to the very "
            "first command right after Start - harmless, just resend it if "
            "the firmware reports 'Unknown command']\n"
        )

    def _request_reboot(self) -> None:
        try:
            self.worker.request_reboot()
            self.log.append_ansi("> reboot\n")
        except RuntimeError as exc:
            self.log.append_ansi(f"[error] {exc}\n")

    def _send_typed_command(self) -> None:
        line = self.command_edit.text().strip()
        if not line:
            return
        self._send_command(line)
        self.command_edit.clear()

    def _send_command(self, line: str) -> None:
        """Shared by the command line and every auto-generated command
        panel button/spinbox/dropdown."""
        self.log.append_ansi(f"> {line}\n")
        try:
            self.worker.send_line(line)
        except RuntimeError as exc:
            self.log.append_ansi(f"[error] {exc}\n")

    def _discover_commands(self) -> None:
        if not self.worker.is_running:
            QMessageBox.warning(self, "Not connected", "Start a session first.")
            return
        self._cmdlist_buffer = ""
        self._send_command("cmdlist")

    def _reload_gateware(self) -> None:
        if self.reload_worker.is_running:
            return
        if not self.worker.is_running:
            proceed = QMessageBox.question(
                self, "litex_term not running",
                "litex_term isn't attached, so it won't catch the reboot "
                "this causes and won't auto-upload the kernel. Click "
                "Start first, then reload. Proceed anyway?",
            )
            if proceed != QMessageBox.StandardButton.Yes:
                return

        confirmed = QMessageBox.question(
            self, "Reprogram FPGA",
            "This reprograms the FPGA over JTAG (same as `make load`), "
            "using the already-built bitstream. Continue?",
        )
        if confirmed != QMessageBox.StandardButton.Yes:
            return

        try:
            argv, env, cwd = build_reload_gateware_command(
                PROJECT_ROOT, DEFAULT_SYS_CLK_FREQ, DEFAULT_BUILD_OPTIONS
            )
        except FileNotFoundError as exc:
            QMessageBox.critical(self, "Error", str(exc))
            return

        self.log.append_ansi(f"[reloading gateware, cwd={cwd}] {' '.join(argv)}\n")
        self.reload_worker.start(argv, env=env, cwd=cwd)
        self.reload_btn.setEnabled(False)

    def _start_ls_capture(self) -> None:
        if not self.worker.is_running:
            QMessageBox.warning(self, "Not connected", "Start a session first.")
            return
        if self.ls_capture.is_busy:
            return
        self.ls_capture_btn.setEnabled(False)
        self.ls_capture.start(self.ls_channel_box.currentData())

    def _on_capture_status(self, message: str) -> None:
        self.capture_status_label.setText(message)

    def _on_ls_samples(self, samples) -> None:
        self.last_ls_capture = samples
        self.ls_capture_btn.setEnabled(True)
        self.plot_panel.set_samples(samples, sample_rate=LS_SAMPLE_RATE_HZ, source_label="LS")
        self.log.append_ansi(
            f"[ls capture] {len(samples)} samples, "
            f"min={samples.min()} max={samples.max()} mean={samples.mean():.1f}\n"
        )

    def _start_hs_capture(self) -> None:
        if not self.worker.is_running:
            QMessageBox.warning(self, "Not connected", "Start a session first.")
            return
        if self.hs_capture.is_busy:
            return
        dst_ip = self.hs_ip_edit.text().strip()
        if not dst_ip:
            QMessageBox.critical(self, "Error", "Enter this PC's IP on the FPGA's network link.")
            return
        dst_port = self.hs_port_box.value()

        self.hs_capture_btn.setEnabled(False)
        self.hs_receiver.start(dst_port)
        self.hs_capture.start(
            channel=self.hs_channel_box.currentData(),
            beats=self.hs_beats_box.value(),
            dst_ip=dst_ip,
            dst_port=dst_port,
            use_ramp=self.hs_ramp_check.isChecked(),
        )

    def _on_hs_samples(self, samples) -> None:
        self.last_hs_capture = samples
        self.plot_panel.set_samples(samples, sample_rate=HS_SAMPLE_RATE_HZ, source_label="HS")
        self.log.append_ansi(
            f"[hs capture] {len(samples)} samples, "
            f"min={samples.min()} max={samples.max()} mean={samples.mean():.1f}\n"
        )

    # -------------------------------------------------------------- poll
    def _poll_worker(self) -> None:
        chunks = self.worker.poll()
        for chunk in chunks:
            self.log.append_ansi(chunk)
            clean = self._ansi_stripper.feed(chunk)
            self.ls_capture.feed_text(clean)
            self.hs_capture.feed_text(clean)
            if self._cmdlist_buffer is not None:
                self._cmdlist_buffer += chunk
                self._try_finish_discovery()

        if not self.worker.is_running:
            self._set_connected_ui(False)

        for chunk in self.reload_worker.poll():
            self.log.append_ansi(chunk)
        if not self.reload_worker.is_running:
            self.reload_btn.setEnabled(True)

    def _poll_captures(self) -> None:
        self.ls_capture.poll_status()

        for packet in self.hs_receiver.poll():
            self.hs_capture.feed_packet(packet)
        self.hs_capture.check_timeout()
        if not self.hs_capture.is_busy:
            # Covers both success and failure (dropped packet, no ARP
            # reply, DMA error, ...) - the button shouldn't stay disabled
            # forever just because the last attempt didn't work out.
            self.hs_capture_btn.setEnabled(True)
            if self.hs_receiver.is_listening:
                self.hs_receiver.stop()

    def _try_finish_discovery(self) -> None:
        assert self._cmdlist_buffer is not None
        if CMDLIST_BEGIN not in self._cmdlist_buffer or CMDLIST_END not in self._cmdlist_buffer:
            return  # keep collecting - reply hasn't fully arrived yet
        try:
            commands = parse_cmdlist(self._cmdlist_buffer)
        except CmdListParseError:
            return  # markers present but malformed so far; keep waiting
        finally:
            self._cmdlist_buffer = None

        panel = CommandPanel(commands, send_command=self._send_command)
        while self._commands_layout.count():
            old = self._commands_layout.takeAt(0).widget()
            if old:
                old.deleteLater()
        self._commands_layout.addWidget(panel)
        self.log.append_ansi(f"\n[discovered {len(commands)} commands]\n")

    def closeEvent(self, event) -> None:
        self.worker.stop()
        self.reload_worker.stop()
        self.hs_receiver.stop()
        super().closeEvent(event)
