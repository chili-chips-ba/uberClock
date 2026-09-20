"""Background UDP socket reader for the UBD3 capture stream.

Mirrors `litex_link.LitexTermWorker`'s shape on purpose - a background
thread pushing raw datagrams into a `queue.Queue`, drained via
`poll()` - so `main_window.py`'s single QTimer poll loop can treat this
the same way it already treats the serial worker, instead of needing a
second, different integration pattern.
"""

from __future__ import annotations

import queue
import socket
import threading
from typing import Optional

# Comfortably above the firmware's UBD3_PAYLOAD_MAX (1400 bytes/packet).
_RECV_BUFSIZE = 2048


class HsUdpReceiver:
    """Listens for UBD3 packets on one UDP port until `stop()`."""

    def __init__(self) -> None:
        self._socket: Optional[socket.socket] = None
        self._thread: Optional[threading.Thread] = None
        self._queue: "queue.Queue[bytes]" = queue.Queue()
        self._stop_event = threading.Event()

    @property
    def is_listening(self) -> bool:
        return self._thread is not None and self._thread.is_alive()

    @property
    def port(self) -> Optional[int]:
        """The actually-bound port - useful when `start(port=0)` asked
        the OS to pick one (as tests do to avoid port collisions)."""
        return self._socket.getsockname()[1] if self._socket is not None else None

    def start(self, port: int) -> None:
        if self.is_listening:
            raise RuntimeError("already listening")
        self._stop_event.clear()

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("0.0.0.0", port))
        sock.settimeout(0.2)  # let the read loop notice stop_event promptly
        self._socket = sock

        self._thread = threading.Thread(target=self._read_loop, daemon=True)
        self._thread.start()

    def _read_loop(self) -> None:
        assert self._socket is not None
        while not self._stop_event.is_set():
            try:
                packet, _addr = self._socket.recvfrom(_RECV_BUFSIZE)
            except socket.timeout:
                continue
            except OSError:
                return  # socket closed out from under us by stop()
            self._queue.put(packet)

    def poll(self) -> list[bytes]:
        """Drain and return every packet received since the last call."""
        packets = []
        while True:
            try:
                packets.append(self._queue.get_nowait())
            except queue.Empty:
                break
        return packets

    def stop(self) -> None:
        self._stop_event.set()
        if self._socket is not None:
            self._socket.close()
        if self._thread is not None:
            self._thread.join(timeout=1.0)
        self._socket = None
        self._thread = None
