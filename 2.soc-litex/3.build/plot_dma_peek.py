#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt

LINE_RE = re.compile(
    r"""
    ^\s*\[\s*\d+\s*\]\s*            # [   0]
    (0x[0-9a-fA-F]+)                # 0x4a01b4fd
    (?:\s+s0=\s*([-+]?\d+)          # s0=...
        \s+s1=\s*([-+]?\d+)         # s1=...
    )?
    """,
    re.VERBOSE,
)
HEX_ONLY_RE = re.compile(r"^\s*(0x[0-9a-fA-F]+)\s*$")

def _parse_words_or_pairs(lines):
    words = []
    s0_list = []
    s1_list = []
    for ln in lines:
        ln = ln.strip()
        if not ln:
            continue
        m = LINE_RE.match(ln)
        if m:
            hexword = int(m.group(1), 16)
            s0 = m.group(2)
            s1 = m.group(3)
            if s0 is not None and s1 is not None:
                s0_list.append(np.int16(int(s0)))
                s1_list.append(np.int16(int(s1)))
            else:
                words.append(hexword)
            continue
        m2 = HEX_ONLY_RE.match(ln)
        if m2:
            words.append(int(m2.group(1), 16))
            continue
    if s0_list and s1_list:
        s0_arr = np.array(s0_list, dtype=np.int16)
        s1_arr = np.array(s1_list, dtype=np.int16)
    else:
        w = np.array(words, dtype=np.uint32)
        # LSB=older, MSB=newer (expected)
        s0_arr = (w & 0xFFFF).astype(np.uint16).view(np.int16)
        s1_arr = ((w >> 16) & 0xFFFF).astype(np.uint16).view(np.int16)
    return s0_arr, s1_arr

def _interleave(a, b):
    out = np.empty(a.size + b.size, dtype=np.int16)
    out[0::2] = a
    out[1::2] = b
    return out

def _signext12_ok(x16):
    """Return count of samples that violate 12->16 sign extension."""
    # For a correct 12->16 sign-extend, bits [15:12] must equal bit11 (sign).
    sign = (x16 & 0x0800) != 0
    top4 = (x16 >> 12) & 0xF
    bad = ((sign) & (top4 != 0xF)) | ((~sign) & (top4 != 0x0))
    return int(np.count_nonzero(bad))

def _to_signed12(x16):
    """Map int16 to signed 12-bit range [-2048, 2047]."""
    v = (x16 & 0x0FFF).astype(np.int32)
    v[v & 0x0800 != 0] -= 4096
    return v.astype(np.int16)

def _delta_check(a12, step, mod=4096):
    """
    Check that successive deltas equal step modulo 4096.
    Return (bad_steps, dupes, skips, wraps).
    """
    if a12.size < 2:
        return 0, 0, 0, 0
    d = np.diff(a12.astype(np.int32))
    # modulo normalize to [-2048..+2047] domain
    # Expected forward step could appear as (step) or (step-4096) on wrap
    step_mod  = int(step % mod)
    wrap_step = step_mod - mod

    ok = (d == step_mod) | (d == wrap_step)

    bad_steps = int(np.count_nonzero(~ok))
    # Quick duplicate / skip heuristics:
    dupes = int(np.count_nonzero(d == 0))
    # Big positive jumps not equal to step => skip
    skips = int(np.count_nonzero((d > step_mod) & (d != step_mod)))
    wraps = int(np.count_nonzero(d < 0))

    return bad_steps, dupes, skips, wraps

def parse_peek_text(lines):
    """Return (samples_LSBfirst, samples_MSBfirst) as int16 sequences."""
    s0, s1 = _parse_words_or_pairs(lines)
    lsbfirst = _interleave(s0, s1)   # expected: older, newer
    msbfirst = _interleave(s1, s0)   # swapped hypothesis
    return lsbfirst, msbfirst

