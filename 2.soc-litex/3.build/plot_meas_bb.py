#!/usr/bin/env python3
import re
import sys

import matplotlib.pyplot as plt
import numpy as np


BB_RE = re.compile(r"\bbb=([+-]?\d+(?:\.\d+)?)Hz\b")


def load_bb_values(path):
    bb_values = []
    with open(path, "r", encoding="utf-8") as f:
        for line_no, line in enumerate(f, start=1):
            match = BB_RE.search(line)
            if not match:
                continue
            try:
                bb_values.append(float(match.group(1)))
            except ValueError as exc:
                raise RuntimeError(f"Invalid bb value on line {line_no}") from exc

    if not bb_values:
        raise RuntimeError("No bb values found. Expect lines like: bb=1000.823Hz")

    return np.asarray(bb_values, dtype=np.float64)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "meas_val.txt"
    bb = load_bb_values(path)
    t = np.arange(bb.size, dtype=np.float64)

    plt.figure(figsize=(10, 4.5))
    plt.plot(t, bb, linewidth=1.2)
    plt.title(f"BB Value vs Time ({bb.size} seconds)")
    plt.xlabel("Time [s]")
    plt.ylabel("BB [Hz]")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
