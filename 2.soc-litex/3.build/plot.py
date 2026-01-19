#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import numpy as np

FS_HZ_DEFAULT = 65e6
U16_LE = np.dtype("<u2")

def pick_interactive_backend():
    # Try Qt first, then Tk. If neither available, keep matplotlib default.
    try:
        import PyQt5  # noqa: F401
        return "Qt5Agg"
    except Exception:
        pass
    try:
        import tkinter  # noqa: F401
        return "TkAgg"
    except Exception:
        pass
    return None

def load_samples(path: str) -> np.ndarray:
    with open(path, "rb") as f:
        buf = f.read()
    u16 = np.frombuffer(buf, dtype=U16_LE)
    # reinterpret same bytes as signed int16
    s16 = u16.view(np.int16)
    return s16

def plot_time(samples: np.ndarray, fs_hz: float, decim: int, start: int, nsamp: int):
    import matplotlib.pyplot as plt

    if decim < 1:
        decim = 1
    if start < 0:
        start = 0
    if start >= samples.size:
        raise ValueError(f"--start {start} is beyond file length ({samples.size} samples)")

    if nsamp is None or nsamp <= 0:
        seg = samples[start:]
    else:
        seg = samples[start:start + nsamp]

    y = seg[::decim]
    t = (np.arange(y.size, dtype=np.float64) * decim) / fs_hz

    fig, ax = plt.subplots()
    ax.plot(t, y, linewidth=0.8)
    ax.set_title(f"{args.file}  (start={start}, nsamp={seg.size}, decim×{decim})  fs={fs_hz/1e6:.2f} MHz")
    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Sample (int16)")
    ax.grid(True)

def plot_fft(samples: np.ndarray, fs_hz: float, decim: int, start: int, nsamp: int):
    import matplotlib.pyplot as plt

    if decim < 1:
        decim = 1
    if start < 0:
        start = 0
    if start >= samples.size:
        raise ValueError(f"--start {start} is beyond file length ({samples.size} samples)")

    if nsamp is None or nsamp <= 0:
        seg = samples[start:]
    else:
        seg = samples[start:start + nsamp]

    x = seg[::decim].astype(np.float64)
    fs_eff = fs_hz / decim

    # remove DC to make spectrum nicer
    x -= np.mean(x)

    # window + rfft
    w = np.hanning(x.size)
    X = np.fft.rfft(x * w)
    f = np.fft.rfftfreq(x.size, d=1.0 / fs_eff)
    mag_db = 20.0 * np.log10(np.maximum(np.abs(X), 1e-12))

    fig, ax = plt.subplots()
    ax.plot(f, mag_db, linewidth=0.8)
    ax.set_title(f"FFT  (fs_eff={fs_eff/1e6:.3f} MHz, N={x.size})")
    ax.set_xlabel("Frequency [Hz]")
    ax.set_ylabel("Magnitude [dBFS-ish]")
    ax.grid(True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot an existing capture.bin")
    parser.add_argument("file", nargs="?", default="capture.bin", help="Input .bin file (default: capture.bin)")
    parser.add_argument("--fs", type=float, default=FS_HZ_DEFAULT, help="Sample rate in Hz (default: 65e6)")
    parser.add_argument("--decim", type=int, default=2000, help="Decimation factor for plotting (default: 2000)")
    parser.add_argument("--start", type=int, default=0, help="Start sample index (default: 0)")
    parser.add_argument("--nsamp", type=int, default=0, help="Number of samples to plot (0 = all from start)")
    parser.add_argument("--fft", action="store_true", help="Also plot FFT of the (decimated) segment")
    args = parser.parse_args()

    import matplotlib
    be = pick_interactive_backend()
    if be is not None:
        matplotlib.use(be)
    import matplotlib.pyplot as plt

    s16 = load_samples(args.file)
    print(f"loaded {args.file}: {s16.size} samples ({s16.size / args.fs:.6f} s @ {args.fs:.3f} Hz)")

    plot_time(s16, fs_hz=args.fs, decim=args.decim, start=args.start, nsamp=(args.nsamp or None))
    if args.fft:
        plot_fft(s16, fs_hz=args.fs, decim=args.decim, start=args.start, nsamp=(args.nsamp or None))

    plt.show()
