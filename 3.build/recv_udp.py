#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import struct
import time
import numpy as np
import sys

# ---------------- UDP protocol ----------------
MAGIC = 0x55424433
HDR   = struct.Struct("<IIII")   # magic, seq, offset, total
PORT  = 5000
OUT   = "capture.bin"
MISSING_LIST = "capture.missing.txt"

# ---------------- receiver tuning ----------------
# NOTE: kernel clamps SO_RCVBUF to net.core.rmem_max (yours was 32MB)
RCVBUF_BYTES  = 256 * 1024 * 1024
SOCK_TIMEOUT  = 0.5
STALL_SECS    = 2.0
PRINT_EVERY   = 0.5

# ---------------- data interpretation ----------------
FS_HZ      = 65e6
BEAT_BYTES = 32
LANES      = 16
U16_LE     = np.dtype("<u2")

# ---------------- plotting ----------------
PLOT_ENABLE = True
PLOT_DECIM  = 2000  # large enough to stay responsive; decrease for more detail

def validate_ramp(buf: bytes):
    beats = len(buf) // BEAT_BYTES
    u16 = np.frombuffer(buf, dtype=U16_LE, count=beats * LANES)
    vals = u16.reshape(beats, LANES).astype(np.uint32)

    print(f"beats: {beats}  u16: {u16.size}")

    k = np.arange(beats, dtype=np.uint32)[:, None]
    lane = np.arange(LANES, dtype=np.uint32)[None, :]
    exp = (k * LANES + lane) & 0xFFFF

    mismatch = np.nonzero(vals != exp)
    if mismatch[0].size == 0:
        print("EXACT beat/lane check: OK (entire capture)")
        return True

    b0 = int(mismatch[0][0])
    l0 = int(mismatch[1][0])
    print("EXACT beat/lane check: FAIL")
    print(f"  first mismatch: beat={b0} lane={l0} got={int(vals[b0,l0])} exp={int(exp[b0,l0])}")
    print("  beat got:", [int(x) for x in vals[b0, :]])
    print("  beat exp:", [int(x) for x in exp[b0, :]])

    for bb in [b0-2, b0-1, b0, b0+1, b0+2]:
        if 0 <= bb < beats:
            print(f"  beat {bb} lane0={int(vals[bb,0])} exp_lane0={int(exp[bb,0])}")

    return False

def pick_interactive_backend():
    # Try Qt first, then Tk. If neither available, keep whatever matplotlib default is.
    try:
        import PyQt5  # noqa
        return "Qt5Agg"
    except Exception:
        pass
    try:
        import tkinter  # noqa
        return "TkAgg"
    except Exception:
        pass
    return None

def plot_interactive(samples: np.ndarray, fs_hz: float, decim: int):
    import matplotlib
    be = pick_interactive_backend()
    if be is not None:
        matplotlib.use(be)

    import matplotlib.pyplot as plt

    if decim < 1:
        decim = 1

    y = samples[::decim]
    t = (np.arange(y.size, dtype=np.float64) * decim) / fs_hz

    fig, ax = plt.subplots()
    ax.plot(t, y, linewidth=0.8)
    ax.set_title(f"capture.bin (decimated ×{decim})  fs={fs_hz/1e6:.2f} MHz")
    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Sample (int16 view)")
    ax.grid(True)

    # Toolbar provides zoom/pan/reset.
    plt.show()

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, RCVBUF_BYTES)
    sock.bind(("0.0.0.0", PORT))
    sock.settimeout(SOCK_TIMEOUT)

    actual = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    print("listening on UDP", PORT)
    print("SO_RCVBUF actual:", actual, "bytes")

    rx = bytearray(4096)

    mv = memoryview(rx)

    buf = None
    total = None
    block_size = None
    nblocks = None
    received = None
    blocks_rx = 0

    pkts = pkts_valid = 0
    dup_blocks = bad_magic = bad_len = bad_bounds = bad_block_align = 0

    t0 = time.time()
    last_print = 0.0
    last_progress = time.time()

    while True:
        try:
            n, _ = sock.recvfrom_into(mv)
        except socket.timeout:
            if buf is not None and (time.time() - last_progress) > STALL_SECS:
                missing = np.nonzero(received == 0)[0]
                pct = 100.0 * blocks_rx / nblocks
                print(f"STALL at {pct:.2f}%: missing {missing.size}/{nblocks} blocks")
                print("first 32 missing:", missing[:32].tolist())
                with open(MISSING_LIST, "w") as f:
                    for b in missing.tolist():
                        f.write(f"{b}\n")
                print("wrote missing list:", MISSING_LIST)
                break
            continue

        pkts += 1
        if n < HDR.size:
            bad_len += 1
            continue

        magic, seq, off, tot = HDR.unpack_from(rx, 0)
        if magic != MAGIC:
            bad_magic += 1
            continue

        payload_len = n - HDR.size
        if payload_len <= 0:
            bad_len += 1
            continue

        if buf is None:
            total = tot
            buf = bytearray(total)
            block_size = payload_len
            nblocks = (total + block_size - 1) // block_size
            received = np.zeros(nblocks, dtype=np.uint8)
            print(f"capture size: {total} bytes")
            print(f"block_size:   {block_size} bytes")
            print(f"blocks:       {nblocks}")

        end = off + payload_len
        if end > total:
            bad_bounds += 1
            continue
        if (off % block_size) != 0:
            bad_block_align += 1
            continue

        b = off // block_size
        if b >= nblocks:
            bad_bounds += 1
            continue

        buf[off:end] = rx[HDR.size:HDR.size + payload_len]
        pkts_valid += 1

        if received[b] == 0:
            received[b] = 1
            blocks_rx += 1
            last_progress = time.time()
        else:
            dup_blocks += 1

        now = time.time()
        if (now - last_print) >= PRINT_EVERY:
            pct = 100.0 * blocks_rx / nblocks
            print(f"pkts={pkts} valid={pkts_valid} blocks={blocks_rx}/{nblocks} ({pct:.2f}%) dups={dup_blocks}")
            last_print = now

        if blocks_rx == nblocks:
            print("coverage 100% -> stopping")
            break

    dt = time.time() - t0
    print(f"received: pkts={pkts} valid={pkts_valid} blocks_rx={blocks_rx}/{(nblocks or 0)} in {dt:.2f}s")
    print(f"errors: bad_magic={bad_magic} bad_len={bad_len} bad_bounds={bad_bounds} bad_block_align={bad_block_align} dup_blocks={dup_blocks}")

    if buf is None:
        print("no data received")
        return 2

    with open(OUT, "wb") as f:
        f.write(buf)
    print("wrote", OUT, len(buf), "bytes")

    ok = validate_ramp(buf)

    if PLOT_ENABLE:
        u16 = np.frombuffer(buf, dtype=U16_LE)
        samples = u16.view(np.int16)
        try:
            plot_interactive(samples, fs_hz=FS_HZ, decim=PLOT_DECIM)
        except Exception as e:
            print("plot failed:", repr(e))
            print("Install one of: PyQt5 / PySide2 / Tk, or set a working matplotlib backend.")
            # don't fail the whole script because of plotting

    return 0 if ok else 1

if __name__ == "__main__":
    raise SystemExit(main())
