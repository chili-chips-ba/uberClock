#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import numpy as np
import matplotlib.pyplot as plt

# ---------------------------------------------------------------------------
# Paste ub_hexdump output here
# ---------------------------------------------------------------------------
hexdump_text = """
a0000000: fe 00 a0 00 40 00 de ff 7e ff 20 ff c2 fe 68 fe
a0000010: 12 fe c0 fd 74 fd 2e fd ee fc b6 fc 86 fc 5e fc
a0000020: 28 fc 1a fc 18 fc 1c fc 2c fc 42 fc 62 fc 8c fc
a0000030: be fc f8 fc 3a fd 7e fd ce fd 20 fe 78 fe d0 fe
a0000040: 90 ff f0 ff 50 00 b0 00 0e 01 68 01 c2 01 16 02
a0000050: 66 02 ae 02 f2 02 2e 03 60 03 8e 03 b2 03 cc 03
a0000060: e6 03 e4 03 da 03 c8 03 ac 03 86 03 58 03 22 03
a0000070: e6 02 a4 02 58 02 08 02 b4 01 5a 01 fe 00 a0 00
a0000080: de ff 7e ff 20 ff c2 fe 68 fe 12 fe c0 fd 74 fd
a0000090: 2e fd ee fc b6 fc 86 fc 5e fc 3e fc 28 fc 1a fc
a00000a0: 1c fc 2c fc 42 fc 62 fc 8c fc c0 fc f8 fc 3a fd
a00000b0: 7e fd ce fd 20 fe 78 fe d0 fe 30 ff 90 ff f0 ff
a00000c0: ae 00 0e 01 68 01 c2 01 16 02 64 02 ae 02 f2 02
a00000d0: 2e 03 60 03 8e 03 b2 03 cc 03 dc 03 e6 03 e4 03
a00000e0: c8 03 ac 03 86 03 58 03 22 03 e6 02 a2 02 58 02
a00000f0: 08 02 b4 01 5a 01 fe 00 a0 00 40 00 de ff 7e ff
"""

def parse_hexdump_to_int16(text: str) -> np.ndarray:
    hex_bytes = re.findall(r"\b[0-9a-fA-F]{2}\b", text)
    if len(hex_bytes) % 2 != 0:
        raise ValueError("Odd number of hex bytes; cannot form 16-bit samples cleanly.")

    byte_vals = np.array([int(b, 16) for b in hex_bytes], dtype=np.uint8)
    samples = byte_vals.view('<i2')
    return samples

def save_samples_txt(samples: np.ndarray, path: str = "samples.txt") -> None:
    with open(path, "w") as f:
        for i, val in enumerate(samples):
            f.write(f"{i}\t{int(val)}\n")
    print(f"Saved {len(samples)} samples to {path}")

def plot_samples_time(samples: np.ndarray,
                      fs: float = 65e6,
                      path: str = "time_domain.png") -> None:
    n = np.arange(len(samples))
    t = n / fs
    t_us = t * 1e6

    plt.figure(figsize=(10, 4))
    plt.plot(t_us, samples, marker='.')
    plt.title(f"Samples vs Time (fs = {fs/1e6:.2f} MHz)")
    plt.xlabel("Time [µs]")
    plt.ylabel("Amplitude [LSB]")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(path, dpi=150)
    plt.close()
    print(f"Saved time-domain plot to {path}")

def plot_fft_and_estimate_freq(samples: np.ndarray,
                               fs: float = 65e6,
                               path: str = "fft.png") -> float:
    N = len(samples)
    x = samples - np.mean(samples)

    window = np.hanning(N)
    xw = x * window

    spec = np.fft.rfft(xw)
    freqs = np.fft.rfftfreq(N, d=1.0/fs)
    mag = np.abs(spec)

    dc_cut = 1
    peak_idx = np.argmax(mag[dc_cut:]) + dc_cut
    f_peak = freqs[peak_idx]

    mag_db = 20 * np.log10(mag / np.max(mag) + 1e-12)

    plt.figure(figsize=(10, 4))
    plt.plot(freqs/1e6, mag_db)
    plt.title(f"Magnitude Spectrum (N = {N}, fs = {fs/1e6:.2f} MHz)")
    plt.xlabel("Frequency [MHz]")
    plt.ylabel("Magnitude [dBFS]")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(path, dpi=150)
    plt.close()
    print(f"Saved FFT plot to {path}")

    rbw = fs / N
    print(f"FFT length: {N}")
    print(f"Frequency resolution (RBW): {rbw:.2f} Hz")
    print(f"Peak bin index: {peak_idx}")
    print(f"Estimated tone frequency: {f_peak:.3f} Hz ({f_peak/1e3:.3f} kHz)")

    return f_peak

def main():
    fs = 65e6

    samples = parse_hexdump_to_int16(hexdump_text)
    print("First 16 samples:", samples[:16])

    save_samples_txt(samples, "samples.txt")
    plot_samples_time(samples, fs=fs, path="time_domain.png")
    f_est = plot_fft_and_estimate_freq(samples, fs=fs, path="fft.png")

    print(f"\n>>> Estimated signal frequency: {f_est/1e3:.3f} kHz")

if __name__ == "__main__":
    main()
