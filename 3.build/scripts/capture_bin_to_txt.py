# SPDX-FileCopyrightText: 2026 Ahmed Imamović
# SPDX-FileCopyrightText: 2026 Tarik Hamedović
# SPDX-License-Identifier: GPL-3.0-or-later

import numpy as np

x = np.fromfile("capture.bin", dtype=np.int16)

np.savetxt(
    "capture.txt",
    x,
    fmt="%d"
)

print(len(x), "samples written")

