from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


FS = 65_000_000.0
FILES = [
    "tx_out1.txt",
    "tx_out2.txt",
    "tx_out3.txt",
    "tx_out4.txt",
    "tx_out5.txt",
    "sum_output.txt",
]


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
    base = Path(__file__).resolve().parent
    fig, axes = plt.subplots(len(FILES), 1, figsize=(12, 16), sharex=True)
    fig.suptitle("Stacked FFT Plots for TX Outputs and System Sum")

    for ax, name in zip(axes, FILES):
        path = base / name
        signal = load_signal(path)
        freqs, power_db = fft_power_db(signal, FS)
        peak_idx = int(np.argmax(power_db[1:]) + 1) if len(power_db) > 1 else 0
        peak_freq = freqs[peak_idx]
        peak_power = power_db[peak_idx]

        ax.plot(freqs, power_db, linewidth=0.8)
        ax.scatter([peak_freq], [peak_power], color="red", zorder=3)
        ax.annotate(
            f"({peak_freq:.3f} Hz, {peak_power:.2f} dB)",
            xy=(peak_freq, peak_power),
            xytext=(10, 8),
            textcoords="offset points",
            color="red",
            fontsize=8,
            bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="red", alpha=0.8),
        )
        ax.set_ylabel("Power (dB)")
        ax.set_title(name)
        ax.grid(True, alpha=0.3)

    axes[-1].set_xlabel("Frequency (Hz)")
    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
