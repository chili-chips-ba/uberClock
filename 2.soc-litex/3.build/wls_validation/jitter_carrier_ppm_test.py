#!/usr/bin/env python3
"""
Checks whether a change in baseband-measured jitter (Hz) on the closed-loop
reference channel is practically significant, by converting it to the same
relative (ppm) units used everywhere else in this pipeline -- using the
REAL carrier the reference channel stands in for (~10 MHz), not the ~1 kHz
baseband IF frequency it's actually measured at.

Context: the closed-loop bench setup (input_select=1) downconverts an
internally generated ~10 MHz tone by a known, exact, jitter-free commanded
LO (phase_down_1) to a ~1 kHz baseband IF, purely to get finer *relative*
frequency resolution from a fixed-length FFT than would be possible
measuring directly at 10 MHz. Absolute Hz jitter measured on that baseband
tone is the SAME absolute Hz jitter on the reconstructed ~10 MHz reference
frequency -- so converting it to ppm must divide by the real ~10 MHz
carrier. Dividing by the ~1 kHz IF instead (an easy mistake -- the same
category of error caught and fixed in taylor_truncation_test.py) makes a
negligible number look 10,000x larger than it really is.

No external dependencies; standard library only.
"""

F_REF_HZ = 10_000_000.0  # the real carrier this reference channel stands in for

# Real per-mode residual noise floors already established in this project
# (Table 1 / eq. 160/168/176/184/192).
SIGMA_TABLE_PPM = {
    "C300": 1.50343, "C100": 14.2234, "B300": 25.1251, "A100": 52.8828, "B100": 69.5483,
}

# Measured jitter (std_mhz from analyze_loopback_capture.py), in Hz, before
# and after the Hann + log-power firmware change, at the N=2048 "mid-bin"
# and bin-boundary operating points.
MEASUREMENTS_HZ = {
    "N=2048 mid-bin,   pre-fix ": 1.533e-3,
    "N=2048 mid-bin,   post-fix": 8.321e-3,
    "N=2048 boundary,  pre-fix ": 19.313e-3,
    "N=2048 boundary,  post-fix": 6.683e-3,
}


def to_ppm(jitter_hz, carrier_hz):
    return jitter_hz / carrier_hz * 1e6


def main():
    print("=== Jitter converted using the WRONG carrier (the 1 kHz baseband IF) ===")
    print(f"{'measurement':28s}{'jitter (Hz)':>14}{'wrong ppm':>14}")
    for name, hz in MEASUREMENTS_HZ.items():
        print(f"{name:28s}{hz*1000:>11.3f}mHz{to_ppm(hz, 1000.0):>14.4f}")
    print()

    print("=== Jitter converted using the CORRECT carrier (the real ~10 MHz reference) ===")
    print(f"{'measurement':28s}{'jitter (Hz)':>14}{'correct ppm':>16}")
    ppm_values = {}
    for name, hz in MEASUREMENTS_HZ.items():
        ppm = to_ppm(hz, F_REF_HZ)
        ppm_values[name] = ppm
        print(f"{name:28s}{hz*1000:>11.3f}mHz{ppm:>16.8f}")
    print()

    worst_post_fix_ppm = max(
        ppm_values["N=2048 mid-bin,   post-fix"],
        ppm_values["N=2048 boundary,  post-fix"],
    )
    print("=== Compared against every real per-mode noise floor in this project (Table 1) ===")
    print(f"{'mode':8s}{'sigma (ppm)':>14}{'x larger than worst post-fix jitter':>40}")
    for name, sigma in SIGMA_TABLE_PPM.items():
        print(f"{name:8s}{sigma:>14.4f}{sigma/worst_post_fix_ppm:>40.0f}x")
    print()
    print("Conclusion: measured in the units that actually matter for this pipeline")
    print("(ppm of the real ~10 MHz reference, not ppm of the ~1 kHz IF it happens to")
    print("be measured at), both the pre-fix and post-fix jitter figures are already")
    print("3-5 orders of magnitude below every real per-mode noise floor already")
    print("established in this project. The post-fix increase at the N=2048 mid-bin")
    print("point is real but does not move the needle on the system's actual")
    print("achievable precision -- it is not a practically significant regression.")


if __name__ == "__main__":
    main()
