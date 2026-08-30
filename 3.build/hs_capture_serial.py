#!/usr/bin/env python3
import argparse
import json
import re
import sys
import time
from pathlib import Path

import serial


ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
HEXDUMP_RE = re.compile(r"^\s*([0-9a-fA-F]{8}):\s*((?:[0-9a-fA-F]{2}\s+)+)\s*$")

DEFAULT_PORT = "/dev/ttyUSB4"
DEFAULT_BAUD = 115200
DEFAULT_ADDR = 0xA0000000
DEFAULT_SOURCE = 0  # filter_in
DEFAULT_BOARD_IP = "192.168.0.123"
DEFAULT_HOST_IP = "192.168.0.2"
DEFAULT_UDP_PORT = 5000
DEFAULT_UDP_RATE_MBPS = 2.0
DEFAULT_UDP_CHUNK_BYTES = 8 * 1024 * 1024
DEFAULT_UDP_INTER_CHUNK_GAP_S = 0.5
SAMPLES_PER_BEAT = 16
BYTES_PER_BEAT = 32


def strip_ansi(text: str) -> str:
    return ANSI_RE.sub("", text)


def beats_for_samples(samples: int) -> int:
    return (samples + SAMPLES_PER_BEAT - 1) // SAMPLES_PER_BEAT


def estimate_dump_seconds(byte_count: int, baud: int) -> float:
    # ASCII hexdump is roughly 3 chars/byte plus line/address overhead.
    estimated_text_bytes = byte_count * 4
    return estimated_text_bytes * 10.0 / float(baud)


def estimate_udp_seconds(byte_count: int, mib_per_s: float = DEFAULT_UDP_RATE_MBPS) -> float:
    return byte_count / (mib_per_s * 1024.0 * 1024.0)


class ConsoleLink:
    def __init__(self, port: str, baud: int, timeout: float) -> None:
        self.ser = serial.Serial(port, baud, timeout=timeout)

    def close(self) -> None:
        self.ser.close()

    def _read_until_idle(self, timeout_s: float, idle_s: float = 0.4) -> str:
        deadline = time.monotonic() + timeout_s
        last_rx = None
        buf = bytearray()

        while time.monotonic() < deadline:
            chunk = self.ser.read(4096)
            now = time.monotonic()
            if chunk:
                buf.extend(chunk)
                last_rx = now
                continue

            if last_rx is not None and (now - last_rx) >= idle_s:
                return strip_ansi(buf.decode(errors="ignore"))

        return strip_ansi(buf.decode(errors="ignore"))

    def _read_until_prompt(self, timeout_s: float) -> str:
        deadline = time.monotonic() + timeout_s
        buf = bytearray()

        while time.monotonic() < deadline:
            chunk = self.ser.read(4096)
            if chunk:
                buf.extend(chunk)
                cleaned = strip_ansi(buf.decode(errors="ignore"))
                if cleaned.rstrip().endswith(">"):
                    return cleaned
                deadline = time.monotonic() + timeout_s

        cleaned = strip_ansi(buf.decode(errors="ignore"))
        raise TimeoutError(f"timed out waiting for prompt; last output:\n{cleaned[-4000:]}")

    def sync_prompt(self) -> None:
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        self.ser.write(b"\n")
        self.ser.flush()
        try:
            self._read_until_prompt(1.5)
        except TimeoutError:
            # Some builds do not print a visible prompt; allow blind command mode.
            self._read_until_idle(0.5)

    def run_command(self, cmd: str, timeout_s: float = 2.0) -> str:
        self.ser.write(cmd.encode("ascii") + b"\n")
        self.ser.flush()
        try:
            return self._read_until_prompt(timeout_s)
        except TimeoutError:
            return self._read_until_idle(timeout_s)


def parse_hexdump(output: str) -> bytes:
    data = bytearray()
    for line in output.splitlines():
        match = HEXDUMP_RE.match(line)
        if not match:
            continue
        hex_bytes = match.group(2).split()
        data.extend(int(b, 16) for b in hex_bytes)
    return bytes(data)


