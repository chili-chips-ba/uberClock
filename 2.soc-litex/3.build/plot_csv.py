# save as plot_csv.py
# usage:
#   python3 plot_csv.py hs.csv even         # even indices, all samples
#   python3 plot_csv.py hs.csv odd          # odd indices, all samples
#   python3 plot_csv.py hs.csv even 500     # even indices, last 500 samples
#   python3 plot_csv.py hs.csv odd  200     # odd indices, last 200 samples

import sys
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} <csvfile> <even|odd> [N_last_samples]")
    sys.exit(1)

csv_file = sys.argv[1]
mode = sys.argv[2].lower()
N = None if len(sys.argv) < 4 else int(sys.argv[3])

# Load CSV
arr = np.loadtxt(csv_file, delimiter=",", dtype=np.int32)

# Select even or odd indices
if mode == "even":
    arr = arr[0::2, :]
elif mode == "odd":
    arr = arr[1::2, :]
else:
    print("Error: mode must be 'even' or 'odd'")
    sys.exit(1)

# Keep only the last N samples if specified
if N is not None:
    arr = arr[-N:, :]

# Split into t (index) and x (value)
t = arr[:, 0]
x = arr[:, 1]

# Plot
plt.figure()
plt.plot(t, x)
plt.xlabel("sample")
plt.ylabel("value")
plt.title(f"{csv_file} ({mode} indices, {len(t)} samples)")
plt.savefig("plot.png", dpi=150)
print(f"Saved plot.png with {len(t)} samples ({mode} indices)")
