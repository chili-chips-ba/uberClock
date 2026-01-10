#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt

F_S = 65e6          # 65 MHz sampling rate
N_PLOT = 512        # samples to plot
FNAME = "capture.bin"

# Load binary data (int16 samples)
x = np.fromfile(FNAME, dtype=np.int16)

if len(x) < N_PLOT:
    raise RuntimeError(f"File has only {len(x)} samples")

# Take last 512 samples
y = x[-N_PLOT:]

# Time axis (seconds -> nanoseconds)
t = np.arange(N_PLOT) / F_S * 1e9

# Plot
plt.figure(figsize=(10, 4))
plt.plot(t, y, marker="o", markersize=3)
plt.xlabel("Time [ns]")
plt.ylabel("Amplitude (int16)")
plt.title("Last 512 samples of nco_cos (Fs = 65 MHz)")
plt.grid(True)
plt.tight_layout()
plt.show()

