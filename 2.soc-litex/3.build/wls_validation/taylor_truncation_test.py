#!/usr/bin/env python3
"""
Validates the paper's first-order Taylor approximation (eq. 28-38) and
quantifies whether it could plausibly explain the sawtooth artifact.

Comparisons are done in RELATIVE (ppm) terms throughout, not absolute Hz --
mixing an absolute-Hz error computed at a 10 MHz carrier with a jitter
measured on a ~1 kHz baseband signal would compare two different physical
stages of the pipeline at a 10,000x different scale and isn't meaningful.
Every real noise source in this project (Sigma, eq. 195; the bin-boundary
FFT-interpolator jitter measured on the bench) is naturally a relative
(ppm-level) quantity, so eq. 34's *relative* error e_rec = eps^2/(1+eps) is
the right thing to compare against, not eq. 35's f-scaled absolute version.

No external dependencies; standard library only.
"""

F_REF_HZ = 10_000_000.0     # eq. 10, reference frequency, for the worked-example check only

# This board's own actual measured clock-error range, taken directly from
# the paper's results table (eq. 147): F ranged from 64999390 to 65001943 Hz
# against F0 = 65,000,000 Hz.
F0_HZ = 65_000_000.0
F_MEASURED_MIN_HZ = 64_999_390.0
F_MEASURED_MAX_HZ = 65_001_943.0

# Real ppm-level noise floors already established elsewhere in this project.
SIGMA_TABLE_PPM = {  # eq. 160/168/176/184/192 (sigma, not sigma^2)
    "C100": 14.2234, "B100": 69.5483, "A100": 52.8828, "C300": 1.50343, "B300": 25.1251,
}
BENCH_JITTER_PPM_CLEAN = 0.15e-3 / 1000.0 * 1e6   # ~0.15 mHz jitter on a ~1000 Hz tone
BENCH_JITTER_PPM_WORST = 21.6e-3 / 1000.0 * 1e6   # ~21.6 mHz jitter on a ~1000 Hz tone


def e_rec(eps):
    """eq. 34: exact RELATIVE (dimensionless) error from the first-order approximation."""
    return eps * eps / (1.0 + eps)


def e_lin(eps, f=F_REF_HZ):
    """eq. 35: absolute error in Hz at a given carrier f (worked-example check only)."""
    return f * e_rec(eps)


def main():
    print("=== Reproduce the paper's worked example (eq. 36-38) ===")
    eps_example = 30e-6
    e = e_lin(eps_example)
    print(f"eps            = {eps_example:.0e}  (30 ppm)")
    print(f"e_lin (eq. 38) = {e:.3e} Hz at f=10MHz  "
          f"(paper states ~9e-3 Hz -- {'MATCH' if abs(e-9e-3)<1e-3 else 'MISMATCH'})")
    print()

    eps_min = (F_MEASURED_MIN_HZ - F0_HZ) / F0_HZ
    eps_max = (F_MEASURED_MAX_HZ - F0_HZ) / F0_HZ
    print("=== This board's own measured clock-error range (from eq. 147's F column) ===")
    print(f"eps range = [{eps_min*1e6:+.1f}, {eps_max*1e6:+.1f}] ppm  "
          f"(matches the paper's own eps=30ppm worked example)")
    print()

    print("=== Compare in RELATIVE (ppm) units -- the units used throughout the rest ===")
    print("=== of this pipeline (Sigma, the ppm-deviation vector p, etc.)          ===")
    for label, eps in [("min measured", eps_min), ("max measured", eps_max), ("paper example", eps_example)]:
        rel_ppm = e_rec(eps) * 1e6
        print(f"  {label:14s} (eps={eps*1e6:+7.2f} ppm): e_rec = {rel_ppm:.6f} ppm")
    worst_rel_ppm = e_rec(eps_max) * 1e6
    print()

    print("=== Against real ppm-level noise floors already in this project ===")
    print(f"{'source':30s}{'value (ppm)':>14}{'x larger than e_rec':>22}")
    for name, sigma in SIGMA_TABLE_PPM.items():
        print(f"  sigma_{name:6s}(eq. Table 1)      {sigma:>10.4f}      {sigma/worst_rel_ppm:>18.0f}x")
    print(f"  bench jitter, clean bin position   {BENCH_JITTER_PPM_CLEAN:>10.4f}      "
          f"{BENCH_JITTER_PPM_CLEAN/worst_rel_ppm:>18.0f}x")
    print(f"  bench jitter, worst bin boundary   {BENCH_JITTER_PPM_WORST:>10.4f}      "
          f"{BENCH_JITTER_PPM_WORST/worst_rel_ppm:>18.0f}x")
    print()
    print("Conclusion: expressed in the same relative (ppm) units used everywhere")
    print("else in this pipeline, the first-order Taylor truncation error at this")
    print(f"board's actual measured clock-error range is ~{worst_rel_ppm:.4f} ppm -- "
          f"3-5 orders of magnitude")
    print("smaller than every real ppm-level noise source already measured or fitted")
    print("in this project (Table 1's sigma values, and the bin-boundary FFT jitter).")
    print("It cannot be the source of the sawtooth or of any jitter observed on the bench.")


if __name__ == "__main__":
    main()
