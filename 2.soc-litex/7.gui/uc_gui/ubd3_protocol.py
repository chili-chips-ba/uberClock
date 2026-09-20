"""Parse and reassemble the firmware's "UBD3" UDP capture-streaming
protocol (`cmd_ub_send()` in `2.soc-litex/2.sw/uberclock.c`).

Each UDP datagram is a fixed 16-byte little-endian header followed by a
chunk of raw DDR memory bytes::

    struct ubd3_hdr {
        uint32_t magic;   // 0x55424433, ASCII "UBD3"
        uint32_t seq;     // packet index, 0-based
        uint32_t offset;  // byte offset of this chunk within the transfer
        uint32_t total;   // total transfer size in bytes (same every packet)
    };

There's no ack/retry - the firmware fires packets as fast as it can and
moves on. That means the receiver has to tolerate loss and reordering
itself, which is exactly what `Reassembler` does: it places each
packet's payload at its `offset` (so out-of-order arrival is harmless)
and tracks how many distinct bytes have actually been covered, without
assuming every packet arrives at all. Duplicate detection isn't needed:
the firmware never resends, so double-writing the same offset is at
worst a no-op.

No Qt or sockets here - this is the plain-Python framing/assembly logic,
kept unit-testable the same way `cmd_catalog.py` and `ansi_text.py` are.
The actual socket I/O lives in `hs_udp_receiver.py`.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from typing import Optional

MAGIC = 0x55424433  # "UBD3"
HEADER_SIZE = 16
_HEADER_STRUCT = struct.Struct("<IIII")  # magic, seq, offset, total


@dataclass(frozen=True)
class PacketHeader:
    seq: int
    offset: int
    total: int


def parse_header(packet: bytes) -> Optional[PacketHeader]:
    """Parse a UBD3 packet's header, or None if it's too short or its
    magic doesn't match (i.e. it isn't one of ours - stray traffic on
    the port shouldn't corrupt a capture)."""
    if len(packet) < HEADER_SIZE:
        return None
    magic, seq, offset, total = _HEADER_STRUCT.unpack_from(packet, 0)
    if magic != MAGIC:
        return None
    return PacketHeader(seq=seq, offset=offset, total=total)


class Reassembler:
    """Accumulates UBD3 packets into one contiguous byte buffer.

    `total` is learned from the first valid packet (every packet
    repeats it, so any one of them is enough). Feed packets in any
    order via `feed_packet()`; check `is_complete` / `missing_bytes`
    to know when (or whether) the transfer actually finished.
    """

    def __init__(self) -> None:
        self._total: Optional[int] = None
        self._buffer = bytearray()
        self._covered = bytearray()  # parallel byte-mask: 1 where written

    @property
    def total(self) -> Optional[int]:
        return self._total

    @property
    def is_complete(self) -> bool:
        return self._total is not None and self.missing_bytes == 0

    @property
    def missing_bytes(self) -> int:
        if self._total is None:
            return 0
        return self._total - sum(self._covered)

    @property
    def buffer(self) -> bytes:
        """The assembled data so far. Any byte never covered by a
        packet is left as 0x00 - callers should check `is_complete`
        before trusting this represents the full capture."""
        return bytes(self._buffer)

    def feed_packet(self, packet: bytes) -> None:
        header = parse_header(packet)
        if header is None:
            return  # not one of ours - ignore rather than raise

        if self._total is None:
            self._total = header.total
            self._buffer = bytearray(header.total)
            self._covered = bytearray(header.total)
        elif header.total != self._total:
            return  # inconsistent restart mid-capture - ignore, don't corrupt state

        payload = packet[HEADER_SIZE:]
        end = header.offset + len(payload)
        if end > self._total:
            payload = payload[: self._total - header.offset]  # truncate a bogus overrun
            end = self._total

        self._buffer[header.offset:end] = payload
        for i in range(header.offset, end):
            self._covered[i] = 1
