#!/usr/bin/env python3
"""
Validates the weighted-least-squares temperature estimator (paper eq.
106/107/115/116/137) against synthetic data with known ground truth, and
checks whether "iterative improvement of a solution to linear equations"
(Numerical Recipes in C, 2nd ed., section 2.5) has anything left to correct.

Real coefficients from ASecondTryAtThis.pdf:
  alpha  (eq. 194), ppm/degC, mode order [C100, B100, A100, C300, B300]
  Sigma  (eq. 195), diagonal covariance, ppm^2

No external dependencies; standard library only.
"""

import math
import random
import statistics

ALPHA = [-1.171846, -29.366872, -34.323755, -0.358644, -25.278849]  # ppm/degC
SIGMA_DIAG = [202.3051, 4836.9699, 2796.5879, 2.2603, 631.2718]     # ppm^2
MODE_NAMES = ["C100", "B100", "A100", "C300", "B300"]

N_TRIALS = 200_000
TRUE_D = 3.0  # degC offset from 25 degC reference, arbitrary for the test


def wls_estimate(p, alpha, sigma_diag):
    """eq. 107: d_hat = sum(alpha_i*p_i/sigma_i^2) / sum(alpha_i^2/sigma_i^2)."""
    num = sum(a * pi / s for a, pi, s in zip(alpha, p, sigma_diag))
    den = sum(a * a / s for a, s in zip(alpha, sigma_diag))
    return num / den, den  # den = alpha^T Sigma^-1 alpha (eq. 115 denominator)


def main():
    random.seed(0)

    # --- Monte Carlo: confirm the estimator recovers the true d and that
    # its quoted uncertainty (eq. 116) matches the empirical spread. ---
    d_hats = []
    for _ in range(N_TRIALS):
        e = [random.gauss(0.0, math.sqrt(s)) for s in SIGMA_DIAG]
        p = [a * TRUE_D + ei for a, ei in zip(ALPHA, e)]
        d_hat, _ = wls_estimate(p, ALPHA, SIGMA_DIAG)
        d_hats.append(d_hat)

    aT_sigmainv_a = sum(a * a / s for a, s in zip(ALPHA, SIGMA_DIAG))
    sigma_dhat_theory = math.sqrt(1.0 / aT_sigmainv_a)  # eq. 116
    sigma_dhat_empirical = statistics.pstdev(d_hats)
    mean_dhat = statistics.mean(d_hats)

    print("=== WLS estimator: Monte Carlo vs. closed-form (eq. 106/107/115/116) ===")
    print(f"true d                 = {TRUE_D:.6f} degC")
    print(f"mean(d_hat), N={N_TRIALS}   = {mean_dhat:.6f} degC")
    print(f"sigma_d_hat (theory)   = {sigma_dhat_theory:.6f} degC   (eq. 116)")
    print(f"sigma_d_hat (empirical)= {sigma_dhat_empirical:.6f} degC")
    rel_err = abs(sigma_dhat_empirical - sigma_dhat_theory) / sigma_dhat_theory
    print(f"relative difference    = {rel_err*100:.3f}%  (expect <1% at this N)")
    print()

    # --- "Is the matrix stable" -- show the object actually being divided
    # by is a single positive scalar, not a conditioned matrix. ---
    print("=== What 'the matrix' actually is (eq. 106 for scalar d) ===")
    print(f"alpha^T Sigma^-1 alpha = {aT_sigmainv_a:.6f}  <- this scalar is 'the matrix'")
    print("Per-channel weight alpha_i^2/sigma_i^2 (relative contribution to the sum):")
    weights = [a * a / s for a, s in zip(ALPHA, SIGMA_DIAG)]
    total = sum(weights)
    for name, w in zip(MODE_NAMES, weights):
        print(f"  {name:5s}: weight={w:10.6f}  ({100*w/total:5.2f}% of total)")
    ratio = max(weights) / min(weights)
    print(f"max/min weight ratio   = {ratio:.1f}x  "
          f"(this is the real risk: one channel's sigma^2 dominates the fit,")
    print(f"                          not a numerical-conditioning problem)")
    print()

    # --- Iterative refinement (Numerical Recipes 2nd ed., sec 2.5): does
    # it find anything to correct? ---
    print("=== Iterative refinement (Numerical Recipes 2nd ed., sec. 2.5) ===")
    p_sample = [a * TRUE_D + 0.1 for a in ALPHA]  # arbitrary fixed residual vector
    d_hat, _ = wls_estimate(p_sample, ALPHA, SIGMA_DIAG)
    residual = [pi - a * d_hat for pi, a in zip(p_sample, ALPHA)]
    correction, _ = wls_estimate(residual, ALPHA, SIGMA_DIAG)
    print(f"d_hat (direct closed-form solve) = {d_hat!r}")
    print(f"one refinement step correction   = {correction!r}")
    print(f"|correction| = {abs(correction):.3e}  "
          f"(floating-point noise floor, not a real residual error to fix)")
    print()
    print("Conclusion: eq. 107 is an exact scalar division, not an iterative")
    print("linear solve -- there is no accumulated solver error for iterative")
    print("refinement to correct. The technique from Numerical Recipes sec. 2.5")
    print("is designed for ill-conditioned Ax=b with vector x; it has nothing")
    print("to act on here.")


if __name__ == "__main__":
    main()