def dump_region(link: ConsoleLink, addr: int, total_bytes: int, chunk_bytes: int, outfile: Path) -> None:
    dumped = 0
    started = time.monotonic()

    with outfile.open("wb") as f:
        while dumped < total_bytes:
            this_chunk = min(chunk_bytes, total_bytes - dumped)
            cmd = f"ub_hexdump 0x{addr + dumped:08x} {this_chunk}"
            output = link.run_command(cmd, timeout_s=max(2.0, this_chunk / 512.0))
            payload = parse_hexdump(output)
            if len(payload) != this_chunk:
                raise RuntimeError(
                    f"hexdump parse mismatch at offset {dumped}: "
                    f"expected {this_chunk} bytes, got {len(payload)}"
                )

            f.write(payload)
            dumped += this_chunk

            elapsed = max(time.monotonic() - started, 1e-6)
            rate = dumped / elapsed
            remaining = total_bytes - dumped
            eta = remaining / rate if rate > 0.0 else 0.0
            print(
                f"\rDumped {dumped}/{total_bytes} bytes "
                f"({100.0 * dumped / total_bytes:5.1f}%) "
                f"rate={rate/1024.0:7.1f} KiB/s eta={eta:7.1f}s",
                end="",
                flush=True,
            )

    print()


def write_metadata(path: Path, meta: dict) -> None:
    with path.open("w", encoding="utf-8") as f:
        json.dump(meta, f, indent=2, sort_keys=True)
        f.write("\n")


