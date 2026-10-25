#!/usr/bin/env python3
import argparse
import socket
import struct
import sys
import time
from pathlib import Path


UBD3_MAGIC = 0x55424433
UBD3_HDR = struct.Struct("<IIII")
PRINT_EVERY_S = 0.25


def recv_one_stream(sock: socket.socket, expect_bytes: int, label: str) -> bytes:
    buf = None
    total = None
    received = 0
    packets = 0
    first_peer = None
    first_seq = None
    seen_offsets = None
    t0 = time.monotonic()
    last_print = 0.0

    while True:
        pkt, peer = sock.recvfrom(2048)
        if len(pkt) < UBD3_HDR.size:
            continue

        magic, seq, offset, pkt_total = UBD3_HDR.unpack_from(pkt, 0)
        if magic != UBD3_MAGIC:
            continue

        if buf is None:
            # Only start a new receive session on the first packet of a ub_send burst.
            # This avoids treating late tail packets from the previous session as a new stream.
            if seq != 0 or offset != 0:
                continue
            total = pkt_total
            if expect_bytes and total != expect_bytes:
                raise RuntimeError(f"{label}: expected {expect_bytes} bytes but stream announced {total}")
            buf = bytearray(total)
            seen_offsets = set()
            first_peer = peer
            first_seq = seq
            print(f"{label}: stream from {peer[0]}:{peer[1]} total={total} bytes")
        elif pkt_total != total:
            raise RuntimeError(f"{label}: stream total changed from {total} to {pkt_total}")

        payload = pkt[UBD3_HDR.size:]
        end = offset + len(payload)
        if end > total:
            raise RuntimeError(f"{label}: packet exceeds total size: offset={offset} len={len(payload)} total={total}")

        if offset not in seen_offsets:
            seen_offsets.add(offset)
            received += len(payload)
        buf[offset:end] = payload
        packets += 1

        elapsed = max(time.monotonic() - t0, 1e-6)
        rate = received / elapsed
        now = time.monotonic()
        if (now - last_print) >= PRINT_EVERY_S:
            print(
                f"\r{label}: packets={packets} seq={seq} received={received}/{total} "
                f"({100.0 * received / total:5.1f}%) rate={rate/1024.0/1024.0:6.2f} MiB/s",
                end="",
                flush=True,
            )
            last_print = now

        if received >= total:
            print()
            print(f"{label}: complete, first peer {first_peer[0]}:{first_peer[1]} first_seq={first_seq}")
            return bytes(buf)


def main() -> int:
    ap = argparse.ArgumentParser(description="Receive UberClock UBD3 UDP stream and save it to a file.")
    ap.add_argument("--bind", default="0.0.0.0", help="local bind address")
    ap.add_argument("--port", type=int, default=5000, help="UDP port to listen on")
    ap.add_argument("--out", required=True, help="output binary file")
    ap.add_argument("--timeout", type=float, default=5.0, help="socket timeout in seconds")
    ap.add_argument("--expect-bytes", type=int, default=0, help="expected byte count for one stream")
    ap.add_argument("--expect-total-bytes", type=int, default=0, help="expected total byte count across multiple streams")
    ap.add_argument("--session-bytes", type=int, default=0, help="expected bytes per session for multi-session receive")
    args = ap.parse_args()

    out_path = Path(args.out)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 16 * 1024 * 1024)
    sock.bind((args.bind, args.port))
    sock.settimeout(args.timeout)

    print(f"Listening on {args.bind}:{args.port}")

    try:
        if args.expect_total_bytes > 0:
            if args.session_bytes <= 0:
                raise SystemExit("--session-bytes must be > 0 when --expect-total-bytes is used")

            written = 0
            session = 0
            with out_path.open("wb") as f:
                while written < args.expect_total_bytes:
                    this_expect = min(args.session_bytes, args.expect_total_bytes - written)
                    chunk = recv_one_stream(sock, this_expect, f"session{session:04d}")
                    if len(chunk) != this_expect:
                        raise RuntimeError(
                            f"session{session:04d}: expected {this_expect} bytes, got {len(chunk)}"
                        )
                    f.write(chunk)
                    f.flush()
                    written += len(chunk)
                    session += 1
                    print(f"Appended session {session}, total written {written}/{args.expect_total_bytes} bytes")
        else:
            chunk = recv_one_stream(sock, args.expect_bytes, "session0000")
            out_path.write_bytes(chunk)
            print(f"Wrote {len(chunk)} bytes to {out_path}")
    except socket.timeout as exc:
        raise TimeoutError("UDP receive timed out before stream completed") from exc
    finally:
        sock.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
