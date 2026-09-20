# Bench captures: what's here, and what to do with each

All files here are `script`-captured serial console sessions from the closed
loop (`input_select=1`) bench tests run during the trackq frequency-estimator
investigation. They're organized into three eras, corresponding to the state
of the firmware at capture time. See the doc "UberClock Estimator
Investigation Record" for the full narrative and analysis these captures
feed into.

## `01_pre_loopback_harness/` — before `LoopbackRetest.md`/`analyze_loopback_capture.py` existed

These predate the `k=`/`y=`/`den=`/`expected=` diagnostic fields and the
`trackq_dump` raw-sample capability. Analyze with a custom regex or by eye;
`analyze_loopback_capture.py` and `analyze_sweep_capture.py` require at
minimum the `k=`/`y=`/`den=` fields these mostly lack.

| File | What it is |
|---|---|
| `01_phase_nco_offset_test.txt` | The original `phase_nco` ±1 LSB test (10324439/10324441 vs baseline 10324440) that first revealed the interpolator jitter anomaly at one specific offset. No diagnostic fields. |
| `02_closedloop_baseline.txt` | The corrected closed-loop baseline (`phase_down_1=10323408`, `phase_nco=10324440`) after fixing the earlier `phase_down_1`/`phase_nco` mix-up. No diagnostic fields. |
| `03_bin_sweep_original_unfixed.txt` | The full `phase_down_1` = 10323406..10323412 (offset -2..+4) sweep on **completely unmodified** original firmware — no clamp, no search-window bound, no Hann, no log-power. This is the "before" baseline behind the bias numbers in doc Section 12. No diagnostic fields (bias/jitter were computed from raw `bb=` values by hand at the time). |
| `04_bin_sweep_after_clamp_searchbound_fix.txt` | The same sweep re-run after adding the windowed-argmax search bound and the half-bin correction clamp (both still in the code today). Has `k=`/`y=`/`den=` fields — this is the capture that first showed the `\|den\|/y2` scalloping-loss signature (doc Section 5). |
| `05_hann_only_uncompensated_REVERTED.txt` | The Hann-window-only experiment (no log-power), which **regressed** baseline jitter due to uncompensated coherent-gain loss and was reverted. Kept for the record — see doc Section 7's "why log-power, not just windowing." Has `k=`/`y=`/`den=` fields. |

## `02_loopback_pre_logpower/` — `LoopbackRetest.md` harness, before the Hann+log-power fix

Full field set (`k=`, `y=`, `den=`, `N=`, `window=`, `input=`, `pnco=`,
`pdown=`, `drained=`, `oldov=`, `ov=`, `uf=`, `expected=`). Firmware still
had the original linear-power (rectangular-window) interpolator. Analyze
with `analyze_loopback_capture.py <file>`.

| File | LoopbackRetest part | Register setting |
|---|---|---|
| `baseline_2048_midbin.txt` | Part A | `phase_down_1=10323408` (mid-bin) |
| `baseline_2048_boundary.txt` | Part A (repeated) | `phase_down_1=10323409` (bin-boundary, the noisy point) |
| `fft_length_64.txt` | Part C | N=64, same registers |
| `fft_length_256.txt` | Part C | N=256, same registers |
| `fft_length_1024.txt` | Part C | N=1024, same registers |

## `03_loopback_post_logpower/` — same harness, after the Hann+log-power fix shipped

Same field set and same register settings as above, captured after
`trackq_fft_peak_vertex_estimate_mhz()` was changed to use an on-the-fly
Hann window and a log-power parabolic fit. This is the "after" side of every
before/after comparison in the doc (Sections 8, 9, 12).

| File | Corresponds to | Register setting |
|---|---|---|
| `baseline_2048_midbin.txt` | pre-logpower `baseline_2048_midbin.txt` | `phase_down_1=10323408` |
| `baseline_2048_boundary.txt` | pre-logpower `baseline_2048_boundary.txt` | `phase_down_1=10323409` |
| `fft_length_64.txt` | pre-logpower `fft_length_64.txt` | N=64 |
| `fft_length_256.txt` | pre-logpower `fft_length_256.txt` | N=256 |
| `fft_length_1024.txt` | pre-logpower `fft_length_1024.txt` | N=1024 |
| `full_sweep_2048.txt` | `01_pre_loopback_harness/03_bin_sweep_original_unfixed.txt` | Full offset -2..+4 sweep, one continuous capture |

A near-duplicate of `full_sweep_2048.txt` (`post_logpower_analyze.txt`,
identical register sequence, one line shorter) was dropped rather than kept
as clutter.

## How to reproduce a capture

```bash
cd 2.soc-litex/3.build
script -q captures/<era>/<name>.txt
make term PORT=/dev/ttyUSB0
```
Then run the exact console commands documented in `LoopbackRetest.md` (for
era 2/3 captures) or in the doc's Section 5 (for era 1's manual sweeps).

## How to analyze

```bash
# single-setting capture with a trackq_dump block (Part A style):
python3 ../analyze_loopback_capture.py captures/03_loopback_post_logpower/baseline_2048_midbin.txt

# multi-setting sweep, no dump needed, produces plots:
python3 ../analyze_sweep_capture.py captures/03_loopback_post_logpower/full_sweep_2048.txt --save /tmp/sweep
```

Files in `01_pre_loopback_harness/` predate both scripts' required fields
and need to be read manually or reprocessed with a one-off regex.
