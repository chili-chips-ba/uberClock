#!/usr/bin/env python3
"""Summarize and plot a multi-setting loopback-fifo-v1 sweep log.

Parses live "trackq validate: ..." lines (one register setting held for many
consecutive samples, then changed - e.g. a phase_down_1 sweep), groups by
(N, pnco, pdown, input), and produces both a stdout summary table (in the
same spirit as analyze_loopback_capture.py) and three diagnostic plots:

  1. bb (and the register-derived expected value) vs sample index, with
     each register-setting segment shaded and labeled.
  2. Bias (mean bb - expected) per setting.
  3. Jitter (population std dev) per setting.
  4. |den|/y2 (curvature-to-peak-power ratio) per setting - the scalloping-
     loss diagnostic used throughout this investigation.

Requires matplotlib (already a project dependency, see plot_meas_bb.py).
Usage: python3 analyze_sweep_capture.py serial_log.txt [--save prefix]
"""
import argparse
import re
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

LINE_RE = re.compile(r"trackq validate:.*")
FIELD_RE = re.compile(r"(\w+)=([^\s]+)")
Y_RE = re.compile(r"\{(-?\d+),(-?\d+),(-?\d+)\}")


def parse(path):
    """Returns a list of per-sample dicts, in file order."""
    rows = []
    for line in Path(path).read_text(errors="replace").splitlines():
        if "trackq validate:" not in line:
            continue
        # tolerate garbled terminal echo before the marker (backspace/redraw
        # artifacts seen in real captures) by slicing from the marker on
        line = line[line.index("trackq validate:"):]
        fields = dict(FIELD_RE.findall(line))
        if "bb" not in fields or "pdown" not in fields:
            continue
        bb_raw = fields["bb"]
        valid = not bb_raw.startswith("(na)")
        bb = None if not valid else float(bb_raw.removesuffix("Hz"))
        expected = None
        if "expected" in fields:
            exp_raw = fields["expected"]
            if not exp_raw.startswith("(na)"):
                expected = float(exp_raw.removesuffix("Hz"))
        y1 = y2 = y3 = den = None
        m = Y_RE.search(line)
        if m:
            y1, y2, y3 = (int(g) for g in m.groups())
        if "den" in fields:
            den = int(fields["den"])
        rows.append({
            "bb": bb, "valid": valid, "expected": expected,
            "pdown": fields.get("pdown"), "pnco": fields.get("pnco"),
            "N": fields.get("N"), "input": fields.get("input"),
            "y1": y1, "y2": y2, "y3": y3, "den": den,
        })
    return rows


def group_key(row):
    return (row["N"], row["pnco"], row["pdown"], row["input"])


def summarize(rows):
    groups = defaultdict(list)
    order = []
    for r in rows:
        k = group_key(r)
        if k not in groups:
            order.append(k)
        groups[k].append(r)

    print(f"{'N':>6}{'pnco':>12}{'pdown':>12}{'input':>7}{'valid':>7}{'invalid':>9}"
          f"{'mean_hz':>13}{'bias_hz':>11}{'std_mhz':>10}{'mean|den|/y2':>14}")
    summary = []
    for k in order:
        N, pnco, pdown, inp = k
        rs = groups[k]
        bbs = [r["bb"] for r in rs if r["valid"]]
        invalid = len(rs) - len(bbs)
        exp_vals = [r["expected"] for r in rs if r["expected"] is not None]
        expected = statistics.mean(exp_vals) if exp_vals else None
        ratios = [abs(r["den"]) / r["y2"] for r in rs
                  if r["valid"] and r["den"] and r["y2"]]
        mean_ratio = statistics.mean(ratios) if ratios else None
        if bbs:
            mean_hz = statistics.mean(bbs)
            std_mhz = statistics.pstdev(bbs) * 1000
            bias = mean_hz - expected if expected is not None else None
        else:
            mean_hz = std_mhz = bias = None
        summary.append({
            "N": N, "pnco": pnco, "pdown": pdown, "input": inp,
            "n_valid": len(bbs), "n_invalid": invalid,
            "mean_hz": mean_hz, "bias_hz": bias, "std_mhz": std_mhz,
            "expected": expected, "mean_ratio": mean_ratio,
        })
        def fmt(v, spec):
            return f"{v:{spec}}" if v is not None else "NA"
        print(f"{N:>6}{pnco:>12}{pdown:>12}{inp:>7}{len(bbs):>7}{invalid:>9}"
              f"{fmt(mean_hz, '13.4f')}{fmt(bias, '11.4f')}{fmt(std_mhz, '10.3f')}"
              f"{fmt(mean_ratio, '14.3f')}")
    return summary


