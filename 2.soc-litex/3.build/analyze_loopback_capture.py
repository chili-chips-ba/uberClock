#!/usr/bin/env python3
"""Summarize loopback-fifo-v1 logs and fit any trackq_dump blocks.

Standard library only. The host sine fit is a diagnostic single-tone model,
not an independent clock reference or a replacement for acquisition checks.
Usage: python3 analyze_loopback_capture.py serial_log.txt
"""
import argparse
import cmath
from collections import defaultdict
import math
from pathlib import Path
import re
import statistics


def fft(x):
    if len(x) == 1:
        return [complex(x[0])]
    even, odd = fft(x[::2]), fft(x[1::2])
    twiddled = [cmath.exp(-2j * math.pi * k / len(x)) * y for k, y in enumerate(odd)]
    return [a + b for a, b in zip(even, twiddled)] + [a - b for a, b in zip(even, twiddled)]


def sine_residual(samples, fs, frequency):
    """Least-squares A*cos(wt)+B*sin(wt)+DC at a trial frequency."""
    cc = ss = cs = csum = ssum = yc = ys = 0.0
    cosines, sines = [], []
    for i, y in enumerate(samples):
        c, s = math.cos(2 * math.pi * frequency * i / fs), math.sin(2 * math.pi * frequency * i / fs)
        cosines.append(c)
        sines.append(s)
        cc += c*c
        ss += s*s
        cs += c*s
        csum += c
        ssum += s
        yc += y*c
        ys += y*s
    matrix = [[cc, cs, csum, yc], [cs, ss, ssum, ys],
              [csum, ssum, float(len(samples)), float(sum(samples))]]
    for column in range(3):
        pivot = max(range(column, 3), key=lambda r: abs(matrix[r][column]))
        matrix[column], matrix[pivot] = matrix[pivot], matrix[column]
        value = matrix[column][column]
        if abs(value) < 1e-12:
            return float("inf")
        matrix[column] = [v / value for v in matrix[column]]
        for row in range(3):
            if row != column:
                factor = matrix[row][column]
                matrix[row] = [a-factor*b for a, b in zip(matrix[row], matrix[column])]
    a, b, dc = [row[3] for row in matrix]
    return sum((y-a*c-b*s-dc)**2 for y, c, s in zip(samples, cosines, sines))


def fit_capture(samples, fs):
    n = len(samples)
    if n < 8 or n & (n-1):
        raise ValueError("Capture length must be a power of two, at least 8")
    if max(samples) == min(samples):
        raise ValueError("Constant capture: no sinusoidal frequency to estimate")
    spectrum = fft(samples)
    power = [abs(x)**2 for x in spectrum]
    peak = max(range(1, n//2), key=power.__getitem__)
    a, b, c = power[peak-1:peak+2]
    parabola = (peak + .5*(a-c)/(a-2*b+c)) * fs/n
    step = fs/n
    # Search around the independently detected spectral peak, not the known
    # register-derived frequency. A small grid avoids jumping to a sidelobe.
    grid = [max(step/1000, min(fs/2-step/1000, (peak-1+i/8)*step)) for i in range(17)]
    best = min(range(len(grid)), key=lambda i: sine_residual(samples, fs, grid[i]))
    lo, hi = grid[max(0, best-1)], grid[min(len(grid)-1, best+1)]
    ratio = (math.sqrt(5)-1)/2
    x1, x2 = hi-ratio*(hi-lo), lo+ratio*(hi-lo)
    r1, r2 = sine_residual(samples, fs, x1), sine_residual(samples, fs, x2)
    for _ in range(42):
        if r1 < r2:
            hi, x2, r2 = x2, x1, r1
            x1 = hi-ratio*(hi-lo)
            r1 = sine_residual(samples, fs, x1)
        else:
            lo, x1, r1 = x1, x2, r2
            x2 = lo+ratio*(hi-lo)
            r2 = sine_residual(samples, fs, x2)
    frequency = (lo+hi)/2
    return parabola, frequency, math.sqrt(sine_residual(samples, fs, frequency)/n)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    groups = defaultdict(list)
    captures, current = [], None
    failures = 0
    for line in args.log.read_text().splitlines():
        if "capture rejected:" in line or "timeout at sample" in line:
            failures += 1
            print(line)
        if "trackq validate:" in line:
            fields = dict(re.findall(r"(\w+)=([^\s]+)", line))
            if not all(k in fields for k in ("N", "pnco", "pdown", "input")):
                raise ValueError("Old log format: use the loopback-fifo-v1 firmware")
            key = tuple(int(fields[k]) for k in ("N", "pnco", "pdown", "input"))
            bb = fields["bb"]
            groups[key].append(None if bb.startswith("(na)") else float(bb.removesuffix("Hz")))
        if "trackq_dump begin:" in line:
            if current is not None:
                raise ValueError("Unfinished dump before another dump began")
            current = (dict(re.findall(r"(\w+)=([^\s]+)", line)), [])
        elif "trackq_dump end" in line and current is not None:
            metadata, samples = current
            if len(samples) != int(metadata["N"]):
                raise ValueError("Incomplete raw capture")
            captures.append(current)
            current = None
        elif current is not None and re.fullmatch(r"\d+,-?\d+", line.strip()):
            index, value = map(int, line.split(","))
            if index != len(current[1]):
                raise ValueError("Missing, duplicated or reordered UART sample row")
            current[1].append(value)
    if current is not None:
        raise ValueError("Raw dump has no end marker")
    print("N,pnco,pdown,input,valid,invalid,mean_hz,bias_hz,std_mhz")
    for (n, pnco, pdown, source), rows in groups.items():
        values = [x for x in rows if x is not None]
        if not values:
            print(f"{n},{pnco},{pdown},{source},0,{len(rows)},NA,NA,NA")
            continue
        expected = abs(pnco-pdown)*65000000/2**26
        mean = statistics.mean(values)
        bias = f"{mean-expected:+.6f}" if source == 1 and 0 < expected < 5000 else "NA"
        print(f"{n},{pnco},{pdown},{source},{len(values)},{len(rows)-len(values)},"
              f"{mean:.6f},{bias},{statistics.pstdev(values)*1000:.3f}")
    for number, (metadata, samples) in enumerate(captures, 1):
        parabola, fit, rms = fit_capture(samples, int(metadata["Fs"]))
        expected = abs(int(metadata["pnco"])-int(metadata["pdown"]))*65000000/2**26
        print(f"Dump {number}: float power-parabola={parabola:.6f} Hz, "
              f"sine fit={fit:.6f} Hz, residual RMS={rms:.3f} sample units")
        if metadata["input"] == "1" and 0 < expected < 5000:
            print(f"  nominal expected={expected:.6f} Hz, sine-fit error={fit-expected:+.6f} Hz")
    print(f"Capture rejection/timeout messages: {failures}")
    if not groups and not captures:
        raise ValueError("No new-format validation records or raw dumps found")


if __name__ == "__main__":
    main()
