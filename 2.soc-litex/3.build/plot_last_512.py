#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
FFT plot for capture.bin (full-length capture).

Assumptions:
- capture.bin contains little-endian uint16 words that you view as int16 samples (same as your receiver).
- Total samples = 2^26 (you said this is known).
- Fs = 65 MHz.

This script:
- Loads the full file
- Interprets as int16
- Removes DC (mean)
- Applies a Hann window (recommended for spectral leakage)
- Computes rFFT (one-sided)
- Plots magnitude (linear) and magnitude in dBFS (optional)
"""

import numpy as np
import matplotlib.pyplot as plt
import os

FS_HZ         = 65e6
FNAME         = "capture.bin"
NSAMP_EXPECT  = 1 << 26          # 2^26 samples
WINDOW        = "hann"           # "hann" or "rect"
PLOT_DBFS     = True            # also show dBFS plot
PEAKS         = 10              # print top-N peaks (excluding DC)

U16_LE = np.dtype("<u2")

def make_window(n: int, kind: str) -> np.ndarray:
    kind = kind.lower()
    if kind in ("rect", "none"):
        return np.ones(n, dtype=np.float64)
    if kind in ("hann", "hanning"):
        return np.hanning(n).astype(np.float64)
    raise ValueError(f"Unknown window: {kind}")

def main():
    if not os.path.exists(FNAME):
        raise SystemExit(f"ERROR: {FNAME} not found")

    # Load full file as u16 then view as int16 (exactly like your receiver code)
    u16 = np.fromfile(FNAME, dtype=U16_LE)
    x = u16.view(np.int16)

    print(f"loaded {FNAME}: {x.size} samples ({x.nbytes} bytes)")

    if x.size != NSAMP_EXPECT:
        print(f"WARNING: expected {NSAMP_EXPECT} samples, got {x.size}")

    # Convert to float for FFT processing
    xf = x.astype(np.float64)

    # Remove DC offset (helps display)
    xf -= xf.mean()

    # Windowing
    w = make_window(xf.size, WINDOW)
    xw = xf * w

    # rFFT (one-sided)
    X = np.fft.rfft(xw)
    f = np.fft.rfftfreq(xw.size, d=1.0 / FS_HZ)

    # Amplitude scaling:
    # Coherent gain of window for amplitude correction
    cg = w.mean()
    # One-sided amplitude spectrum (approx): |X| / (N*cg) * 2, except DC/Nyquist
    N = xw.size
    mag = np.abs(X) / (N * cg)
    if mag.size > 2:
        mag[1:-1] *= 2.0  # double non-DC/non-Nyquist bins for one-sided

    # dBFS (relative to full-scale sine amplitude; for int16, full scale = 32768)
    # This is a common convention; exact meaning depends on your signal chain.
    if PLOT_DBFS:
        fullscale = 32768.0
        mag_dbfs = 20.0 * np.log10(np.maximum(mag / fullscale, 1e-20))

    # Print some peaks (skip DC bin 0)
    if PEAKS > 0 and mag.size > 2:
        k0 = 1
        mags = mag[k0:].copy()
        idx = np.argpartition(mags, -PEAKS)[-PEAKS:]
        idx = idx[np.argsort(mags[idx])[::-1]]
        print("\nTop peaks (approx):")
        for i in idx:
            bin_i = i + k0
            print(f"  f={f[bin_i]/1e6:9.4f} MHz   mag={mag[bin_i]:.3f}"
                  + (f"   {mag_dbfs[bin_i]:.2f} dBFS" if PLOT_DBFS else ""))

    # Plot
    plt.figure(figsize=(11, 5))
    plt.plot(f / 1e6, mag, linewidth=0.8)
    plt.title(f"FFT magnitude (one-sided)  N={N}  Fs={FS_HZ/1e6:.2f} MHz  window={WINDOW}")
    plt.xlabel("Frequency [MHz]")
    plt.ylabel("Magnitude [counts]")
    plt.grid(True)
    plt.tight_layout()

    if PLOT_DBFS:
        plt.figure(figsize=(11, 5))
        plt.plot(f / 1e6, mag_dbfs, linewidth=0.8)
        plt.title(f"FFT magnitude (dBFS, one-sided)  N={N}  Fs={FS_HZ/1e6:.2f} MHz  window={WINDOW}")
        plt.xlabel("Frequency [MHz]")
        plt.ylabel("Magnitude [dBFS]")
        plt.grid(True)
        plt.tight_layout()

    plt.show()
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
