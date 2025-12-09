#!/usr/bin/env python3
import serial
import time
import csv
import re
import argparse

PORT = "/dev/ttyUSB3"   # adjust if needed
BAUD = 115200           # adjust if needed

LINE_RE = re.compile(r"^\s*(\d+)\s*,\s*(-?\d+)\s*$")

def auto_capture(mode: str, outfile: str):
    if mode == "low":
        start_cmd = b"cap_start\n"
        dump_cmd  = b"cap_dump\n"
        wait_s    = 0.5   # 2048 samples @ 10 kHz ≈ 0.2 s → 0.5 s is safe
    elif mode == "high":
        start_cmd = b"hs_start\n"
        dump_cmd  = b"hs_dump\n"
        wait_s    = 0.1   # 8192 samples @ 65 MHz ≪ 0.1 s
    else:
        raise ValueError("mode must be 'low' or 'high'")

    print(f"Opening {PORT} @ {BAUD}...")
    ser = serial.Serial(PORT, BAUD, timeout=0.5)

    # Clean buffers
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # Start capture
    print(f"Sending {start_cmd.strip().decode()}...")
    ser.write(start_cmd)
    ser.flush()

    # Give the FPGA time to fill RAM
    print(f"Waiting {wait_s*1000:.0f} ms for capture to complete...")
    time.sleep(wait_s)

    # Request dump
    print(f"Sending {dump_cmd.strip().decode()}...")
    ser.write(dump_cmd)
    ser.flush()

    rows = []
    print("Reading dump...")

    while True:
        line_bytes = ser.readline()
        if not line_bytes:
            # timeout with no data → assume we’re done
            break

        line = line_bytes.decode(errors="ignore").strip()
        if not line:
            continue

        # Uncomment for debugging:
        # print("RAW:", repr(line))

        # Stop if shell prompt shows up again
        if "uberClock>" in line:
            print("Prompt detected, stopping.")
            break

        # Ignore header/end lines like "#idx,value" / "HS dump DONE"
        if line.startswith("#"):
            continue
        if "dump DONE" in line:
            print(line)
            continue

        m = LINE_RE.match(line)
        if m:
            idx = int(m.group(1))
            val = int(m.group(2))
            rows.append((idx, val))
            if idx % 256 == 0:
                print(f"[{idx}] {val}")
        else:
            # Non-data line, ignore
            # print("?", line)
            pass

    ser.close()

    print(f"Captured {len(rows)} samples, writing {outfile}...")
    with open(outfile, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["idx", "value"])
        w.writerows(rows)

    print("Done.")

def main():
    ap = argparse.ArgumentParser(description="UberClock low/high speed capture to CSV")
    ap.add_argument("mode", choices=["low", "high"], help="capture mode")
    ap.add_argument("outfile", help="output CSV file")
    args = ap.parse_args()

    auto_capture(args.mode, args.outfile)

if __name__ == "__main__":
    main()
