"""Drive a high-speed debug capture: DMA to DDR, then stream it back over
UDP, behind one button.

Without this, an HS capture means hand-typing four commands in the right
order - `highspeed_dbg_select`, `ub_cap <addr> <beats>`, `ub_wait`, then
`ub_send <addr> <bytes> <ip> <port>` - and separately running something
to catch the UDP stream (see `cmd_ub_send()` / the `ubd3_hdr` protocol in
`2.soc-litex/2.sw/uberclock.c`). This wraps that sequence into a state
machine, the same shape as `LsCaptureController`: call `start(...)` once,
feed it console text and arriving UDP packets as they show up, and get a
finished numpy array back.

Kept socket-free on purpose (like `ls_capture.py` is serial-io-free): the
caller owns an `HsUdpReceiver`, polls it, and forwards packets in via
`feed_packet()`. That keeps this class testable with plain synthetic
packets, no real socket needed.
"""

from __future__ import annotations

import re
import time
from typing import Callable

import numpy as np

from .ubd3_protocol import Reassembler

SendCommand = Callable[[str], None]
StatusCallback = Callable[[str], None]
SamplesCallback = Callable[[np.ndarray], None]

# The S2MM path packs one 16-bit (sign-extended) sample per 16-bit lane,
# 16 lanes per 256-bit beat - see SamplePackerStream/RampSource in
# 8.python/src/uberclock_soc/streams.py. So every beat is 32 raw bytes,
# and the whole captured region is just consecutive little-endian int16
# samples once reassembled.
_BEAT_BYTES = 32

# UBDDR3's dedicated side-memory region (soc.py: UBDDR3_BASE), used as
# scratch space for every capture - there's no reason to ever point this
# elsewhere, so it isn't exposed as a user-facing option.
DDR_CAPTURE_BASE = 0xA0000000

_DMA_DONE_RE = re.compile(r"^Waiting for DMA \.\.\. done\.$")
_DMA_ERROR_RE = re.compile(r"^DMA error flag is set!$")


class HsCaptureController:
    """State machine: select channel -> DMA capture to DDR -> wait for
    DMA -> stream over UDP -> reassembled samples.

    Feed it plain, ANSI-stripped console text via `feed_text()` and
    incoming UDP datagrams via `feed_packet()`; call `check_timeout()`
    periodically while `is_busy` so a lost final UDP packet doesn't wait
    forever (UDP has no retry - see `ubd3_protocol.py`).
    """

    def __init__(
        self,
        send_command: SendCommand,
        on_samples: SamplesCallback,
        on_status: StatusCallback = lambda message: None,
        ddr_base: int = DDR_CAPTURE_BASE,
        # cmd_ub_send() (uberclock.c) re-runs eth_init() - PHY link
        # re-negotiation - and then busy-polls ARP resolution for up to
        # 200000 iterations before it ever calls udp_send() the first
        # time, all *inside* this "streaming" window. On real hardware
        # that easily exceeds 5s, especially right after a fresh
        # gateware load; a too-short deadline here closes the UDP
        # socket before the firmware has actually sent anything, which
        # looks like "missing 0 of None bytes" even though the packets
        # do go out (confirmed via tcpdump) - just too late.
        timeout_seconds: float = 20.0,
    ) -> None:
        self._send_command = send_command
        self._on_samples = on_samples
        self._on_status = on_status
        self._ddr_base = ddr_base
        self._timeout_seconds = timeout_seconds

        self._state = "idle"  # idle -> waiting_dma -> streaming -> idle
        self._line_buffer = ""
        self._reassembler = Reassembler()
        self._beats = 0
        self._dst_ip = ""
        self._dst_port = 0
        self._stream_deadline = 0.0

    @property
    def is_busy(self) -> bool:
        return self._state != "idle"

    def start(
        self, channel: int, beats: int, dst_ip: str, dst_port: int, use_ramp: bool = False
    ) -> None:
        if self.is_busy:
            raise RuntimeError("a capture is already in progress")
        self._state = "waiting_dma"
        self._line_buffer = ""
        self._reassembler = Reassembler()
        self._beats = beats
        self._dst_ip = dst_ip
        self._dst_port = dst_port

        addr_hex = f"0x{self._ddr_base:x}"
        if use_ramp:
            # ub_ramp forces the DMA to capture a deterministic internal
            # counting pattern instead of the design's selected signal -
            # a way to test the DMA/UDP/plot pipeline on its own, so
            # highspeed_dbg_select is irrelevant here.
            self._send_command(f"ub_ramp {addr_hex} {beats}")
        else:
            self._send_command(f"highspeed_dbg_select {channel}")
            self._send_command(f"ub_cap {addr_hex} {beats}")
        # ub_cap/ub_ramp only kick off the DMA request and return
        # immediately ("S2MM start: ..."); the "Waiting for DMA ...
        # done." line this controller watches for is only ever printed
        # by ub_wait, which has to be sent explicitly - it isn't implied
        # by either of them.
        self._send_command("ub_wait")
        self._on_status("Capturing to DDR...")

    def feed_text(self, text: str) -> None:
        """Feed a chunk of plain (ANSI-stripped) console text. See
        `LsCaptureController.feed_text` - same line-buffering reasoning:
        a UART read can end mid-line, so only complete lines are acted
        on and a trailing partial line waits for the next call."""
        self._line_buffer += text
        while "\n" in self._line_buffer:
            line, self._line_buffer = self._line_buffer.split("\n", 1)
            self._feed_line(line.strip())

    def _feed_line(self, line: str) -> None:
        if _DMA_ERROR_RE.match(line):
            # ub_wait prints "done." *then* checks the error flag, so
            # this can arrive right after we've already moved on to
            # "streaming" - check it independent of state rather than
            # only while still "waiting_dma".
            if self.is_busy:
                self._fail("Firmware reported a DMA error.")
            return

        if self._state == "waiting_dma":
            if _DMA_DONE_RE.match(line):
                self._start_streaming()
        elif self._state == "streaming":
            if line == "No ARP reply.":
                self._fail("No ARP reply from the host - check dst IP and the network link.")
            elif line.startswith("Error:"):
                self._fail(f"ub_send rejected the request: {line}")

    def _start_streaming(self) -> None:
        self._state = "streaming"
        self._stream_deadline = time.monotonic() + self._timeout_seconds
        total_bytes = self._beats * _BEAT_BYTES
        addr_hex = f"0x{self._ddr_base:x}"
        self._on_status(f"DMA done, streaming {total_bytes} bytes over UDP...")
        self._send_command(f"ub_send {addr_hex} {total_bytes} {self._dst_ip} {self._dst_port}")

    def feed_packet(self, packet: bytes) -> None:
        """Feed one UDP datagram, in whatever order it arrived."""
        if self._state != "streaming":
            return
        self._reassembler.feed_packet(packet)
        if self._reassembler.is_complete:
            samples = np.frombuffer(self._reassembler.buffer, dtype="<i2")
            self._state = "idle"
            self._on_status(f"Captured {len(samples)} samples.")
            self._on_samples(samples)

    def check_timeout(self) -> None:
        """Call periodically (e.g. from a QTimer) while `is_busy` - a
        dropped final packet would otherwise leave `streaming` waiting
        forever, since UDP never retries."""
        if self._state != "streaming" or time.monotonic() < self._stream_deadline:
            return
        missing = self._reassembler.missing_bytes
        self._fail(f"Capture timed out - missing {missing} of {self._reassembler.total} bytes.")

    def _fail(self, message: str) -> None:
        self._state = "idle"
        self._on_status(message)
