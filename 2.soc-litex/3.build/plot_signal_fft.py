#!/usr/bin/env python3
import sys
import numpy as np
import matplotlib.pyplot as plt

FS = 10_000.0  # sampling rate (Hz)

# ----- labeling knobs -----
MIN_HEIGHT_FRAC = 0.01   # label peaks >= 1% of max amplitude
MIN_SPACING_HZ  = 0.0    # set >0 to prevent labeling peaks too close
MAX_LABELS      = None   # e.g. 50 to cap labels, or None for unlimited
# --------------------------

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
                i = int(parts[0].strip()); v = float(parts[1].strip())
            except ValueError:
                continue
            idx.append(i); val.append(v)
    if not val:
        raise RuntimeError("No data parsed. Expect lines like: 0,123")
    return np.asarray(idx, dtype=np.int64), np.asarray(val, dtype=np.float64)

def find_peaks_by_threshold(freqs, mag, min_height_frac=0.01, min_spacing_hz=0.0,
                            exclude_dc=True, max_labels=None):
    """Return indices of all local maxima with height >= min_height_frac * max(mag)."""
    # local maxima
    locs = np.where((mag[1:-1] > mag[:-2]) & (mag[1:-1] > mag[2:]))[0] + 1
    if exclude_dc:
        locs = locs[freqs[locs] > 0.0]

    # threshold by height
    thresh = np.max(mag) * float(min_height_frac)
    locs = locs[mag[locs] >= thresh]

    # sort by frequency (for deterministic labeling order)
    locs = np.sort(locs)

    # enforce spacing if requested
    if min_spacing_hz > 0.0 and locs.size:
        selected = [locs[0]]
        for k in locs[1:]:
            if all(abs(freqs[k] - freqs[j]) >= min_spacing_hz for j in selected):
                selected.append(k)
        locs = np.array(selected, dtype=int)

    # optionally cap label count by amplitude (keep strongest)
    if (max_labels is not None) and (locs.size > max_labels):
        order = np.argsort(mag[locs])[::-1]   # by amplitude desc
        locs = locs[order[:max_labels]]
        locs = np.sort(locs)                  # plot left→right
    return locs

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 plot_signal_fft_linear_label_all.py samples.csv")
        sys.exit(1)

    path = sys.argv[1]
    idx, x = load_csv(path)
    N = len(x)
    t = np.arange(N) / FS

    # --- Time domain ---
    plt.figure()
    plt.step(t, x)
    plt.title("Signal vs Time")
    plt.xlabel("Time [s]"); plt.ylabel("Amplitude")
    plt.grid(True)

    # --- FFT (Hann window, single-sided, linear amplitude) ---
    win = np.hanning(N); xw = x * win
    X = np.fft.rfft(xw)
    freqs = np.fft.rfftfreq(N, d=1.0/FS)

    CG = 0.5  # Hann coherent gain
    mag = np.abs(X) * (2.0 / (N * CG))
    if N % 2 == 0:
        mag[0] *= 0.5; mag[-1] *= 0.5
    else:
        mag[0] *= 0.5

    # --- Peaks: label ALL above threshold ---
    peak_idx = find_peaks_by_threshold(
        freqs, mag,
        min_height_frac=MIN_HEIGHT_FRAC,
        min_spacing_hz=MIN_SPACING_HZ,
        exclude_dc=True,
        max_labels=MAX_LABELS
    )

    # --- Stem plot ---
    plt.figure()
    markerline, stemlines, baseline = plt.stem(freqs, mag)
    try: baseline.set_linewidth(0.8)
    except Exception: pass

    # emphasize + annotate peaks
    if peak_idx.size:
        plt.stem(freqs[peak_idx], mag[peak_idx])
        for k in peak_idx:
            plt.annotate(f"{mag[k]:.2f}",
                         xy=(freqs[k], mag[k]),
                         xytext=(0, 6),
                         textcoords="offset points",
                         ha="center", va="bottom", fontsize=8)

    plt.title("Single-Sided FFT Magnitude (Linear Scale)")
    plt.xlabel("Frequency [Hz]"); plt.ylabel("Amplitude")
    plt.grid(True, alpha=0.3); plt.xlim(0, FS/2)
    plt.tight_layout(); plt.show()

if __name__ == "__main__":
    main()

