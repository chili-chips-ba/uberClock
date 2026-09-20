# WLS / Temperature-Estimator Validation (software-only, no board required)

This directory answers questions raised in the 2026-08-31 meeting notes that
are pure mathematics/numerics questions about the temperature-estimation
pipeline in `ASecondTryAtThis.pdf`, plus one later question about whether a
hardware jitter change is practically significant. None of the scripts here
need the FPGA board — they operate on closed-form equations, the real
numeric coefficients already derived in the paper (`α` from eq. 194, `Σ`
from eq. 195), and, for the last script, real jitter figures already
measured on the bench (see the main investigation record for how those were
obtained).

Run any script with plain `python3` (standard library only — `random`,
`statistics`, `math`). No numpy dependency, since `Σ` is diagonal and every
formula below reduces to per-channel scalar arithmetic.

```bash
python3 wls_stability_test.py
python3 taylor_truncation_test.py
python3 ppm_hz_equivalence_test.py
python3 jitter_carrier_ppm_test.py
```

---

## 1. `wls_stability_test.py` — is the WLS "matrix" actually unstable?

**Meeting question:** *"How stable is the matrix? We have diagonal values
changing within 4 orders of magnitude — is that matrix solution stable?
Should we do iterative improvement of a solution to linear equations
(Numerical Recipes)?"*

**The equations being tested** (paper §1.17–1.19, §1.23):

- Linear model: `p = α d + e`, `e ~ N(0, Σ)` (eq. 136)
- WLS estimate: `d̂ = (αᵀΣ⁻¹α)⁻¹ αᵀΣ⁻¹ p` (eq. 106, 137)
- For diagonal Σ, this reduces to a **weighted scalar average**, no matrix
  inversion in the linear-algebra sense (eq. 107):

  ```
  d̂ = Σᵢ(αᵢpᵢ/σᵢ²) / Σᵢ(αᵢ²/σᵢ²)
  ```

- Estimator variance (eq. 115/116): `σ_d̂² = (αᵀΣ⁻¹α)⁻¹`
- Real coefficients used (eq. 194, 195):

  ```
  α  = [-1.171846, -29.366872, -34.323755, -0.358644, -25.278849]   ppm/°C
  Σ_diag = [202.3051, 4836.9699, 2796.5879, 2.2603, 631.2718]        ppm²
  ```

**Why "the matrix" can't be ill-conditioned here:** the unknown `d` is a
**scalar**, so the object being "inverted" — `αᵀΣ⁻¹α` — is a single positive
number: a sum of non-negative terms `αᵢ²/σᵢ²`. A sum of non-negative terms
can only be ill-conditioned (near zero) if every term is near zero, i.e. if
every `αᵢ ≈ 0` — the sensor would have to be totally insensitive to
temperature everywhere, which it isn't. Genuine ill-conditioning is a
property of solving `Ax=b` for a vector `x` when `AᵀA` has a large ratio
between its largest and smallest eigenvalues (Golub & Van Loan, *Matrix
Computations*, §5.3); with `x` one-dimensional, there is only one eigenvalue,
so the concept doesn't apply.

**What the script does:**
1. Monte Carlo: draws `e ~ N(0, Σ)` (independent per channel, since Σ is
   diagonal) for a chosen true `d`, forms `p = αd + e`, computes `d̂` via
   eq. 107, over 200,000 trials.
2. Checks the empirical `mean(d̂)` and `std(d̂)` against the closed-form
   `d` and `σ_d̂` from eq. 115 — confirms the estimator is unbiased and its
   quoted uncertainty is correct.
