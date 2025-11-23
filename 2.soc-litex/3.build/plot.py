import re
import numpy as np
import matplotlib.pyplot as plt


hexdump_text = """
a0000000: 00 00 01 00 02 00 03 00 04 00 05 00 06 00 07 00
a0000010: 08 00 09 00 0a 00 0b 00 0c 00 0d 00 0e 00 0f 00
a0000020: 10 00 11 00 12 00 13 00 14 00 15 00 16 00 17 00
a0000030: 18 00 19 00 1a 00 1b 00 1c 00 1d 00 1e 00 1f 00
a0000040: 20 00 21 00 22 00 23 00 24 00 25 00 26 00 27 00
a0000050: 28 00 29 00 2a 00 2b 00 2c 00 2d 00 2e 00 2f 00
a0000060: 30 00 31 00 32 00 33 00 34 00 35 00 36 00 37 00
a0000070: 38 00 39 00 3a 00 3b 00 3c 00 3d 00 3e 00 3f 00
a0000080: 40 00 41 00 42 00 43 00 44 00 45 00 46 00 47 00
a0000090: 48 00 49 00 4a 00 4b 00 4c 00 4d 00 4e 00 4f 00
a00000a0: 50 00 51 00 52 00 53 00 54 00 55 00 56 00 57 00
a00000b0: 58 00 59 00 5a 00 5b 00 5c 00 5d 00 5e 00 5f 00
a00000c0: 60 00 61 00 62 00 63 00 64 00 65 00 66 00 67 00
a00000d0: 68 00 69 00 6a 00 6b 00 6c 00 6d 00 6e 00 6f 00
a00000e0: 70 00 71 00 72 00 73 00 74 00 75 00 76 00 77 00
a00000f0: 78 00 79 00 7a 00 7b 00 7c 00 7d 00 7e 00 7f 00
"""


hex_bytes = re.findall(r"\b[0-9a-fA-F]{2}\b", hexdump_text)
byte_vals = np.array([int(b, 16) for b in hex_bytes], dtype=np.uint8)


samples = byte_vals.view(np.uint16)


with open("samples.txt", "w") as f:
    for i, val in enumerate(samples):
        f.write(f"{i}\t{val}\n")

print(f"Saved {len(samples)} samples to samples.txt")


plt.figure(figsize=(10, 4))
plt.plot(samples, marker='o')
plt.title("Ramp values from DDR (16-bit LE samples)")
plt.xlabel("Sample index")
plt.ylabel("Value")
plt.grid(True)
plt.tight_layout()
plt.savefig("output.png", dpi=150)
plt.close()

print("Saved plot to output.png")
