"""Pure numpy transforms behind the plot panel's Time/FFT/Histogram views.

Split out from `plot_panel.py` for the same reason `ansi_text.py` splits
parsing from Qt rendering: this is the part that can have an off-by-one
or wrong-axis bug, so it's the part worth covering with tests that don't
need a display or matplotlib.
"""

from __future__ import annotations

import numpy as np


def time_series(samples: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Sample index vs raw value - the plain waveform view."""
    return np.arange(len(samples)), samples


def fft_spectrum(
    samples: np.ndarray, sample_rate: float = 1.0
) -> tuple[np.ndarray, np.ndarray]:
    """One-sided magnitude spectrum in dB, via `np.fft.rfft`.

    `sample_rate` defaults to 1.0 (normalized frequency, 0..0.5) since the
    LS capture RAM doesn't currently report its clock - pass the real
    sample rate once that's available to get frequencies in Hz instead.
    """
    values = samples.astype(np.float64)
    values = values - values.mean()  # drop DC bias so it doesn't dwarf real content
    spectrum = np.fft.rfft(values)
    freqs = np.fft.rfftfreq(len(values), d=1.0 / sample_rate)
    magnitude = np.abs(spectrum)
    magnitude_db = 20 * np.log10(np.maximum(magnitude, 1e-12))
    return freqs, magnitude_db


def histogram(samples: np.ndarray, bins: int = 64) -> tuple[np.ndarray, np.ndarray]:
    """Bin edges and counts for a value-distribution view."""
    counts, edges = np.histogram(samples, bins=bins)
    return edges, counts
