#!/usr/bin/env python3
import argparse, time, struct
import serial  # pyserial
import matplotlib.pyplot as plt
import numpy as np

MAGIC = b"##BIN##"

def read_exact(ser, n):
    buf = bytearray()
    while len(buf) < n:
        chunk = ser.read(n - len(buf))
        if not chunk:
            raise RuntimeError("Timeout while reading")
        buf.extend(chunk)
    return bytes(buf)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--addr", required=True, help="DDR addr, e.g. 0xA0000000")
    ap.add_argument("--nbytes", type=int, required=True, help="How many bytes to send from firmware")
    ap.add_argument("--save", default="uart_payload.bin")
    ap.add_argument("--no-show", action="store_true")
    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1.0)
    try:
        # Clean any stale text
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        # Send console command (no litex_term running!)
        cmd = f"ub_send {args.addr} {args.nbytes}\r\n"
        ser.write(cmd.encode("ascii"))

        # Wait for magic
        print("Waiting for magic preamble ...")
        window = bytearray()
        t0 = time.time()
        while True:
            b = ser.read(1)
            if not b:
                if time.time()-t0 > 5:
                    raise RuntimeError("Timeout waiting for preamble")
                continue
            window += b
            if len(window) > len(MAGIC):
                window = window[-len(MAGIC):]
            if window == MAGIC:
                break

        # length (little-endian 32-bit)
        rawlen = read_exact(ser, 4)
        n = struct.unpack("<I", rawlen)[0]
        print(f"Incoming payload: {n} bytes")

        data = read_exact(ser, n)
        print(f"Received {len(data)} bytes.")
        with open(args.save, "wb") as f:
            f.write(data)
        print(f"Saved to {args.save}")

        # quick plot (assume bytes are 0..255 ramp)
        y = np.frombuffer(data, dtype=np.uint8)
        plt.figure()
        plt.plot(y)
        plt.title("UART payload")
        plt.xlabel("sample")
        plt.ylabel("value (byte)")
        if args.no_show:
            plt.savefig("uart_plot.png")
            print("Saved plot to uart_plot.png")
        else:
            plt.show()

    finally:
        ser.close()

if __name__ == "__main__":
    main()
