import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui.ls_capture import LsCaptureController


def _dump_text(values: list[int]) -> str:
    lines = ["#idx,value"]
    lines += [f"{i},{v}" for i, v in enumerate(values)]
    return "\n".join(lines) + "\n"


class LsCaptureControllerTests(unittest.TestCase):
    def test_start_sends_channel_select_then_cap_start(self):
        sent = []
        controller = LsCaptureController(send_command=sent.append, on_samples=lambda arr: None)
        controller.start(channel=6)
        self.assertEqual(sent, ["lowspeed_dbg_select 6", "cap_start"])
        self.assertTrue(controller.is_busy)

    def test_raises_if_already_busy(self):
        controller = LsCaptureController(send_command=lambda line: None, on_samples=lambda arr: None)
        controller.start(channel=0)
        with self.assertRaises(RuntimeError):
            controller.start(channel=1)

    def test_full_happy_path_produces_2048_samples(self):
        sent = []
        results = []
        controller = LsCaptureController(send_command=sent.append, on_samples=results.append)
        controller.start(channel=2)

        values = list(range(-1024, 1024))  # 2048 distinct values
        # Firmware reports IN-PROGRESS once before DONE - controller should
        # just keep waiting, not treat it as an error or a finish signal.
        controller.feed_text("Capture IN-PROGRESS\n")
        self.assertTrue(controller.is_busy)

        controller.feed_text("Capture DONE\n")
        self.assertIn("cap_dump", sent)

        controller.feed_text(_dump_text(values))

        self.assertFalse(controller.is_busy)
        self.assertEqual(len(results), 1)
        np.testing.assert_array_equal(results[0], np.array(values, dtype=np.int16))
        self.assertEqual(results[0].dtype, np.int16)

    def test_status_line_split_across_feed_calls_is_still_recognized(self):
        # Regression test: real serial reads have no notion of line
        # boundaries. "Capture DONE\n" arriving as "Capture DO" then
        # "NE\n" across two separate feed_text() calls used to be dropped
        # silently (each call ran splitlines() independently with no
        # memory of the previous call's dangling fragment) - the GUI just
        # kept polling `cap_status` in a loop instead of moving on.
        sent = []
        results = []
        controller = LsCaptureController(send_command=sent.append, on_samples=results.append)
        controller.start(channel=0)
        sent.clear()

        controller.feed_text("Capture DO")
        self.assertTrue(controller.is_busy)
        self.assertNotIn("cap_dump", sent)  # must not have triggered yet

        controller.feed_text("NE\n")
        self.assertIn("cap_dump", sent)

        controller.feed_text(_dump_text(list(range(2048))))
        self.assertEqual(len(results), 1)

    def test_dump_line_split_across_feed_calls_is_still_recognized(self):
        sent = []
        results = []
        controller = LsCaptureController(send_command=sent.append, on_samples=results.append)
        controller.start(channel=0)
        controller.feed_text("Capture DONE\n")

        full_dump = _dump_text(list(range(2048)))
        midpoint = len(full_dump) // 2
        # Split mid-line on purpose, not on a convenient newline boundary.
        split_at = full_dump.index(",", midpoint)
        controller.feed_text(full_dump[:split_at])
        self.assertEqual(len(results), 0)
        controller.feed_text(full_dump[split_at:])

        self.assertEqual(len(results), 1)
        np.testing.assert_array_equal(results[0], np.array(range(2048), dtype=np.int16))

    def test_ignores_noise_lines_and_ansi_prompt_leftovers(self):
        # Real firmware output includes the colored prompt reprinted between
        # commands (already ANSI-stripped by the time it reaches here) and
        # blank lines - none of that should confuse the line parser.
        sent = []
        results = []
        controller = LsCaptureController(send_command=sent.append, on_samples=results.append)
        controller.start(channel=0)

        controller.feed_text("uberClock> \ncap_status\n\nCapture DONE\n\nuberClock> \n")
        controller.feed_text("cap_dump\n\n" + _dump_text(list(range(2048))) + "uberClock> ")

        self.assertEqual(len(results), 1)
        np.testing.assert_array_equal(results[0], np.array(range(2048), dtype=np.int16))

    def test_status_callback_reports_progress(self):
        statuses = []
        controller = LsCaptureController(
            send_command=lambda line: None,
            on_samples=lambda arr: None,
            on_status=statuses.append,
        )
        controller.start(channel=0)
        controller.feed_text("Capture DONE\n")
        controller.feed_text(_dump_text(list(range(2048))))

        self.assertEqual(len(statuses), 3)
        self.assertIn("started", statuses[0])
        self.assertIn("downloading", statuses[1])
        self.assertIn("Captured 2048", statuses[2])

    def test_poll_status_only_sends_while_armed(self):
        sent = []
        controller = LsCaptureController(send_command=sent.append, on_samples=lambda arr: None)

        controller.poll_status()  # idle - should do nothing
        self.assertEqual(sent, [])

        controller.start(channel=0)
        sent.clear()
        controller.poll_status()
        self.assertEqual(sent, ["cap_status"])

        controller.feed_text("Capture DONE\n")
        sent.clear()
        controller.poll_status()  # now waiting_dump, not armed - should do nothing
        self.assertEqual(sent, [])


if __name__ == "__main__":
    unittest.main()
