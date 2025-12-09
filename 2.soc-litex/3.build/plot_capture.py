
#!/usr/bin/env python3
import sys
import numpy as np
import matplotlib.pyplot as plt

def load_csv(path):
    """Load idx,value CSV into numpy arrays."""
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
    if not (2 <= len(sys.argv) <= 3):
        print("Usage: python3 plot_capture.py samples.csv [Fs_Hz]")
        print("  default Fs = 10000 Hz")
        sys.exit(1)

    path = sys.argv[1]
    if len(sys.argv) == 3:
        Fs = float(sys.argv[2])
    else:
        Fs = 10_000.0  # default 10 kHz

    idx, x = load_csv(path)
    N = len(x)
    print(f"Loaded {N} samples, Fs = {Fs} Hz")

    # --- Time vector ---
    t = np.arange(N) / Fs

    # --- Time-domain plot ---
    plt.figure()
    plt.step(t, x)
    plt.xlabel("Time [s]")
    plt.ylabel("Amplitude")
    plt.title("Signal vs Time")
    plt.grid(True)

    # --- FFT (rfft, linear scale) ---
    # optional: remove DC offset to make spectrum nicer
    x_zeromean = x - np.mean(x)
    X = np.fft.rfft(x_zeromean)
    freqs = np.fft.rfftfreq(N, d=1.0/Fs)
    mag = np.abs(X)

    plt.figure()
    markerline, stemlines, baseline = plt.stem(freqs, mag, basefmt=" ")
    plt.xlabel("Frequency [Hz]")
    plt.ylabel("Magnitude (linear)")
    plt.title("FFT Magnitude")
    plt.grid(True)

    plt.show()

if __name__ == "__main__":
    main()