def make_plots(rows, summary, save_prefix):
    labels = [f"{s['pdown']}\n(offset)" for s in summary]

    # 1. Full time series, segments shaded
    fig, ax = plt.subplots(figsize=(11, 4.5))
    idx = 0
    boundaries = []
    for s in summary:
        n = s["n_valid"] + s["n_invalid"]
        boundaries.append(idx)
        idx += n
    boundaries.append(idx)
    bb_series = [r["bb"] if r["valid"] else float("nan") for r in rows]
    ax.plot(bb_series, linewidth=1, color="#1f77b4", label="bb (measured)")
    exp_series = [r["expected"] for r in rows]
    ax.plot(exp_series, linewidth=1, linestyle="--", color="#d62728", label="expected")
    for b in boundaries:
        ax.axvline(b, color="grey", alpha=0.3, linewidth=0.8)
    for s, x0, x1 in zip(summary, boundaries[:-1], boundaries[1:]):
        ax.text((x0 + x1) / 2, ax.get_ylim()[1], s["pdown"], ha="center",
                va="bottom", fontsize=7, rotation=0)
    ax.set_xlabel("sample index")
    ax.set_ylabel("Hz")
    ax.set_title("bb vs expected across sweep, by pdown segment")
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    if save_prefix:
        fig.savefig(f"{save_prefix}_timeseries.png", dpi=150)

    # 2. Bias per setting
    fig, ax = plt.subplots(figsize=(8, 4))
    biases = [s["bias_hz"] if s["bias_hz"] is not None else 0 for s in summary]
    ax.bar(labels, biases, color="#2ca02c")
    ax.axhline(0, color="black", linewidth=0.8)
    ax.set_ylabel("bias (Hz)")
    ax.set_title("Mean bias vs pdown setting")
    ax.grid(alpha=0.3, axis="y")
    fig.tight_layout()
    if save_prefix:
        fig.savefig(f"{save_prefix}_bias.png", dpi=150)

    # 3. Jitter per setting
    fig, ax = plt.subplots(figsize=(8, 4))
    stds = [s["std_mhz"] if s["std_mhz"] is not None else 0 for s in summary]
    ax.bar(labels, stds, color="#ff7f0e")
    ax.set_ylabel("jitter, std dev (mHz)")
    ax.set_title("Jitter vs pdown setting")
    ax.grid(alpha=0.3, axis="y")
    fig.tight_layout()
    if save_prefix:
        fig.savefig(f"{save_prefix}_jitter.png", dpi=150)

    # 4. Scalloping-loss diagnostic
    fig, ax = plt.subplots(figsize=(8, 4))
    ratios = [s["mean_ratio"] if s["mean_ratio"] is not None else 0 for s in summary]
    ax.bar(labels, ratios, color="#9467bd")
    ax.set_ylabel("mean |den| / y2")
    ax.set_title("Curvature-to-peak-power ratio vs pdown setting\n(low value = near a bin boundary)")
    ax.grid(alpha=0.3, axis="y")
    fig.tight_layout()
    if save_prefix:
        fig.savefig(f"{save_prefix}_scalloping.png", dpi=150)

    if not save_prefix:
        plt.show()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("log", type=Path)
    ap.add_argument("--save", default=None,
                     help="save PNGs with this filename prefix instead of showing them")
    args = ap.parse_args()

    rows = parse(args.log)
    if not rows:
        raise SystemExit("No 'trackq validate:' lines found - old log format or empty file")
    summary = summarize(rows)
    make_plots(rows, summary, args.save)


if __name__ == "__main__":
    main()
