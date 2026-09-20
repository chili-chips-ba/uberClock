"""Run `litex_term` as a managed subprocess and exchange text with it.

`litex_term` already implements the LiteX BIOS's serial boot protocol
(the SFL handshake used to auto-upload a kernel image), so instead of
re-implementing that over a raw serial port, this module spawns the
real `litex_term` binary inside a pseudo-terminal (PTY) and treats it
as a plain bidirectional text stream::

    PTY master (this process) <--> PTY slave <--> litex_term <--> board

`litex_term` thinks it's talking to a real terminal (a PTY looks like
one from the child process's side), while we get raw read/write access
to its stdin/stdout as byte streams we can parse and inject into.

Lifted and cleaned up from the original prototype in
`2.soc-litex/7.gui/uberclock_gui.py`.
"""

from __future__ import annotations

import glob
import os
import pty
import queue
import select
import shutil
import subprocess
import threading
from pathlib import Path

import serial.tools.list_ports


def list_serial_ports() -> list[str]:
    """Device paths of currently available serial ports (e.g. /dev/ttyUSB1)."""
    return [port.device for port in serial.tools.list_ports.comports()]


def find_newest_bin(directory: str | Path) -> str | None:
    """Path to the most recently modified ``*.bin`` in `directory`, or
    None if there isn't one."""
    candidates = glob.glob(os.path.join(directory, "*.bin"))
    if not candidates:
        return None
    return max(candidates, key=os.path.getmtime)


def find_litex_term() -> str | None:
    """Path to the ``litex_term`` executable, or None if not on PATH."""
    return shutil.which("litex_term")


def build_command(port: str, kernel_path: str) -> list[str]:
    """Command line to boot `kernel_path` over `port` via litex_term.

    Raises FileNotFoundError if litex_term isn't on PATH, e.g. because
    the LiteX venv hasn't been activated.
    """
    litex_term = find_litex_term()
    if litex_term is None:
        raise FileNotFoundError(
            "litex_term not found on PATH - activate the LiteX venv "
            "before starting this GUI (e.g. `source ~/.venv/litex/bin/activate`)."
        )
    return [litex_term, port, f"--kernel={kernel_path}"]


def build_reload_gateware_command(
    project_root: str | Path,
    sys_clk_freq: str = "100e6",
    options: tuple[str, ...] = ("--with-uberddr3", "--with-uberclock", "--with-ethernet"),
) -> tuple[list[str], dict[str, str], str]:
    """Command + environment + working directory to reprogram the FPGA
    with the already-built bitstream - mirrors the ``load`` target in
    `2.soc-litex/3.build/Makefile` exactly (same target script, same
    working directory, same --sys-clk-freq/OPTIONS defaults), so it
    needs the same care: this is a real JTAG reconfiguration, not a
    lightweight reset.

    The working directory matters: the target script resolves its build
    output (``build/<board>/gateware/*.bit``) relative to the process's
    CWD, not to the script's own location. `make load` always runs from
    `3.build/` (where the Makefile lives), which is where `make
    build-board` actually put the bitstream - running this script from
    anywhere else makes it look for a `build/` directory that doesn't
    exist there and fail with "Open file FAIL".

    Uses the same venv `litex_term` was found in (its sibling
    `python3`), rather than a second hardcoded venv path, so there's
    only one place that needs the LiteX venv activated.

    Returns (argv, env, cwd) instead of running anything - the caller
    decides when to actually launch it via `GatewareReloadWorker`.
    """
    litex_term = find_litex_term()
    if litex_term is None:
        raise FileNotFoundError(
            "litex_term not found on PATH - activate the LiteX venv first."
        )
    venv_python = Path(litex_term).parent / "python3"

    project_root = Path(project_root)
    target_py = project_root / "8.python" / "src" / "targets" / "alinx_ax7203_uberclock.py"
    pythonpath_repo = project_root / "8.python" / "src"
    build_cwd = project_root / "3.build"
    if not target_py.is_file():
        raise FileNotFoundError(f"target script not found: {target_py}")
    if not build_cwd.is_dir():
        raise FileNotFoundError(f"build directory not found: {build_cwd}")

    argv = [str(venv_python), str(target_py), "--load", f"--sys-clk-freq={sys_clk_freq}", *options]
    env = os.environ.copy()
    env["PYTHONPATH"] = str(pythonpath_repo)
    return argv, env, str(build_cwd)


