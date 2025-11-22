import numpy as np
import matplotlib.pyplot as plt

# --- Configuration ---
filename = "hs.txt"  # your file with "index,value"
sample_rate = 65e6  # <-- CHANGE THIS to your actual sample rate in Hz

# --- Load values ---
values = np.loadtxt(filename, delimiter=",", usecols=1)

# --- Plot signal ---
plt.figure()
plt.plot(values)
plt.title("Time-domain signal")
plt.xlabel("Sample")
plt.ylabel("Amplitude")
plt.savefig("time_plot.png", dpi=150)
print("Saved time_plot.png")

# --- FFT to find frequency ---
n = len(values)
fft_vals = np.fft.fft(values)
fft_freqs = np.fft.fftfreq(n, d=1/sample_rate)

# Only keep positive frequencies
pos_mask = fft_freqs > 0
fft_vals = np.abs(fft_vals[pos_mask])
fft_freqs = fft_freqs[pos_mask]

# Find the peak frequency
peak_idx = np.argmax(fft_vals)
peak_freq = fft_freqs[peak_idx]

print(f"Estimated frequency: {peak_freq:.3f} Hz")

# --- Plot frequency spectrum ---
plt.figure()
plt.plot(fft_freqs, fft_vals)
plt.title("Frequency Spectrum")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude")
plt.xlim(0, sample_rate/2)
plt.savefig("freq_spectrum.png", dpi=150)
print("Saved freq_spectrum.png")