def main():
    ap = argparse.ArgumentParser(description="Verify LiteX uberClock dma_peek capture")
    ap.add_argument("--in", dest="infile", default="-",
                    help="Input file with dma_peek dump (default: stdin)")
    ap.add_argument("--fs", type=float, default=65e6,
                    help="Sample rate in Hz (default: 65e6)")
    ap.add_argument("--first", type=int, default=4096,
                    help="How many samples to show in time plot (default: 4096)")
    ap.add_argument("--save-npy", type=str, default="",
                    help="Optional .npy filename to save raw int16 samples (chosen order)")
    ap.add_argument("--png-prefix", type=str, default="",
                    help="If set, save plots to <prefix>_time.png and _spectrum.png")
    ap.add_argument("--expect-step", type=int, default=None,
                    help="Expected ramp step (usually phase_cpu & 0x0FFF)")
    args = ap.parse_args()

    # Read text
    if args.infile == "-":
        lines = sys.stdin.read().splitlines()
    else:
        with open(args.infile, "r", encoding="utf-8") as f:
            lines = f.read().splitlines()

    s_lsb, s_msb = parse_peek_text(lines)
    print(f"Parsed {s_lsb.size} int16 samples (assuming packed 2x int16 per word).")

    # Evaluate both word orders and pick the best (fewest errors)
    def score(seq):
        signext_bad = _signext12_ok(seq)
        a12 = _to_signed12(seq)
        if args.expect_step is None:
            # If no step given, just use signext as score
            bad_steps = 0
            dupes = skips = wraps = 0
        else:
            bad_steps, dupes, skips, wraps = _delta_check(a12, args.expect_step)
        total_bad = signext_bad + bad_steps
        return dict(signext_bad=signext_bad, bad_steps=bad_steps,
                    dupes=dupes, skips=skips, wraps=wraps, total_bad=total_bad)

    res_lsb = score(s_lsb)
    res_msb = score(s_msb)

    if res_lsb["total_bad"] <= res_msb["total_bad"]:
        chosen = "LSBfirst"
        seq16  = s_lsb
        res    = res_lsb
    else:
        chosen = "MSBfirst"
        seq16  = s_msb
        res    = res_msb

    a12 = _to_signed12(seq16)

    print("\n--- CAPTURE CHECK ---")
    if args.expect_step is not None:
        print(f"EXPECT_STEP={args.expect_step}")
    print(f"ORDER={chosen}")
    print(f"signext_bad={res['signext_bad']}  bad_steps={res['bad_steps']}  "
          f"dupes={res['dupes']}  skips={res['skips']}  wraps={res['wraps']}")
    if res["signext_bad"] == 0 and res["bad_steps"] == 0:
        print("✅ CAPTURE OK")
    else:
        print("⚠️  CAPTURE ANOMALIES DETECTED")

    # Optional save
    if args.save_npy:
        np.save(args.save_npy, seq16)
        print(f"Saved chosen-order samples to {args.save_npy}")

    # Plots
    fs = float(args.fs)
    Nshow = min(args.first, seq16.size)
    t = np.arange(Nshow) / fs

    plt.figure()
    plt.plot(t, a12[:Nshow])
    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude (signed 12-bit)")
    ttl = f"Time domain (first {Nshow} / {seq16.size} @ {fs/1e6:.2f} MS/s, {chosen})"
    if args.expect_step is not None:
        ttl += f"  step={args.expect_step}"
    plt.title(ttl)
    plt.grid(True)
    if args.png_prefix:
        plt.savefig(f"{args.png_prefix}_time.png", dpi=120)

    # Spectrum quicklook
    Nfft_max = 1 << 20
    Nfft = 1 << int(np.floor(np.log2(min(seq16.size, Nfft_max))))
    if Nfft >= 16:
        win = np.hanning(Nfft)
        X = np.fft.rfft(a12[:Nfft].astype(np.float64) * win)
        f = np.fft.rfftfreq(Nfft, d=1/fs)
        mag = 20 * np.log10(np.maximum(np.abs(X), 1e-12))
        plt.figure()
        plt.plot(f / 1e6, mag)
        plt.xlabel("Frequency (MHz)")
        plt.ylabel("Magnitude (dBFS-ish)")
        plt.title(f"Spectrum (N={Nfft}, {chosen})")
        plt.grid(True)
        if args.png_prefix:
            plt.savefig(f"{args.png_prefix}_spectrum.png", dpi=120)
    else:
        print("Not enough samples for FFT; skipping spectrum plot.")

    plt.show()

if __name__ == "__main__":
    main()
