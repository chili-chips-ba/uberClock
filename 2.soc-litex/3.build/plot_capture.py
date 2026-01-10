#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
plot_capture.py — plot a waveform from capture.bin produced by your UberClock UDP capture.

Key points:
- Your file is 16 parallel u16 lanes per "beat" (32 bytes = 16 * 2 bytes).
- If you plot the raw interleaved stream, it will look jagged.
- Correct plotting: reshape to (beats, lanes) and choose a lane (or combine lanes).

Examples:
  ./plot_capture.py capture.bin --lane 0
  ./plot_capture.py capture.bin --lane 3 --decim 16
  ./plot_capture.py capture.bin --combine mean
  ./plot_capture.py capture.bin --lane 0 --fft
"""

import argparse
import numpy as np


BEAT_BYTES = 32
LANES = 16
U16_LE = np.dtype("<u2")


def pick_interactive_backend():
    try:
        import PyQt5  # noqa
        return "Qt5Agg"
    except Exception:
        pass
    try:
        import tkinter  # noqa
        return "TkAgg"
    except Exception:
        pass
    return None


def load_capture(path: str):
    buf = np.fromfile(path, dtype=np.uint8)
    if buf.size % BEAT_BYTES != 0:
        raise SystemExit(
            f"{path}: size {buf.size} not a multiple of BEAT_BYTES={BEAT_BYTES}. "
            "File may be truncated or beat size is different."
        )
    beats = buf.size // BEAT_BYTES
    u16 = np.frombuffer(buf.tobytes(), dtype=U16_LE, count=beats * LANES)
    lanes = u16.reshape(beats, LANES)
    return lanes  # shape: (beats, 16), dtype: u16


def lane_to_signed(x_u16: np.ndarray) -> np.ndarray:
    # u16 -> signed centered at midscale, matches typical ADC packing
    return (x_u16.astype(np.int32) - 32768).astype(np.int16)


def build_time_axis(n: int, fs_lane_hz: float) -> np.ndarray:
    return np.arange(n, dtype=np.float64) / fs_lane_hz


def main():
    ap = argparse.ArgumentParser(description="Plot waveform from capture.bin (16-lane u16 beats).")
    ap.add_argument("file", help="Path to capture.bin")
    ap.add_argument("--fs", type=float, default=65e6, help="Clock/sample rate BEFORE lane split (default: 65e6)")
    ap.add_argument("--lanes", type=int, default=LANES, help="Number of lanes (default: 16)")
    ap.add_argument("--lane", type=int, default=0, help="Lane index to plot (default: 0)")
    ap.add_argument(
        "--combine",
        choices=["none", "mean", "sum", "interleave"],
        default="none",
        help=(
            "How to form a single waveform:\n"
            "  none      = plot selected lane\n"
            "  mean/sum  = combine all lanes per beat\n"
            "  interleave= reconstruct a single fast stream by interleaving lanes (rarely what you want)"
        ),
    )
    ap.add_argument("--decim", type=int, default=1, help="Decimate for plotting (default: 1)")
    ap.add_argument("--seconds", type=float, default=None, help="Plot only first N seconds")
    ap.add_argument("--fft", action="store_true", help="Also show magnitude FFT of the plotted signal")
    args = ap.parse_args()

    lanes = load_capture(args.file)  # (beats, 16) u16
    beats, L = lanes.shape
    if args.lanes != L:
        # If user passes a different lanes count, you likely want to reshape differently.
        # Keep it simple: just warn and proceed with what file says.
        print(f"Note: file decodes as {L} lanes; ignoring --lanes={args.lanes}")

    if not (0 <= args.lane < L):
        raise SystemExit(f"--lane must be in [0, {L-1}]")

    # Build the waveform
    if args.combine == "none":
        y_u16 = lanes[:, args.lane]
        y = lane_to_signed(y_u16).astype(np.float64)
        fs_plot = args.fs / L  # each lane is fs/L samples per second
        label = f"lane {args.lane} (signed int16 view), fs={fs_plot/1e6:.6f} MHz"
    elif args.combine in ("mean", "sum"):
        y_all = lane_to_signed(lanes).astype(np.float64)  # (beats, lanes)
        if args.combine == "mean":
            y = y_all.mean(axis=1)
            label = f"mean({L} lanes), fs={args.fs/L/1e6:.6f} MHz"
        else:
            y = y_all.sum(axis=1)
            label = f"sum({L} lanes), fs={args.fs/L/1e6:.6f} MHz"
        fs_plot = args.fs / L
    else:  # interleave
        # Interleave lanes to reconstruct a single high-rate stream:
        # sample order within each beat: lane0..lane15, then next beat...
        y_all = lane_to_signed(lanes).astype(np.float64)
        y = y_all.reshape(beats * L)
        fs_plot = args.fs
        label = f"interleaved lanes -> fs={fs_plot/1e6:.3f} MHz"

    # Optionally limit duration
    if args.seconds is not None:
        nmax = int(np.floor(args.seconds * fs_plot))
        nmax = max(0, min(nmax, y.size))
        y = y[:nmax]

    # Decimation for plotting
    decim = max(1, int(args.decim))
    y_plot = y[::decim]
    t = build_time_axis(y_plot.size, fs_plot / decim)

    # Plot
    import matplotlib
    be = pick_interactive_backend()
    if be is not None:
        matplotlib.use(be)
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots()
    ax.plot(t, y_plot, linewidth=0.8)
    ax.set_title(f"{args.file} — {label} — decim×{decim}")
    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Amplitude")
    ax.grid(True)

    if args.fft and y.size > 0:
        # FFT on the non-decimated (or limited) signal for better frequency accuracy
        # Remove DC to make the spectrum cleaner
        yy = y - np.mean(y)
        # Use a power-of-two-ish size for speed (optional)
        n = int(2 ** np.floor(np.log2(yy.size))) if yy.size >= 8 else yy.size
        yy = yy[:n]
        win = np.hanning(n)
        Y = np.fft.rfft(yy * win)
        f = np.fft.rfftfreq(n, d=1.0 / fs_plot)
        mag = 20 * np.log10(np.maximum(np.abs(Y), 1e-12))

        fig2, ax2 = plt.subplots()
        ax2.plot(f, mag, linewidth=0.8)
        ax2.set_title(f"FFT magnitude — fs={fs_plot/1e6:.6f} MHz, N={n}")
        ax2.set_xlabel("Frequency [Hz]")
        ax2.set_ylabel("Magnitude [dBFS-ish]")
        ax2.grid(True)

    plt.show()


if __name__ == "__main__":
    main()

