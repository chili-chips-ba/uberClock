# Closed-loop test analysis

Generated from `test1/*.txt` and `test2/*.txt`. Reproduce the CSV files with:

```sh
python3 2.soc-litex/3.build/wls_validation/analyze_loopback_tests.py
```

No firmware was changed and no board commands were executed.

## Method and limits

- Expected frequency is `(10324440 - phase_down_1) * 65000000 / 2**26`,
  expressed in nominal-clock Hz. Settings are inferred from commands, startup
  messages, and filenames, not independently measured register snapshots.
- Test 1 excludes the first seven acquisition records in each file. The first
  file has seven invalid records; subsequent files retain the previous setting
  for seven records. The eighth record shows the new plateau.
- Test 2 uses the last 50 records of each configuration segment (or all if fewer
  exist). These are descriptive late-run statistics, not proof of continuity.
- `1024.txt` also contains a second start at N=2048 and two readings. These are
  separated in the CSV; they must not be included in N=1024 statistics.
- Standard deviations use the population definition. Millihertz output
  quantization is present. No statistical independence is assumed.
- `records.csv` retains startup records, invalid flags, diagnostic powers, and
  nominal elapsed time from cumulative dticks, excluding the first interval.

## Findings

1. **Stale data:** Test 1 repeatedly reports the old setting for seven updates.
   The current gateware source has a 16,384-frame DS FIFO. Capture consumes
   `N + 256` frames each update, with no drain-to-empty in the multi-channel
   capture function. At N=2048, `16384 / 2304 = 7.11` updates. This is a strong
   explanation for the observed plateau; loaded gateware identity is unverified.

2. **Potential discontinuities:** Source writes only while FIFO is writable and
   flags overflow otherwise, dropping new samples. At roughly one update per
   second, the reader removes much less than the 10,000 frames/s produced.
   Captures can therefore encounter segments separated by missing samples.
   The logs do not contain sample sequence IDs, so continuity cannot be proved
   or individual gaps localized. Similar backlog-clearance scales are 51.2
   updates at N=64, 32 at N=256, and 12.8 at N=1024. The latter run changes from
   alternating estimates near 999/1002 Hz to a stable 996.163 Hz around this scale.

3. **Likely loaded-build mismatch:** All seven Test 1 means are within 6.4 mHz
   of an ideal Hann-window, three-bin power-parabola model. At the baseline,
   measured mean is 1000.119823 Hz; Hann predicts 1000.119073 Hz, while a
   rectangular window predicts 1000.822316 Hz. Current source applies no window.
   This strongly suggests an older/different loaded build, but is not proof.
   The model uses a zero-phase cosine and double arithmetic; it excludes the
   actual fixed-point pipeline and FIFO behavior.

4. **Accuracy versus precision:** Test 1 biases reach about 0.567 Hz despite
   standard deviations of 2.66–18.90 mHz. Averaging these records does not remove
   the systematic frequency-dependent bias.

5. **Original sawtooth not reproduced:** Late N=64 readings span approximately
   951.321–961.967 Hz, with mean 956.209 Hz versus expected 999.570 Hz. This
   demonstrates a large measurement error, but does not reproduce Figure 1's
   roughly 400 Hz repetitive sweep. Larger FFTs also change bias and scatter;
   build/window uncertainty and acquisition effects confound a pure FFT-length
   comparison.

## Recommended next experiment

First establish the flashed firmware/gateware identity and print actual window
selection, FFT length and tuning words. Then instrument sample acquisition:

- Drain old frames before each capture; log the drained count and verify empty
  was reached rather than merely exhausting a fixed flush limit.
- Clear overflow status after draining, allow/discard the intended filter
  settling samples, and acquire a fresh contiguous N-frame record.
- Add a monotonically increasing source sample ID per frame, if feasible, and
  reject records with non-unit sample-ID differences. Log overflow alongside it.
- Do not assume `ds_fifo_clear` empties the FIFO: in the current source it clears
  status flags. Do not assume longer waiting flushes anything.
- Repeat N=2048 at baseline and the noisy setting, then N=64, using the same
  identified window configuration. Inspect raw records with a host estimator.

No need to retune the controller or change WLS weights to diagnose these issues.

## Source locations

- `2.soc-litex/8.python/src/uberclock_soc/uberclock_core.py`: DS FIFO depth,
  write enable, and overflow handling near lines 368–406.
- `2.soc-litex/2.sw/uberclock.c`: `capture_ds_track_multi` near line 756;
  no-window FFT loading near line 1073; 256-frame settling constant near line 527.
- Parabolic interpolation is generally biased away from special symmetry
  positions: https://dsprelated.com/freebooks/sasp/Bias_Parabolic_Peak_Interpolation.html
