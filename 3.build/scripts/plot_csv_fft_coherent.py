#!/usr/bin/env python3
import sys
import numpy as np
import matplotlib.pyplot as plt

FS = 10_000.0

# coherent analysis settings for 940/1000/1060 Hz
SKIP = 200
NFFT = 1000   # 10 Hz/bin, tones land exactly on bins

TARGETS = [940.0, 1000.0, 1060.0]

def load_csv(path):
    idx, val = [], []
    with open(path, "r") as f:
        for ln in f:
            ln = ln.strip()
            if not ln or ln.startswith("#"):
                continue
            parts = ln.split(",")
            if len(parts) != 2:
                continue
            try:
                i = int(parts[0].strip())
                v = float(parts[1].strip())
            except ValueError:
                continue
            idx.append(i)
            val.append(v)
    if not val:
        raise RuntimeError("No data parsed")
    return np.asarray(idx), np.asarray(val, dtype=np.float64)

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 plot_csv_fft_coherent.py samples.csv")
        sys.exit(1)

    _, x = load_csv(sys.argv[1])

    if len(x) < SKIP + NFFT:
        raise RuntimeError(f"Need at least {SKIP + NFFT} samples, got {len(x)}")

    x = x[SKIP:SKIP + NFFT]
    N = len(x)
    t = np.arange(N) / FS

    # Time domain
    plt.figure()
    plt.step(t, x, where="mid")
    plt.title(f"Signal vs Time (skip={SKIP}, N={N})")
    plt.xlabel("Time [s]")
    plt.ylabel("Amplitude")
    plt.grid(True)

    # Rectangular-window FFT (best for coherent tones)
    X = np.fft.rfft(x)
    freqs = np.fft.rfftfreq(N, d=1.0 / FS)

    mag = np.abs(X) * (2.0 / N)
    mag[0] *= 0.5
    if N % 2 == 0:
        mag[-1] *= 0.5

    plt.figure()
    plt.stem(freqs, mag)
    plt.title(f"Single-Sided FFT Magnitude (Rectangular, N={N}, df={FS/N:.1f} Hz)")
    plt.xlabel("Frequency [Hz]")
    plt.ylabel("Amplitude")
    plt.xlim(850, 1150)
    plt.grid(True, alpha=0.3)

    for f0 in TARGETS:
        k = int(round(f0 * N / FS))
        plt.annotate(f"{freqs[k]:.1f} Hz\n{mag[k]:.2f}",
                     xy=(freqs[k], mag[k]),
                     xytext=(0, 8),
                     textcoords="offset points",
                     ha="center", va="bottom", fontsize=8)

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
