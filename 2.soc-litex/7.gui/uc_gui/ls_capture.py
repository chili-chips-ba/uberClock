"""Drive the low-speed on-chip capture RAM behind one button.

Without this, capturing a channel means remembering and hand-typing
three commands in order - `cap_start`, then polling `cap_status` until
it says DONE, then `cap_dump` and reading 2048 CSV lines back off the
console (see `cap_start_cmd`/`cap_status_cmd`/`cap_dump_cmd` in
`2.soc-litex/2.sw/uberclock.c`). This wraps that sequence into a small
state machine: call `start(channel)` once, keep calling `poll_status()`
on a timer while `is_busy`, and get a finished numpy array back.
"""

from __future__ import annotations

import re
from typing import Callable

import numpy as np

SendCommand = Callable[[str], None]
StatusCallback = Callable[[str], None]
SamplesCallback = Callable[[np.ndarray], None]

_CAPTURE_LEN = 2048
_STATUS_DONE_RE = re.compile(r"^Capture DONE$")
_DUMP_LINE_RE = re.compile(r"^(\d+),(-?\d+)$")


class LsCaptureController:
    """State machine: select channel -> cap_start -> poll cap_status ->
    cap_dump -> parsed samples.

    Feed it plain, ANSI-stripped text as it arrives (see
    `uc_gui.ansi_text.AnsiStripper`) via `feed_text()`.
    """

    def __init__(
        self,
        send_command: SendCommand,
        on_samples: SamplesCallback,
        on_status: StatusCallback = lambda message: None,
    ) -> None:
        self._send_command = send_command
        self._on_samples = on_samples
        self._on_status = on_status
        self._state = "idle"  # idle -> armed -> waiting_dump -> idle
        self._samples: list[int] = []
        self._line_buffer = ""  # holds a trailing incomplete line across feed_text() calls

    @property
    def is_busy(self) -> bool:
        return self._state != "idle"

    def start(self, channel: int) -> None:
        if self.is_busy:
            raise RuntimeError("a capture is already in progress")
        self._state = "armed"
        self._samples = []
        self._line_buffer = ""
        self._send_command(f"lowspeed_dbg_select {channel}")
        self._send_command("cap_start")
        self._on_status("Capture started, waiting for completion...")

    def poll_status(self) -> None:
        """Call periodically (e.g. from a QTimer) while `is_busy` - the
        firmware doesn't push a completion notice, so this has to ask."""
        if self._state == "armed":
            self._send_command("cap_status")

    def feed_text(self, text: str) -> None:
        """Feed a chunk of plain text. Chunks can end mid-line (a real
        UART read has no notion of line boundaries), so only complete
        lines - terminated by '\\n' - are processed; any trailing partial
        line is held in `_line_buffer` and completed by a later call."""
        self._line_buffer += text
        while "\n" in self._line_buffer:
            line, self._line_buffer = self._line_buffer.split("\n", 1)
            self._feed_line(line.strip())

    def _feed_line(self, line: str) -> None:
        if self._state == "armed" and _STATUS_DONE_RE.match(line):
            self._state = "waiting_dump"
            self._on_status("Capture done, downloading 2048 samples...")
            self._send_command("cap_dump")
            return

        if self._state == "waiting_dump":
            match = _DUMP_LINE_RE.match(line)
            if match:
                self._samples.append(int(match.group(2)))
                if len(self._samples) == _CAPTURE_LEN:
                    samples = np.array(self._samples, dtype=np.int16)
                    self._state = "idle"
                    self._samples = []
                    self._on_status(f"Captured {_CAPTURE_LEN} samples.")
                    self._on_samples(samples)