def run_udp_send(
    link: ConsoleLink,
    addr: int,
    byte_count: int,
    host_ip: str,
    udp_port: int,
    capture_seconds: float,
    chunk_bytes: int,
    inter_chunk_gap_s: float,
) -> None:
    sent = 0
    chunk_index = 0

    while sent < byte_count:
        this_chunk = min(chunk_bytes, byte_count - sent)
        udp_seconds = estimate_udp_seconds(this_chunk)
        print(
            f"Sending UDP chunk {chunk_index} "
            f"offset=0x{addr + sent:08x} bytes={this_chunk} "
            f"to {host_ip}:{udp_port}"
        )
        print(
            strip_ansi(
                link.run_command(
                    f"ub_send 0x{addr + sent:08x} {this_chunk} {host_ip} {udp_port}",
                    timeout_s=max(20.0, capture_seconds + udp_seconds + 20.0),
                )
            ).strip()
        )
        sent += this_chunk
        chunk_index += 1
        print(f"Completed send chunk {chunk_index}, total sent {sent}/{byte_count} bytes")
        if sent < byte_count and inter_chunk_gap_s > 0.0:
            time.sleep(inter_chunk_gap_s)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Capture UberClock high-speed debug data over the serial console and save raw DDR contents."
    )
    ap.add_argument("outfile", help="raw output file (.bin recommended)")
    ap.add_argument("--port", default=DEFAULT_PORT, help=f"serial port (default: {DEFAULT_PORT})")
    ap.add_argument("--baud", type=int, default=DEFAULT_BAUD, help=f"serial baud (default: {DEFAULT_BAUD})")
    ap.add_argument("--addr", type=lambda x: int(x, 0), default=DEFAULT_ADDR, help="DDR base address")
    ap.add_argument("--samples", type=int, default=1 << 26, help="number of 16-bit samples to capture")
    ap.add_argument("--source", type=int, default=DEFAULT_SOURCE, choices=range(4), help="highspeed_dbg_select source")
    ap.add_argument("--action", choices=("capture", "send", "both"), default="both", help="serial control action")
    ap.add_argument("--transport", choices=("udp", "serial"), default="udp", help="data transfer method")
    ap.add_argument("--host-ip", default=DEFAULT_HOST_IP, help="host IP for board UDP sender and local UDP bind")
    ap.add_argument("--udp-port", type=int, default=DEFAULT_UDP_PORT, help="UDP port for UBD3 transfer")
    ap.add_argument("--udp-chunk-bytes", type=int, default=DEFAULT_UDP_CHUNK_BYTES, help="UDP send chunk size in bytes")
    ap.add_argument("--udp-inter-chunk-gap", type=float, default=DEFAULT_UDP_INTER_CHUNK_GAP_S, help="delay between UDP send chunks in seconds")
    ap.add_argument("--chunk-bytes", type=int, default=4096, help="bytes per ub_hexdump command")
    ap.add_argument("--timeout", type=float, default=0.25, help="serial read timeout in seconds")
    args = ap.parse_args()

    if args.chunk_bytes <= 0:
        raise SystemExit("--chunk-bytes must be > 0")
    if args.udp_chunk_bytes <= 0:
        raise SystemExit("--udp-chunk-bytes must be > 0")

    outfile = Path(args.outfile)
    meta_path = outfile.with_suffix(outfile.suffix + ".json" if outfile.suffix else ".json")

    beats = beats_for_samples(args.samples)
    sample_count = beats * SAMPLES_PER_BEAT
    byte_count = beats * BYTES_PER_BEAT
    capture_seconds = sample_count / 65_000_000.0
    dump_seconds = estimate_dump_seconds(byte_count, args.baud)

    print(f"Port:           {args.port} @ {args.baud}")
    print(f"Source:         highspeed_dbg_select={args.source}")
    print(f"Action:         {args.action}")
    print(f"Transport:      {args.transport}")
    print(f"Requested:      {args.samples} samples")
    print(f"Captured:       {sample_count} samples ({beats} beats)")
    print(f"DDR bytes:      {byte_count}")
    print(f"Capture time:   {capture_seconds:.3f} s at 65 MHz")
    if args.transport == "serial":
        print(f"Serial dump ETA:{dump_seconds/60.0:.1f} min at {args.baud} baud")
    else:
        print(f"Board IP:       {DEFAULT_BOARD_IP}")
        print(f"Host UDP dst:   {args.host_ip}:{args.udp_port}")
        print(f"UDP chunk:      {args.udp_chunk_bytes} bytes")
        print(f"UDP gap:        {args.udp_inter_chunk_gap} s")
    print(f"Output:         {outfile}")
    print(f"Metadata:       {meta_path}")

    if sample_count != args.samples:
        print(
            f"Note: requested sample count rounded up to {sample_count} "
            f"because one DDR beat holds {SAMPLES_PER_BEAT} samples."
        )

    link = ConsoleLink(args.port, args.baud, args.timeout)
    try:
        print("Synchronizing console prompt...")
        link.sync_prompt()

        if args.action in ("capture", "both"):
            print("Configuring high-speed source and starting capture...")
            print(strip_ansi(link.run_command(f"highspeed_dbg_select {args.source}", timeout_s=2.0)).strip())
            print(strip_ansi(link.run_command(f"cap_beats {beats}", timeout_s=2.0)).strip())
            print(strip_ansi(link.run_command(f"ub_cap 0x{args.addr:08x} {beats}", timeout_s=4.0)).strip())
            print(strip_ansi(link.run_command("ub_wait", timeout_s=max(5.0, capture_seconds + 5.0))).strip())

        if args.action in ("send", "both"):
            if args.transport == "udp":
                run_udp_send(
                    link,
                    args.addr,
                    byte_count,
                    args.host_ip,
                    args.udp_port,
                    capture_seconds,
                    args.udp_chunk_bytes,
                    args.udp_inter_chunk_gap,
                )
            else:
                print("Dumping DDR region over serial...")
                dump_region(link, args.addr, byte_count, args.chunk_bytes, outfile)
    finally:
        link.close()

    write_metadata(
        meta_path,
        {
            "addr": args.addr,
            "baud": args.baud,
            "byte_count": byte_count,
            "capture_seconds_at_65mhz": capture_seconds,
            "chunk_bytes": args.chunk_bytes,
            "ddr_beats": beats,
            "dump_format": "raw bytes from ub_hexdump" if args.transport == "serial" else "raw bytes from UBD3 UDP stream",
            "host_ip": args.host_ip,
            "port": args.port,
            "requested_samples": args.samples,
            "sample_encoding": "little-endian signed int16",
            "sample_rate_hz": 65_000_000,
            "samples_per_beat": SAMPLES_PER_BEAT,
            "action": args.action,
            "source": args.source,
            "source_name": {
                0: "filter_in",
                1: "filter_in_1",
                2: "sum[13:2]",
                3: "nco_cos",
            }[args.source],
            "stored_samples": sample_count,
            "transport": args.transport,
            "udp_chunk_bytes": args.udp_chunk_bytes,
            "udp_inter_chunk_gap_s": args.udp_inter_chunk_gap,
            "udp_port": args.udp_port,
        },
    )

    print("Done.")
    print("Later analysis example:")
    print(f"  import numpy as np; x = np.fromfile({str(outfile)!r}, dtype='<i2')")
    return 0


if __name__ == "__main__":
    sys.exit(main())
