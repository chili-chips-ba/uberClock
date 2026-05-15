# SPDX-FilecopyrightText:2026
# Ahmed Imamović Tarik Hamedović
# SPDX-License-Identifier:
# APGL-3.0-or-later

import numpy as np

x = np.fromfile("capture.bin", dtype=np.int16)

np.savetxt(
    "capture.txt",
    x,
    fmt="%d"
)

print(len(x), "samples written")