3. Performs one step of **iterative refinement** (Press et al., *Numerical
   Recipes in C*, 2nd ed., §2.5, "Iterative Improvement of a Solution to
   Linear Equations" — the exact technique named in the meeting) on the
   *already-computed* `d̂`: forms the residual `r = p − αd̂`, solves the
   correction `δ = (αᵀΣ⁻¹α)⁻¹αᵀΣ⁻¹r`, and shows `δ ≈ 0` to floating-point
   precision. This demonstrates directly, rather than by argument, that
   there is no residual solver error for iterative refinement to correct —
   the one-line closed-form division in eq. 107 is already the exact
   solution, not an approximate iterative one.
4. Prints the actual per-channel weight `αᵢ²/σᵢ²` and each channel's share
   of the total. The real numbers show **B300 contributes ~60% of the total
   weight and A100 ~25%** — together 85% of the estimate — while C300,
   despite having by far the smallest `σ²` (2.26 ppm²), contributes only
   ~3.4%, because its temperature sensitivity `α_C300 = −0.359 ppm/°C` is
   tiny compared to `α_B300 = −25.3 ppm/°C`. (An earlier verbal claim in
   this investigation — that C300's small variance made it dominate the
   fit — was wrong, because it compared `σ²` alone without accounting for
   `α`; running the actual numbers here caught that.) The real risk buried
   in the "4 orders of magnitude" observation is that B100 and A100's
   `σ²` (both fitted from only 8 residual d.o.f., eq. 153, from a 10-point
   sweep) need to be right, not C300's.

**Sources:**
- Björck, Å. *Numerical Methods for Least Squares Problems*. SIAM, 1996. —
  standard reference for WLS conditioning and iterative refinement of
  least-squares problems specifically.
- Press, W.H., Teukolsky, S.A., Vetterling, W.T., Flannery, B.P.
  *Numerical Recipes in C: The Art of Scientific Computing*, 2nd ed.,
  Cambridge University Press, 1992 — §2.5 (iterative improvement),
  §15.4 (general linear least squares). This is the book named in the
  meeting notes.
- Golub, G.H., Van Loan, C.F. *Matrix Computations*, 4th ed., Johns Hopkins
  University Press, 2013 — §5.3, conditioning of least-squares problems.
- Draper, N.R., Smith, H. *Applied Regression Analysis*, 3rd ed., Wiley,
  1998 — standard textbook derivation of WLS estimator variance (eq. 115
  is a standard result, ch. 2 & 5).

---

## 2. `taylor_truncation_test.py` — does the linearization explain the sawtooth?

**Meeting question:** *"Maybe take a first-order approach when it wasn't —
approximating, truncating a Taylor series for eq. 134?"*

**The equations being tested** (paper §1.5):

- Exact reciprocal: `1/(1+ε)`, Taylor-expanded: `1 − ε + ε² − ε³ + …` (eq. 28)
- First-order approximation: `1/(1+ε) ≈ 1 − ε` (eq. 29)
- Exact linearization error: `e_rec = 1/(1+ε) − (1−ε) = ε²/(1+ε)` (eq. 34)
- Absolute frequency error: `e_lin = f · ε²/(1+ε)` (eq. 35)
- Paper's worked example: at `ε = 30×10⁻⁶` and `f = 10 MHz`,
  `e_lin ≈ 9×10⁻³ Hz` (eq. 36–38)

**Why the comparison must be done in relative (ppm) units, not absolute Hz:**
eq. 35's `e_lin` is an *absolute* Hz error at whatever carrier frequency
`f` you plug in. Comparing it directly against jitter measured on a
~1 kHz baseband tone would silently compare two different stages of the
pipeline at a 10,000x different scale and prove nothing. eq. 34's `e_rec`
is the *relative* (dimensionless/ppm) error, which is the same kind of
quantity as `Σ` (ppm²) and the ppm-deviation vector `p` used everywhere
else in the pipeline — that's the fair, apples-to-apples comparison.

**What the script does:**
1. Reproduces the paper's exact worked example bit-for-bit (`ε=30ppm` →
   `e_lin≈9mHz` at 10 MHz), confirming the derivation in the PDF is
   arithmetically correct.
2. Reads this board's **own actual measured clock-error range** directly
   from the paper's own results table (eq. 147's `F` column: 64,999,390 to
   65,001,943 Hz against `F0=65,000,000`), giving `ε ∈ [−9.4, +29.9] ppm` —
   confirming the paper's `ε=30ppm` worked example wasn't arbitrary, it's
   this board's real observed range.
3. Computes `e_rec` (eq. 34, relative/ppm units) at that real range and
   compares it against every real ppm-level noise floor already
   established in this project: all five `σ` values from Table 1
   (14.2–69.5 ppm), and the bin-boundary FFT-interpolator jitter measured
   on the bench (0.15–21.6 ppm, expressed as jitter/signal). Result:
   `e_rec ≈ 0.0009 ppm` at this board's worst measured clock error — **3–5
   orders of magnitude smaller** than every one of those real noise
   sources. This closes out the "is the Taylor truncation the source of
   the sawtooth" question with a like-for-like, unit-consistent
   comparison: it cannot be, at any clock error this board has actually
   shown.

**Sources:**
- Burden, R.L., Faires, J.D. *Numerical Analysis*, 9th ed., Brooks/Cole,
  2010 — Taylor's theorem with remainder (ch. 1), the standard reference
  for bounding truncation error of a linearization.
- The paper's own §1.5 derivation (eq. 28–38), which this script verifies
  numerically rather than re-deriving.

---

## 3. `ppm_hz_equivalence_test.py` — does working in ppm change the answer?

**Meeting question:** *"Why are we working in ppm values? Would working
with Hz be better?"*

**The equations being tested** (paper §1.20–1.21, §1.23):

