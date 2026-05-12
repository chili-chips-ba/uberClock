#!/usr/bin/env python3
import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


DEFAULT_FS_HZ = 65_000_000.0
N_FFT = 1 << 26
MAX_TIME_PLOT_POINTS = 200_000
MAX_FREQ_PLOT_POINTS = 200_000


def load_signal(path: Path, n_fft: int) -> np.ndarray:
    raw = np.fromfile(path, dtype="<i2")
    if raw.size >= n_fft:
        x = raw[:n_fft].astype(np.float32, copy=False)
    else:
        x = np.zeros(n_fft, dtype=np.float32)
        x[: raw.size] = raw.astype(np.float32, copy=False)
    return x


def decimate_for_plot(x: np.ndarray, max_points: int) -> tuple[np.ndarray, np.ndarray]:
    if x.size <= max_points:
        idx = np.arange(x.size)
        return idx, x

    step = int(np.ceil(x.size / max_points))
    idx = np.arange(0, x.size, step)
    return idx, x[idx]


def compute_rfft(x: np.ndarray, fs_hz: float) -> tuple[np.ndarray, np.ndarray]:
    try:
        import scipy.fft as sp_fft  # type: ignore

        spec = sp_fft.rfft(x)
        freqs = sp_fft.rfftfreq(x.size, d=1.0 / fs_hz)
    except Exception:
        spec = np.fft.rfft(x)
        freqs = np.fft.rfftfreq(x.size, d=1.0 / fs_hz)
    return freqs, spec


def make_time_plot(x: np.ndarray, fs_hz: float) -> tuple[plt.Figure, plt.Axes]:
    idx, y = decimate_for_plot(x, MAX_TIME_PLOT_POINTS)
    t_us = idx / fs_hz * 1e6

    fig, ax = plt.subplots(figsize=(12, 5), constrained_layout=True)
    ax.plot(t_us, y, linewidth=0.7)
    ax.set_title(f"Time Domain ({x.size} samples, Fs={fs_hz/1e6:.1f} MHz)")
    ax.set_xlabel("Time (us)")
    ax.set_ylabel("Amplitude (counts)")
    ax.grid(True, alpha=0.25)
    return fig, ax


def make_fft_plot(freqs: np.ndarray, spec: np.ndarray, fs_hz: float) -> tuple[plt.Figure, plt.Axes]:
    mag = np.abs(spec).astype(np.float32, copy=False)
    mag_db = 20.0 * np.log10(np.maximum(mag, 1e-12))

    idx, y = decimate_for_plot(mag_db, MAX_FREQ_PLOT_POINTS)
    f_mhz = freqs[idx] / 1e6

    fig, ax = plt.subplots(figsize=(12, 5), constrained_layout=True)
    ax.plot(f_mhz, y, linewidth=0.7)
    ax.set_title(f"FFT Magnitude ({freqs.size} bins, Fs={fs_hz/1e6:.1f} MHz)")
    ax.set_xlabel("Frequency (MHz)")
    ax.set_ylabel("Magnitude (dB, arbitrary)")
    ax.grid(True, alpha=0.25)
    return fig, ax


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Analyze captured UberClock samples: force length to 2^26, compute FFT, and save plots."
    )
    ap.add_argument("input", help="raw input file, little-endian int16 samples")
    ap.add_argument("--fs", type=float, default=DEFAULT_FS_HZ, help="sample rate in Hz (default: 65000000)")
    ap.add_argument("--nfft", type=int, default=N_FFT, help="FFT length / padded length (default: 2^26)")
    ap.add_argument("--prefix", default="", help="output filename prefix")
    ap.add_argument("--show", action="store_true", help="show interactive figures")
    ap.add_argument("--no-save", action="store_true", help="do not write PNG files")
    args = ap.parse_args()

    fs_hz = float(args.fs)

    in_path = Path(args.input)
    prefix = args.prefix or in_path.stem
    time_plot = in_path.with_name(f"{prefix}_time.png")
    fft_plot = in_path.with_name(f"{prefix}_fft.png")

    raw = np.fromfile(in_path, dtype="<i2")
    print(f"Loaded {raw.size} samples from {in_path}")
    if raw.size < args.nfft:
        print(f"Zero-padding from {raw.size} to {args.nfft} samples")
    elif raw.size > args.nfft:
        print(f"Trimming from {raw.size} to {args.nfft} samples")

    x = load_signal(in_path, args.nfft)

    print("Preparing time-domain plot...")
    time_fig, _ = make_time_plot(x, fs_hz)

    print("Computing FFT...")
    freqs, spec = compute_rfft(x, fs_hz)

    print("Preparing FFT plot...")
    fft_fig, _ = make_fft_plot(freqs, spec, fs_hz)

    if not args.no_save:
        print("Saving plots...")
        time_fig.savefig(time_plot, dpi=150)
        fft_fig.savefig(fft_plot, dpi=150)
        print(f"Wrote {time_plot}")
        print(f"Wrote {fft_plot}")

    if args.show:
        plt.show()
    else:
        plt.close(time_fig)
        plt.close(fft_fig)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
