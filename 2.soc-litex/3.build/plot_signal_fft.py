#!/usr/bin/env python3
import sys
import numpy as np
import matplotlib.pyplot as plt

FS = 10_000.0  # sampling rate (Hz)

def load_csv(path):
    idx = []
    val = []
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
        raise RuntimeError("No data parsed. Expect lines like: 0,123")
    return np.asarray(idx, dtype=np.int64), np.asarray(val, dtype=np.float64)

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 plot_signal_fft.py samples.csv")
        sys.exit(1)

    path = sys.argv[1]
    idx, x = load_csv(path)
    N = len(x)
    t = np.arange(N) / FS

    # --- Time-domain plot ---
    plt.figure()
    plt.plot(t, x)
    plt.title("Signal vs Time")
    plt.xlabel("Time [s]")
    plt.ylabel("Amplitude")
    plt.grid(True)

    # --- FFT (single-sided) ---
    # Windowing to reduce leakage
    win = np.hanning(N)
    xw = x * win

    # Use rfft for real input; get frequency bins up to Nyquist
    X = np.fft.rfft(xw)
    freqs = np.fft.rfftfreq(N, d=1.0/FS)

    # Amplitude scaling for Hann window to approximate true amplitude
    # Coherent gain (CG) of Hann is 0.5
    CG = 0.5
    # Convert to magnitude spectrum (per-bin), normalized by N and CG
    mag = np.abs(X) * (2.0 / (N * CG))  # factor 2 for single-sided spectrum (except DC/Nyquist)
    # Avoid log(0)
    mag_db = 20.0 * np.log10(np.maximum(mag, 1e-12))

    plt.figure()
    plt.plot(freqs, mag)
    plt.title("Single-Sided FFT Magnitude (Hann, dB)")
    plt.xlabel("Frequency [Hz]")
    plt.ylabel("Magnitude [dB]")
    plt.grid(True)
    plt.xlim(0, FS/2)

    plt.show()

if __name__ == "__main__":
    main()

