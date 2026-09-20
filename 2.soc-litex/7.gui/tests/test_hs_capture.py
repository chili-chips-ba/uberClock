import struct
import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui.hs_capture import DDR_CAPTURE_BASE, HsCaptureController
from uc_gui.ubd3_protocol import MAGIC


def _packet(seq: int, offset: int, total: int, payload: bytes) -> bytes:
    return struct.pack("<IIII", MAGIC, seq, offset, total) + payload


def _samples_packet(values: list[int]) -> bytes:
    payload = np.array(values, dtype="<i2").tobytes()
    return _packet(0, 0, len(payload), payload)


class HsCaptureControllerTests(unittest.TestCase):
    def test_start_sends_channel_select_then_ub_cap_then_ub_wait(self):
        sent = []
        controller = HsCaptureController(send_command=sent.append, on_samples=lambda arr: None)
        controller.start(channel=1, beats=2, dst_ip="192.168.0.2", dst_port=5000)

        addr = f"0x{DDR_CAPTURE_BASE:x}"
        self.assertEqual(sent, ["highspeed_dbg_select 1", f"ub_cap {addr} 2", "ub_wait"])
        self.assertTrue(controller.is_busy)

    def test_start_with_use_ramp_sends_ub_ramp_instead_of_channel_select_and_ub_cap(self):
        sent = []
        controller = HsCaptureController(send_command=sent.append, on_samples=lambda arr: None)
        controller.start(channel=1, beats=2, dst_ip="192.168.0.2", dst_port=5000, use_ramp=True)

        addr = f"0x{DDR_CAPTURE_BASE:x}"
        self.assertEqual(sent, [f"ub_ramp {addr} 2", "ub_wait"])

    def test_raises_if_already_busy(self):
        controller = HsCaptureController(send_command=lambda line: None, on_samples=lambda arr: None)
        controller.start(channel=0, beats=2, dst_ip="192.168.0.2", dst_port=5000)
        with self.assertRaises(RuntimeError):
            controller.start(channel=1, beats=2, dst_ip="192.168.0.2", dst_port=5000)

    def test_dma_done_triggers_ub_send_with_correct_byte_count(self):
        sent = []
        controller = HsCaptureController(send_command=sent.append, on_samples=lambda arr: None)
        controller.start(channel=0, beats=4, dst_ip="192.168.0.2", dst_port=5000)
        sent.clear()

        controller.feed_text("Waiting for DMA ... done.\n")

        addr = f"0x{DDR_CAPTURE_BASE:x}"
        self.assertEqual(sent, [f"ub_send {addr} 128 192.168.0.2 5000"])  # 4 beats * 32 bytes

    def test_full_happy_path_produces_samples(self):
        results = []
        controller = HsCaptureController(
            send_command=lambda line: None, on_samples=results.append
        )
        controller.start(channel=0, beats=2, dst_ip="192.168.0.2", dst_port=5000)  # 64 bytes = 32 samples
        controller.feed_text("Waiting for DMA ... done.\n")

        values = list(range(-16, 16))  # 32 samples
        controller.feed_packet(_samples_packet(values))

        self.assertFalse(controller.is_busy)
        self.assertEqual(len(results), 1)
        np.testing.assert_array_equal(results[0], np.array(values, dtype=np.int16))

    def test_packets_before_streaming_state_are_ignored(self):
        results = []
        controller = HsCaptureController(send_command=lambda line: None, on_samples=results.append)
        controller.start(channel=0, beats=1, dst_ip="192.168.0.2", dst_port=5000)
        controller.feed_packet(_samples_packet(list(range(16))))  # arrives before "done." - stray/early
        self.assertEqual(results, [])
        self.assertTrue(controller.is_busy)

    def test_dma_error_line_fails_the_capture(self):
        statuses = []
        controller = HsCaptureController(
            send_command=lambda line: None, on_samples=lambda arr: None, on_status=statuses.append
        )
        controller.start(channel=0, beats=1, dst_ip="192.168.0.2", dst_port=5000)
        controller.feed_text("Waiting for DMA ... done.\nDMA error flag is set!\n")

        self.assertFalse(controller.is_busy)
        self.assertIn("DMA error", statuses[-1])

    def test_no_arp_reply_fails_during_streaming(self):
        statuses = []
        controller = HsCaptureController(
            send_command=lambda line: None, on_samples=lambda arr: None, on_status=statuses.append
        )
        controller.start(channel=0, beats=1, dst_ip="192.168.0.2", dst_port=5000)
        controller.feed_text("Waiting for DMA ... done.\n")
        controller.feed_text("No ARP reply.\n")

        self.assertFalse(controller.is_busy)
        self.assertIn("No ARP reply", statuses[-1])

    def test_error_prefixed_line_fails_during_streaming(self):
        statuses = []
        controller = HsCaptureController(
            send_command=lambda line: None, on_samples=lambda arr: None, on_status=statuses.append
        )
        controller.start(channel=0, beats=1, dst_ip="bad-ip", dst_port=5000)
        controller.feed_text("Waiting for DMA ... done.\n")
        controller.feed_text("Error: bad dst_ip format (use a.b.c.d)\n")

        self.assertFalse(controller.is_busy)
        self.assertIn("ub_send rejected", statuses[-1])

    def test_dma_done_line_split_across_feed_calls_is_still_recognized(self):
        sent = []
        controller = HsCaptureController(send_command=sent.append, on_samples=lambda arr: None)
        controller.start(channel=0, beats=1, dst_ip="192.168.0.2", dst_port=5000)
        sent.clear()

        controller.feed_text("Waiting for DMA ... do")
        self.assertEqual(sent, [])
        controller.feed_text("ne.\n")
        self.assertEqual(len(sent), 1)
        self.assertTrue(sent[0].startswith("ub_send"))

    def test_timeout_fails_an_incomplete_streaming_capture(self):
        statuses = []
        controller = HsCaptureController(
            send_command=lambda line: None,
            on_samples=lambda arr: None,
            on_status=statuses.append,
            timeout_seconds=0.0,
        )
        controller.start(channel=0, beats=2, dst_ip="192.168.0.2", dst_port=5000)
        controller.feed_text("Waiting for DMA ... done.\n")

        controller.check_timeout()

        self.assertFalse(controller.is_busy)
        self.assertIn("timed out", statuses[-1])

    def test_check_timeout_is_a_noop_while_idle_or_waiting_for_dma(self):
        controller = HsCaptureController(
            send_command=lambda line: None, on_samples=lambda arr: None, timeout_seconds=0.0
        )
        controller.check_timeout()  # idle - must not raise or change state
        self.assertFalse(controller.is_busy)

        controller.start(channel=0, beats=1, dst_ip="192.168.0.2", dst_port=5000)
        controller.check_timeout()  # still waiting_dma, not streaming yet
        self.assertTrue(controller.is_busy)

    def test_status_callback_reports_progress(self):
        statuses = []
        controller = HsCaptureController(
            send_command=lambda line: None, on_samples=lambda arr: None, on_status=statuses.append
        )
        controller.start(channel=0, beats=1, dst_ip="192.168.0.2", dst_port=5000)
        controller.feed_text("Waiting for DMA ... done.\n")
        controller.feed_packet(_samples_packet(list(range(16))))

        self.assertIn("Capturing", statuses[0])
        self.assertIn("streaming", statuses[1])
        self.assertIn("Captured 16", statuses[2])


if __name__ == "__main__":
    unittest.main()
