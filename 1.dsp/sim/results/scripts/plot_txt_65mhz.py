# SPDX-FileCopyrightText: 2026 Ahmed Imamović
# SPDX-FileCopyrightText: 2026 Tarik Hamedović
# SPDX-License-Identifier: GPL-3.0-or-later

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


FS = 65_000_000.0


def load_signal(path: Path) -> np.ndarray:
    data = np.loadtxt(path, dtype=float)
    if data.ndim == 0:
        data = np.array([float(data)])
    return data


def fft_power_db(signal: np.ndarray, fs: float):
    n = len(signal)
    if n < 2:
        raise ValueError("Need at least 2 samples for FFT")
    fft_len = 1 << (n - 1).bit_length()
    window = np.hanning(n)
    signal_win = signal * window
    signal_pad = np.zeros(fft_len)
    signal_pad[:n] = signal_win
    fft_vals = np.fft.rfft(signal_pad)
    freqs = np.fft.rfftfreq(fft_len, d=1.0 / fs)
    power = np.abs(fft_vals) ** 2
    power_db = 10.0 * np.log10(np.maximum(power, 1e-20))
    return freqs, power_db


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 plot_txt_65mhz.py <file1.txt> [file2.txt ...]")
        sys.exit(1)

    for arg in sys.argv[1:]:
        path = Path(arg)
        if not path.exists():
            path = Path(__file__).resolve().parent / arg
        signal = load_signal(path)
        t = np.arange(len(signal)) / FS
        freqs, power_db = fft_power_db(signal, FS)
        peak_idx = int(np.argmax(power_db[1:]) + 1) if len(power_db) > 1 else 0
        peak_freq = freqs[peak_idx]
        peak_power = power_db[peak_idx]

        fig, (ax_t, ax_f) = plt.subplots(2, 1, figsize=(12, 8))
        fig.suptitle(f"{path.name} at 65 MHz sample rate")

        ax_t.plot(t, signal, linewidth=0.8)
        ax_t.set_title("Time Domain")
        ax_t.set_xlabel("Time (s)")
        ax_t.set_ylabel("Amplitude")
        ax_t.grid(True, alpha=0.3)

        ax_f.plot(freqs, power_db, linewidth=0.8)
        ax_f.scatter([peak_freq], [peak_power], color="red", zorder=3)
        ax_f.annotate(
            f"({peak_freq:.3f} Hz, {peak_power:.2f} dB)",
            xy=(peak_freq, peak_power),
            xytext=(10, 10),
            textcoords="offset points",
            color="red",
            fontsize=9,
            bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="red", alpha=0.8),
        )
        ax_f.set_title("FFT Power Spectrum")
        ax_f.set_xlabel("Frequency (Hz)")
        ax_f.set_ylabel("Power (dB)")
        ax_f.grid(True, alpha=0.3)

        fig.tight_layout()

    plt.show()


if __name__ == "__main__":
    main()
