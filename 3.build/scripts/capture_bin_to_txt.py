import numpy as np

x = np.fromfile("capture.bin", dtype=np.int16)

np.savetxt(
    "capture.txt",
    x,
    fmt="%d"
)

print(len(x), "samples written")

