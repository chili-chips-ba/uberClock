import socket
import sys
import time
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui.hs_udp_receiver import HsUdpReceiver


class HsUdpReceiverTests(unittest.TestCase):
    def test_receives_packets_sent_over_loopback(self):
        # Real loopback UDP traffic rather than a mock socket - fast and
        # gives genuine coverage of the bind/thread/queue plumbing, the
        # same "real stand-in over mock" approach used for LitexTermWorker.
        receiver = HsUdpReceiver()
        receiver.start(port=0)
        sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sender.sendto(b"hello", ("127.0.0.1", receiver.port))
            sender.sendto(b"world", ("127.0.0.1", receiver.port))

            packets = []
            deadline = time.monotonic() + 2.0
            while time.monotonic() < deadline and len(packets) < 2:
                packets.extend(receiver.poll())
                time.sleep(0.01)

            self.assertEqual(sorted(packets), [b"hello", b"world"])
        finally:
            sender.close()
            receiver.stop()

    def test_is_listening_reflects_state(self):
        receiver = HsUdpReceiver()
        self.assertFalse(receiver.is_listening)
        receiver.start(port=0)
        self.assertTrue(receiver.is_listening)
        receiver.stop()
        self.assertFalse(receiver.is_listening)

    def test_stop_then_start_again_works(self):
        receiver = HsUdpReceiver()
        receiver.start(port=0)
        receiver.stop()
        receiver.start(port=0)  # must not raise
        receiver.stop()

    def test_starting_twice_without_stop_raises(self):
        receiver = HsUdpReceiver()
        receiver.start(port=0)
        try:
            with self.assertRaises(RuntimeError):
                receiver.start(port=0)
        finally:
            receiver.stop()

    def test_stop_before_start_is_safe(self):
        HsUdpReceiver().stop()  # must not raise

    def test_poll_returns_empty_when_nothing_arrived(self):
        receiver = HsUdpReceiver()
        receiver.start(port=0)
        try:
            self.assertEqual(receiver.poll(), [])
        finally:
            receiver.stop()


if __name__ == "__main__":
    unittest.main()
