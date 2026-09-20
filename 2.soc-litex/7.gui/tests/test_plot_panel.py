import os
import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication

from uc_gui.plot_panel import PlotPanel, VIEWS

_app = QApplication.instance() or QApplication([])


class PlotPanelTests(unittest.TestCase):
    def test_starts_empty_with_no_error(self):
        # isHidden() reflects the widget's own explicit visibility flag;
        # isVisible() would be False here regardless since the panel is
        # never actually shown in a window during this test.
        panel = PlotPanel()
        self.assertTrue(panel.canvas.isHidden())
        self.assertFalse(panel.empty_label.isHidden())

    def test_set_samples_shows_canvas_and_hides_empty_label(self):
        panel = PlotPanel()
        panel.set_samples(np.arange(100))
        self.assertFalse(panel.canvas.isHidden())
        self.assertTrue(panel.empty_label.isHidden())

    def test_every_view_renders_without_raising(self):
        panel = PlotPanel()
        panel.set_samples(np.array(range(-50, 50), dtype=np.int16))
        for view in VIEWS:
            panel.view_box.setCurrentText(view)  # triggers _redraw via signal

    def test_switching_view_before_any_capture_is_a_noop(self):
        panel = PlotPanel()
        panel.view_box.setCurrentText("FFT")
        self.assertFalse(panel.empty_label.isHidden())

    def test_fft_x_axis_uses_the_given_sample_rate(self):
        # LS debug capture runs on a fixed 10 kHz tick - a tone at 1 kHz
        # should show its FFT peak near 1000 on the x-axis, not near 0.1
        # (which is what a default sample_rate=1.0 would produce).
        sample_rate = 10_000.0
        panel = PlotPanel(sample_rate=sample_rate)
        n = 2048
        t = np.arange(n) / sample_rate
        panel.set_samples(np.sin(2 * np.pi * 1000.0 * t))
        panel.view_box.setCurrentText("FFT")

        line = panel.axes.lines[0]
        peak_freq = line.get_xdata()[np.argmax(line.get_ydata())]
        self.assertAlmostEqual(peak_freq, 1000.0, delta=sample_rate / n)

    def test_set_samples_can_override_the_sample_rate_per_call(self):
        # LS and HS captures run at very different rates - a later
        # HS capture's FFT shouldn't still be scaled by LS's rate.
        panel = PlotPanel(sample_rate=10_000.0)
        n = 2048
        hs_rate = 65_000_000.0
        t = np.arange(n) / hs_rate
        panel.set_samples(np.sin(2 * np.pi * 1_000_000.0 * t), sample_rate=hs_rate)
        panel.view_box.setCurrentText("FFT")

        line = panel.axes.lines[0]
        peak_freq = line.get_xdata()[np.argmax(line.get_ydata())]
        self.assertAlmostEqual(peak_freq, 1_000_000.0, delta=hs_rate / n)

    def test_source_label_is_included_in_the_title(self):
        panel = PlotPanel()
        panel.set_samples(np.arange(10), source_label="HS")
        self.assertIn("HS", panel.axes.get_title())


if __name__ == "__main__":
    unittest.main()
