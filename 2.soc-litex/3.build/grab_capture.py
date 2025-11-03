#!/usr/bin/env python3
import argparse, sys, time, re
from datetime import datetime
import serial

PROMPT = b"uberClock> "

def expect_prompt(ser, timeout=2.0):
    ser.timeout = 0.1
    buf = b""
    t0 = time.time()
    while time.time() - t0 < timeout:
        chunk = ser.read(4096)
        if chunk:
            buf += chunk
            if PROMPT in buf:
                return buf
    return buf

def send_cmd(ser, cmd):
    ser.write(cmd.encode("ascii") + b"\r\n")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--len",  type=int, default=2048, help="capture length (cap_len)")
    ap.add_argument("--out",  default=None, help="output CSV file")
    args = ap.parse_args()

    out_path = args.out or f"cap_{args.len}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    ser = serial.Serial(args.port, args.baud, timeout=0.1)
    time.sleep(0.2)
    ser.reset_input_buffer(); ser.reset_output_buffer()

    # wake + sync to prompt
    ser.write(b"\r\n")
    expect_prompt(ser, timeout=2.0)

    # set length, start
    send_cmd(ser, f"cap_len {args.len}")
    expect_prompt(ser, timeout=1.0)
    send_cmd(ser, "cap_start")

    # poll status until done
    done = False
    for _ in range(200):  # ~20s max
        time.sleep(0.05)
        send_cmd(ser, "cap_status")
        buf = expect_prompt(ser, timeout=0.5).decode("ascii", errors="ignore")
        m = re.search(r"busy=(\d+)\s+done=(\d+)\s+len=(\d+)", buf)
        if m:
            busy, done_flag, nlat = map(int, m.groups())
            if done_flag == 1:
                done = True
                cap_n = nlat
                break
    if not done:
        print("Timeout waiting for capture to finish.", file=sys.stderr)
        sys.exit(1)

    # request dump
    send_cmd(ser, "cap_dump")

    # read header + exactly cap_n lines of CSV
    lines = []
    got_header = False
    ser.timeout = 1.0
    t0 = time.time()
    while len(lines) < cap_n and (time.time() - t0) < 5.0:
        ln = ser.readline().decode("ascii", errors="ignore").strip()
        if not ln:
            continue
        if not got_header:
            if ln.startswith("#idx"):
                got_header = True
            continue
        # Expect "idx,value"
        if "," in ln:
            lines.append(ln)
    if len(lines) != cap_n:
        print(f"Expected {cap_n} rows, got {len(lines)}", file=sys.stderr)

    # save
    with open(out_path, "w") as f:
        f.write("#idx,value\n")
        f.write("\n".join(lines))
        f.write("\n")
    print(f"Saved {len(lines)} samples to {out_path}")

    # return to prompt cleanly
    expect_prompt(ser, timeout=0.5)
    ser.close()

if __name__ == "__main__":
    main()

