import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui import plot_data


class TimeSeriesTests(unittest.TestCase):
    def test_x_is_sample_index_y_is_unchanged(self):
        samples = np.array([5, -3, 7, 0], dtype=np.int16)
        x, y = plot_data.time_series(samples)
        np.testing.assert_array_equal(x, np.arange(4))
        np.testing.assert_array_equal(y, samples)


class FftSpectrumTests(unittest.TestCase):
    def test_pure_tone_peaks_at_its_own_frequency(self):
        sample_rate = 1000.0
        tone_hz = 100.0
        n = 1000
        t = np.arange(n) / sample_rate
        samples = np.sin(2 * np.pi * tone_hz * t)

        freqs, magnitude_db = plot_data.fft_spectrum(samples, sample_rate)
        peak_freq = freqs[np.argmax(magnitude_db)]

        self.assertAlmostEqual(peak_freq, tone_hz, delta=sample_rate / n)

    def test_dc_bias_is_removed_before_transform(self):
        # A pure DC signal should show (near) nothing above the floor at
        # every frequency once the mean is subtracted - if the DC offset
        # leaked through, bin 0 would dominate every other bin.
        samples = np.full(256, 1000.0)
        _, magnitude_db = plot_data.fft_spectrum(samples)
        self.assertTrue(np.all(magnitude_db < -100))

    def test_output_lengths_match_rfft_convention(self):
        samples = np.zeros(256)
        freqs, magnitude_db = plot_data.fft_spectrum(samples)
        self.assertEqual(len(freqs), 129)  # n // 2 + 1
        self.assertEqual(len(magnitude_db), 129)


class HistogramTests(unittest.TestCase):
    def test_counts_sum_to_sample_count(self):
        samples = np.array([1, 2, 2, 3, 3, 3, -5, 10], dtype=np.int16)
        edges, counts = plot_data.histogram(samples, bins=4)
        self.assertEqual(len(edges), 5)  # bins + 1 edges
        self.assertEqual(counts.sum(), len(samples))

    def test_all_identical_values_land_in_one_bin(self):
        samples = np.full(50, 7, dtype=np.int16)
        _, counts = plot_data.histogram(samples, bins=10)
        self.assertEqual(counts.max(), 50)


if __name__ == "__main__":
    unittest.main()