class GatewareReloadWorker:
    """Runs a one-shot subprocess (an FPGA reprogram) and streams its
    combined stdout/stderr without blocking the GUI thread.

    Simpler than `LitexTermWorker`: no PTY and no further input once
    started - this command runs to completion and exits on its own.
    """

    def __init__(self) -> None:
        self._process: subprocess.Popen | None = None
        self._reader_thread: threading.Thread | None = None
        self.received: queue.Queue[str] = queue.Queue()
        self.exit_code: int | None = None

    @property
    def is_running(self) -> bool:
        return self._process is not None and self._process.poll() is None

    def start(self, command: list[str], env: dict | None = None, cwd: str | None = None) -> None:
        if self.is_running:
            raise RuntimeError("a gateware reload is already running")
        self.exit_code = None
        self._process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env,
            cwd=cwd,
            text=True,
            bufsize=1,
        )
        self._reader_thread = threading.Thread(target=self._read_loop, daemon=True)
        self._reader_thread.start()

    def poll(self) -> list[str]:
        """Drain and return all text chunks received since the last call."""
        chunks = []
        while not self.received.empty():
            chunks.append(self.received.get_nowait())
        return chunks

    def stop(self) -> None:
        if self._process is not None and self._process.poll() is None:
            self._process.terminate()
            try:
                self._process.wait(timeout=1.0)
            except subprocess.TimeoutExpired:
                self._process.kill()

    def _read_loop(self) -> None:
        assert self._process is not None and self._process.stdout is not None
        with self._process.stdout:
            for line in self._process.stdout:
                self.received.put(line)
        self.exit_code = self._process.wait()
        self.received.put(f"\n[gateware reload finished, exit code={self.exit_code}]\n")


class LitexTermWorker:
    """Owns one litex_term subprocess running inside a PTY.

    Incoming bytes are decoded and pushed onto a thread-safe
    ``queue.Queue`` by a background reader thread. The GUI drains that
    queue itself on a timer tick (see `poll()`), which keeps all widget
    updates on the Qt event loop thread as Qt's threading rules require
    (https://doc.qt.io/qtforpython-6/overviews/thread-basics.html),
    while this class stays Qt-free and independently testable - see
    `tests/test_litex_link.py`, which exercises it against `cat` as a
    stand-in child process instead of real hardware.
    """

    def __init__(self) -> None:
        self._process: subprocess.Popen | None = None
        self._master_fd: int | None = None
        self._reader_thread: threading.Thread | None = None
        self.received: queue.Queue[str] = queue.Queue()

    @property
    def is_running(self) -> bool:
        return self._process is not None and self._process.poll() is None

    def start(self, command: list[str], env: dict | None = None) -> None:
        """Spawn `command` (typically from `build_command()`) inside a PTY."""
        self.stop()

        master_fd, slave_fd = pty.openpty()
        self._master_fd = master_fd
        self._process = subprocess.Popen(
            command,
            stdin=slave_fd,
            stdout=slave_fd,
            stderr=slave_fd,
            env=env,
            close_fds=True,
            text=False,  # serial data isn't guaranteed valid UTF-8 mid-stream
        )
        os.close(slave_fd)  # only the child needs the slave end

        self._reader_thread = threading.Thread(target=self._read_loop, daemon=True)
        self._reader_thread.start()

    def send_line(self, line: str) -> None:
        """Send one line of text, as if typed at the litex_term prompt."""
        if not self.is_running or self._master_fd is None:
            raise RuntimeError("litex_term is not running")
        os.write(self._master_fd, (line.rstrip("\r\n") + "\n").encode())

    def request_reboot(self) -> None:
        """Soft-reset the CPU via the BIOS's own ``reboot`` command.

        This does NOT reliably force litex_term to re-upload a kernel -
        tested against real hardware and found to just re-enter whatever
        program is already resident, rather than dropping back to the
        BIOS's serial-boot-wait state. Getting a fresh kernel uploaded
        still requires a real FPGA reprogram (`make load` in
        `2.soc-litex/3.build/Makefile`, which resets everything via
        JTAG). Useful only as a generic "soft-reset the CPU" action.
        """
        self.send_line("reboot")

    def poll(self) -> list[str]:
        """Drain and return all text chunks received since the last call."""
        chunks = []
        while not self.received.empty():
            chunks.append(self.received.get_nowait())
        return chunks

    def stop(self) -> None:
        if self._process is not None:
            if self._process.poll() is None:
                self._process.terminate()
                try:
                    self._process.wait(timeout=1.0)
                except subprocess.TimeoutExpired:
                    self._process.kill()
            self._process = None

        if self._master_fd is not None:
            try:
                os.close(self._master_fd)
            except OSError:
                pass
            self._master_fd = None

        self._reader_thread = None

    def _read_loop(self) -> None:
        try:
            while self.is_running and self._master_fd is not None:
                ready, _, _ = select.select([self._master_fd], [], [], 0.1)
                if not ready:
                    continue
                data = os.read(self._master_fd, 4096)
                if not data:
                    break
                self.received.put(data.decode(errors="replace"))
        except OSError:
            pass  # master fd was closed from stop() while we were reading

        if self._process is not None:
            exit_code = self._process.poll()
            if exit_code is not None:
                self.received.put(f"\n[litex_term exited, code={exit_code}]\n")
