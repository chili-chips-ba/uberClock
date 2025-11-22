#!/usr/bin/env python3
import sys, csv, os
import matplotlib.pyplot as plt

if len(sys.argv) != 2:
    print("Usage: python3 plot_simple.py samples.txt")
    sys.exit(1)

path = sys.argv[1]
base = os.path.splitext(path)[0]  # "samples" if file is "samples.txt"
idx, xs, ys = [], [], []

with open(path, "r", newline="") as f:
    # ignore comment lines starting with '#'
    r = csv.reader((ln for ln in f if not ln.lstrip().startswith("#")), delimiter=",")
    for row in r:
        if len(row) < 3:
            continue
        try:
            i = int(row[0].strip())
            x = float(row[1].strip())
            y = float(row[2].strip())
        except ValueError:
            continue
        idx.append(i); xs.append(x); ys.append(y)

if not idx:
    print("No data parsed. Check file format.")
    sys.exit(1)

# 1) time-domain plot
plt.figure()
plt.plot(idx, xs, label="X")
plt.plot(idx, ys, label="Y")
plt.title("I/Q vs Sample")
plt.xlabel("Sample index")
plt.ylabel("Amplitude")
plt.legend()
plt.tight_layout()
time_path = f"{base}_time.png"
plt.savefig(time_path, dpi=150)
print(f"Saved: {time_path}")

# 2) Lissajous plot
plt.figure()
plt.plot(xs, ys)
plt.title("Lissajous (Y vs X)")
plt.xlabel("X")
plt.ylabel("Y")
plt.axis("equal")
plt.tight_layout()
xy_path = f"{base}_xy.png"
plt.savefig(xy_path, dpi=150)
print(f"Saved: {xy_path}")

plt.show()
