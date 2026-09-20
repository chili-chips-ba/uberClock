#!/usr/bin/env python3
"""Analyze test1/test2 logs using only the standard library.

Output goes to loopback_analysis beside test1/test2. No firmware is changed.
Test 1 drops seven acquisition records (the observed old-setting plateau).
Test 2 reports the last 50 records of each configuration segment, without
claiming that this guarantees fresh or contiguous input samples.
"""
import cmath
import csv
import math
from pathlib import Path
import re
import statistics

ROOT = Path(__file__).resolve().parent.parent
RECORD = re.compile(
    r"bb=(\(na\))?([-\d.]+)Hz dticks=(\d+) k=(\d+) "
    r"y=\{(-?\d+),(-?\d+),(-?\d+)\} den=(-?\d+)"
)


def ideal_fit(frequency, n, hann=False):
    """Double-precision, zero-phase real cosine, three-bin power parabola.

    This model excludes quantization, filtering, FIFO discontinuities and
    noise; agreement is evidence of a window, not proof of firmware identity.
    """
    samples = [math.cos(2 * math.pi * frequency * j / 10000) *
               (0.5 - 0.5 * math.cos(2 * math.pi * j / (n - 1)) if hann else 1)
               for j in range(n)]
    nearest = round(frequency * n / 10000)
    power = {k: abs(sum(x * cmath.exp(-2j * math.pi * k * j / n)
                        for j, x in enumerate(samples))) ** 2
             for k in range(nearest - 2, nearest + 3)}
    peak = max(range(nearest - 1, nearest + 2), key=power.get)
    a, b, c = (power[peak + offset] for offset in (-1, 0, 1))
    return (peak + 0.5 * (a - c) / (a - 2 * b + c)) * 10000 / n


def main():
    output = ROOT / "loopback_analysis"
    output.mkdir(exist_ok=True)
    summaries, records, models = [], [], []
    for path in sorted(ROOT.glob("test[12]/*.txt")):
        segments = []
        for line in path.read_text().splitlines():
            if line.startswith("trackq_start:"):
                n = int(re.search(r"\bN=(\d+)", line)[1])
                segments.append((n, []))
            match = RECORD.search(line)
            if match:
                if not segments:
                    raise ValueError(f"Missing configuration in {path}")
                na, bb, ticks, peak, y1, y2, y3, den = match.groups()
                segments[-1][1].append(dict(
                    valid=not bool(na), bb_hz=float(bb), dticks=int(ticks),
                    peak=int(peak), y1=int(y1), y2=int(y2), y3=int(y3), den=int(den)))
        for segment, (n, rows) in enumerate(segments, 1):
            if not rows:
                print(f"NOTE: {path.name} contains an extra N={n} start with no readings")
                continue
            down = int(path.stem) if path.parent.name == "test1" else 10323408
            expected = (10324440 - down) * 65000000 / 2**26
            start = 7 if path.parent.name == "test1" else max(0, len(rows) - 50)
            selected = [r for r in rows[start:] if r["valid"]]
            values = [r["bb_hz"] for r in selected]
            summary = dict(
                file=str(path.relative_to(ROOT)), segment=segment, fft_n=n,
                phase_nco_assumed=10324440, phase_down_assumed=down,
                total_records=len(rows), invalid_total=sum(not r["valid"] for r in rows),
                excluded_leading_records=start, used_records=len(values),
                expected_hz=expected, mean_hz=statistics.mean(values),
                bias_hz=statistics.mean(values)-expected,
                population_std_mhz=1000*statistics.pstdev(values),
                min_hz=min(values), max_hz=max(values),
                peak_bins=" ".join(str(k) for k in sorted({r["peak"] for r in selected})))
            summaries.append(summary)
            elapsed = 0
            for index, row in enumerate(rows):
                if index:
                    elapsed += row["dticks"] / 10000
                records.append(dict(file=summary["file"], segment=segment, fft_n=n,
                                    record=index+1, nominal_elapsed_s=elapsed,
                                    selected=index >= start and row["valid"], **row))
            if path.parent.name == "test1":
                models.append(dict(phase_down=down, expected_hz=expected,
                                   measured_mean_hz=summary["mean_hz"],
                                   rectangular_model_hz=ideal_fit(expected, n),
                                   hann_model_hz=ideal_fit(expected, n, hann=True)))
    for name, data in [("summary.csv", summaries), ("records.csv", records),
                       ("window_comparison.csv", models)]:
        with (output / name).open("w", newline="") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(data[0]))
            writer.writeheader()
            writer.writerows(data)
    for s in summaries:
        print(f"{s['file']:24s} N={s['fft_n']:4d} used={s['used_records']:3d} "
              f"mean={s['mean_hz']:.6f} bias={s['bias_hz']:+.6f} Hz "
              f"std={s['population_std_mhz']:.3f} mHz")
    print(f"CSV files written to {output}")


if __name__ == "__main__":
    main()
