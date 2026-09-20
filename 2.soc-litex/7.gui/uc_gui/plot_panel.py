"""Embedded matplotlib plot with switchable Time/FFT/Histogram views.

Wired up to a capture controller's `on_samples` callback (see
`LsCaptureController` in `ls_capture.py`) via `set_samples()`. Kept as a
thin Qt shell around `plot_data`: this file only owns matplotlib
Figure/Axes bookkeeping and widget layout - the actual math lives in
`plot_data.py` and is tested there, without needing a display.
"""

from __future__ import annotations

import numpy as np
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg
from matplotlib.figure import Figure
from PySide6.QtWidgets import QComboBox, QLabel, QVBoxLayout, QWidget

from . import plot_data

VIEWS = ("Time", "FFT", "Histogram")


class PlotPanel(QWidget):
    """Shows the most recently captured array; call `set_samples()` when
    a new capture arrives, and the user can switch views at any time."""

    def __init__(self, sample_rate: float = 1.0) -> None:
        super().__init__()
        self._samples: np.ndarray | None = None
        self._sample_rate = sample_rate
        self._source_label = ""

        layout = QVBoxLayout(self)

        self.view_box = QComboBox()
        self.view_box.addItems(VIEWS)
        self.view_box.currentTextChanged.connect(self._redraw)
        layout.addWidget(self.view_box)

        self.figure = Figure(figsize=(5, 3))
        self.axes = self.figure.add_subplot(111)
        self.canvas = FigureCanvasQTAgg(self.figure)
        layout.addWidget(self.canvas, 1)

        self.empty_label = QLabel("No capture yet.")
        layout.addWidget(self.empty_label)

        self._redraw()

    def set_samples(self, samples: np.ndarray, sample_rate: float | None = None, source_label: str = "") -> None:
        """Show a newly captured array. `sample_rate` overrides the rate
        used for the FFT view - LS and HS captures run at very different
        rates, so whichever controller produced `samples` should pass
        its own rate rather than relying on a single fixed default."""
        self._samples = samples
        if sample_rate is not None:
            self._sample_rate = sample_rate
        self._source_label = source_label
        self._redraw()

    def _redraw(self, *_args) -> None:
        self.axes.clear()

        if self._samples is None:
            self.canvas.setVisible(False)
            self.empty_label.setVisible(True)
            self.canvas.draw_idle()
            return

        self.canvas.setVisible(True)
        self.empty_label.setVisible(False)

        view = self.view_box.currentText()
        if view == "Time":
            x, y = plot_data.time_series(self._samples)
            self.axes.plot(x, y)
            self.axes.set_xlabel("Sample index")
            self.axes.set_ylabel("Value")
        elif view == "FFT":
            x, y = plot_data.fft_spectrum(self._samples, self._sample_rate)
            self.axes.plot(x, y)
            freq_label = "Normalized frequency" if self._sample_rate == 1.0 else "Frequency (Hz)"
            self.axes.set_xlabel(freq_label)
            self.axes.set_ylabel("Magnitude (dB)")
        elif view == "Histogram":
            edges, counts = plot_data.histogram(self._samples)
            self.axes.stairs(counts, edges, fill=True)
            self.axes.set_xlabel("Value")
            self.axes.set_ylabel("Count")

        title = f"{view} - {len(self._samples)} samples"
        if self._source_label:
            title = f"{self._source_label} - {title}"
        self.axes.set_title(title)
        self.figure.tight_layout()
        self.canvas.draw_idle()