- ppm deviation: `pᵢ = 10⁶ · (M_corr,i − M_n,i) / M_n,i` (eq. 121, 135)
- Division-free precomputed constant: `Kᵢ = 10⁶ / M_n,i` (eq. 123)
- Same WLS estimator (eq. 106/107) applied either to the ppm-scaled vector
  `p` (with `α` in ppm/°C, `Σ` in ppm²) or directly to the raw Hz-domain
  deviations (with `α` and `Σ` rescaled by the same per-channel factor
  `Kᵢ`).

**Why this must give the identical answer:** eq. 107 is a per-channel
weighted average. Rescaling channel `i`'s data, its sensitivity `αᵢ`, *and*
its variance `σᵢ²` by the same constant `Kᵢ` leaves the weight
`αᵢ²/σᵢ²` and the numerator term `αᵢpᵢ/σᵢ²` both invariant, because `Kᵢ`
appears once in the numerator (`αᵢ→Kᵢαᵢ`, `pᵢ→Kᵢpᵢ`, giving `Kᵢ²` in the
numerator term) and `Kᵢ²` in the denominator weight (`σᵢ²→Kᵢ²σᵢ²`) — they
cancel exactly. This is the standard **scale-invariance of weighted least
squares** under per-observation rescaling (Draper & Smith, ch. 2).

**What the script does:**
1. Builds a synthetic Hz-domain dataset (`M_m,i`, `M_n,i` for the 5 real
   modes from eq. 133/eq. 194's mode list) with a known injected `d`.
2. Computes `d̂` two ways: (a) the paper's actual pipeline — convert to ppm
   via eq. 121/123 first, then WLS with `α` in ppm/°C and `Σ` in ppm²; (b)
   directly in Hz — rescale `α` and `Σ` by `Kᵢ` to undo the ppm conversion,
   then WLS with everything in Hz.
3. Confirms the two results agree to floating-point precision (differences
   only at the ~10⁻¹² relative level from IEEE-754 non-associativity of
   floating-point multiply/divide — not a real discrepancy). Formally
   closes the question: ppm vs. Hz is a units choice for human readability
   and precomputed-constant convenience (eq. 123–130), not a source of
   numerical difference, and cannot be responsible for any of the observed
   sawtooth or jitter.

**Sources:**
- Draper, N.R., Smith, H. *Applied Regression Analysis*, 3rd ed., Wiley,
  1998 — ch. 2, scale invariance of (weighted) least squares under linear
  rescaling of variables.
- IEEE Std 754-2019, *IEEE Standard for Floating-Point Arithmetic* — basis
  for the floating-point-precision caveat on "bit-identical."

---

## 4. `jitter_carrier_ppm_test.py` — is the post-fix N=2048 jitter increase a real problem?

**Question:** after porting a Hann-window + log-power interpolator to
firmware (see the main investigation record), jitter at the N=2048
bin-boundary point improved (19.3 → 6.7 mHz) but jitter at the N=2048
mid-bin point got worse (1.5 → 8.3 mHz). Is that regression practically
significant?

**Why the obvious comparison is wrong:** the closed-loop bench setup
(`input_select=1`) downconverts an internally generated ~10 MHz reference
tone by a known, exact, jitter-free commanded LO to a ~1 kHz baseband IF,
purely to get finer *relative* frequency resolution from a fixed-length FFT
than measuring directly at 10 MHz would give. Absolute Hz jitter measured
on that ~1 kHz baseband is the *same* absolute Hz jitter on the
reconstructed ~10 MHz reference — so converting it to ppm must divide by
the real ~10 MHz carrier, not the ~1 kHz IF it happens to be measured at.
Dividing by 1 kHz instead turns a negligible number into a
10,000x-inflated, alarming-looking one — the same category of unit mistake
caught and corrected in `taylor_truncation_test.py`.

**What the script does:** converts both the pre-fix and post-fix jitter
figures (at the mid-bin and boundary points) to ppm using the correct ~10
MHz carrier, then compares against every real per-mode noise floor already
established in Table 1 (`σ` from 1.5 to 69.5 ppm).

**Result:** using the wrong (1 kHz) carrier, the mid-bin jitter looks like
it went from 1.53 ppm to 8.32 ppm — an uncomfortable-looking 5.4x
regression. Using the correct (10 MHz) carrier, it's **0.000153 ppm →
0.000832 ppm** — both numbers are 1,800x to 84,000x smaller than the
*tightest* real noise floor already in this project (σ_C300 = 1.50 ppm).
The regression is real but does not move the needle on the system's actual
achievable precision.

**Sources:**
- Same unit-consistency principle as `taylor_truncation_test.py`; see that
  section's rationale for why relative (ppm) comparison, not absolute Hz,
  is the only valid way to judge significance across signals at different
  carrier frequencies.
