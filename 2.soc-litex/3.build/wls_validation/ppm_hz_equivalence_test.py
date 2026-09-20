#!/usr/bin/env python3
"""
Confirms that computing the WLS temperature estimate in ppm (the paper's
actual pipeline, eq. 121/123/135) gives the identical answer as computing
it directly in Hz -- i.e. that "ppm vs Hz" is a units/precomputation choice
(eq. 123-130) and not a source of numerical difference.

No external dependencies; standard library only.
"""

import random

# Real nominal mode frequencies, eq. 133 order [C100, B100, A100, C300, B300].
M_N_HZ = [3_388_594.0, 3_727_969.0, 6_269_781.0, 10_004_000.0, 10_964_730.0]
ALPHA_PPM = [-1.171846, -29.366872, -34.323755, -0.358644, -25.278849]  # ppm/degC
SIGMA_PPM2 = [202.3051, 4836.9699, 2796.5879, 2.2603, 631.2718]        # ppm^2
MODE_NAMES = ["C100", "B100", "A100", "C300", "B300"]

TRUE_D = 4.2  # arbitrary synthetic ground truth, degC offset


def wls_estimate(p, alpha, sigma_diag):
    """eq. 107, generic: works for any consistent units of p/alpha/sigma^2."""
    num = sum(a * pi / s for a, pi, s in zip(alpha, p, sigma_diag))
    den = sum(a * a / s for a, s in zip(alpha, sigma_diag))
    return num / den


def main():
    random.seed(1)

    # --- Build a synthetic Hz-domain measurement: M_corr,i = M_n,i * (1 + alpha_i*1e-6*d + noise) ---
    m_corr_hz = []
    for m_n, alpha, sigma2 in zip(M_N_HZ, ALPHA_PPM, SIGMA_PPM2):
        noise_ppm = random.gauss(0.0, sigma2 ** 0.5)
        p_true_ppm = alpha * TRUE_D + noise_ppm
        m_corr_hz.append(m_n * (1.0 + p_true_ppm * 1e-6))

    # --- Method A: the paper's actual pipeline -- convert to ppm first (eq. 121/123),
    # then WLS with alpha in ppm/degC and Sigma in ppm^2. ---
    p_ppm = [1e6 * (mc - mn) / mn for mc, mn in zip(m_corr_hz, M_N_HZ)]  # eq. 121
    d_hat_ppm_method = wls_estimate(p_ppm, ALPHA_PPM, SIGMA_PPM2)

    # --- Method B: work directly in Hz -- undo the ppm scaling on alpha and Sigma
    # by the same per-channel K_i = 1e6/M_n,i (eq. 123), so everything is in Hz / (Hz/degC) / Hz^2. ---
    K = [1e6 / mn for mn in M_N_HZ]
    alpha_hz = [a / k for a, k in zip(ALPHA_PPM, K)]        # Hz/degC
    sigma2_hz = [s / (k * k) for s, k in zip(SIGMA_PPM2, K)]  # Hz^2
    p_hz = [mc - mn for mc, mn in zip(m_corr_hz, M_N_HZ)]     # Hz
    d_hat_hz_method = wls_estimate(p_hz, alpha_hz, sigma2_hz)

    print("=== ppm-domain pipeline vs. Hz-domain pipeline, same synthetic data ===")
    print(f"true d                      = {TRUE_D} degC")
    print(f"d_hat via ppm (eq.121+107)  = {d_hat_ppm_method!r}")
    print(f"d_hat via raw Hz (rescaled) = {d_hat_hz_method!r}")
    abs_diff = abs(d_hat_ppm_method - d_hat_hz_method)
    rel_diff = abs_diff / abs(d_hat_ppm_method)
    print(f"absolute difference         = {abs_diff:.3e} degC")
    print(f"relative difference         = {rel_diff:.3e}  "
          f"(floating-point noise floor, not a real discrepancy)")
    print()
    print("Per-mode check that alpha_hz/K_i reproduces the original ppm alpha exactly:")
    for name, a_ppm, a_hz, k in zip(MODE_NAMES, ALPHA_PPM, alpha_hz, K):
        print(f"  {name:5s}: alpha_hz*K = {a_hz*k:+.6f}  (should equal alpha_ppm = {a_ppm:+.6f})")
    print()
    print("Conclusion: the two pipelines agree to within IEEE-754 floating-point")
    print("precision (~1e-12 relative), confirming ppm vs Hz is a units choice for")
    print("human readability and division-free precomputation (eq. 123-130), not a")
    print("source of numerical difference in the WLS result.")


if __name__ == "__main__":
    main()
