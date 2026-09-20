# Internal loopback retest: fresh acquisition

This firmware is **measurement-only**: `trackq_start` measures channel-1 Y;
it does not run the crystal tracker, WLS temperature calculation, or output
correction controller. `input_select 1` selects the internal NCO. No gateware
changes are required for this patch; the existing FIFO status CSRs are used.

## Build and identify the loaded binary

From the repository root:

```sh
make -C 2.soc-litex/3.build build-sw
```

Use `build-sw`, which selects the correct code/data memory regions; invoking
the software Makefile directly with the untouched linker template is not the
equivalent full build. The outputs are `2.soc-litex/2.sw/demo.bin` and
`demo.fbi`. Load the binary using the usual workflow. For the serial loader:

```sh
make -C 2.soc-litex/3.build term PORT=/dev/ttyUSB0
```

Use the appropriate serial device and reset the board to enter its serial
bootloader if needed. Start terminal logging. Check the boot/startup output
contains **`build=loopback-fifo-v1`**, the compilation date/time, and
**`window=rect`**. Do not compare against an unidentified or Hann build.

The helper converting requested Hz to a DDS tuning word now rounds to nearest.
Therefore boot defaults / frequency-in-Hz commands can differ by one tuning
word from older builds. These tests explicitly set raw tuning words, so their
expected frequencies do not change. The first three zero arguments to
`trackq_start` preserve the existing mixer settings.

## A. Baseline and raw capture (about 90 seconds)

Send these console commands one at a time:

```text
trackq_stop
input_select 1
fft_fs 10000
phase_nco 10324440
phase_down_1 10323408
trackq_start 0 0 0 2048 1000 10 100 20
```

Record at least 60 validation lines, including the first ones. Then:

```text
trackq_dump
```

The dump command stops validation and prints the **last completed raw capture**,
with `sample,y1` rows between begin/end markers. Keep the entire dump in the log.
Save as `fresh_baseline_2048.txt`.

Expected nominal beat: **999.569893 Hz**. The printed `expected` field rounds
this to 999.570 Hz. The known rectangular-window power-parabola may report
roughly 1000.82 Hz; this is a diagnostic bias, not a pass requirement that the
printed BB must equal the true beat. Windowing and estimator corrections are
deliberately held fixed while checking acquisition.

## B. Step response without restarting (about 90 seconds)

Restart the same 2048-point command. Keep 20 readings at each step:

```text
trackq_start 0 0 0 2048 1000 10 100 20
```

After 20 readings:

```text
phase_down_1 10323409
```

After 20 more readings:

```text
phase_down_1 10323408
```

After 20 more readings:

```text
trackq_dump
```

Save as `fresh_step_2048.txt`, retaining the command echoes. The expected beat
changes from **999.569893 to 998.601317 Hz and back**. Every line logs `pnco`
and `pdown`; a seven-record plateau of the previous setting should no longer
occur. A short physical filter transient is distinct from the previous seven
stale FFT blocks. Keep all readings so any remaining transient is visible.

## C. FFT length comparison

Keep `phase_nco 10324440`, `phase_down_1 10323408`, and `fft_fs 10000`.
For each command, record at least 60 lines, then run `trackq_dump` to stop and
save the raw record. Use a separate log per length.

```text
trackq_start 0 0 0 64 1000 10 100 20
```

```text
trackq_start 0 0 0 256 1000 10 100 20
```

```text
trackq_start 0 0 0 1024 1000 10 100 20
```

Expected nominal beat stays **999.569893 Hz**. At N=64 a substantial bias and
phase-dependent scatter can occur even for an ideal real sinusoid with this
estimator. The raw-sample sine fit below is the more useful accuracy check.

## What to check

- `N`, `window=rect`, `input=1`, `pnco`, and `pdown` must match the test.
- Successful captures must have `ov=0 uf=0`. A new overflow/underflow causes
  `capture rejected:` and stops validation rather than publishing a bad FFT.
- `drained` counts discarded stale frames. Large values are expected because
  the foreground only acquires periodically.
- `oldov=1` is allowed: it records overflow from **before** the drain, while
  idle/printing/computing. It is cleared before settling/acquisition. It is
  different from `ov=1` during the new record.
- A drain-limit, clear, or timeout error is an acquisition failure. Save the
  message; do not fold invalid records into frequency statistics.
- The first `dticks` is zero. Later values measure serviced `ce_down` interrupt
  ticks between capture completions. They include work/capture cadence and
  are not an independent wall-clock or sample-continuity measurement: the ISR
  masks its event until foreground servicing re-enables it. Use terminal/host
  timestamps for real elapsed time. There is no hardware sample sequence ID
  in this patch.

## Analyze the log without additional packages

```sh
python3 2.soc-litex/3.build/analyze_loopback_capture.py fresh_baseline_2048.txt
```

The script prints mean, bias, population standard deviation, and invalid count
per `(N,pnco,pdown,input)` group. It includes all records; it does not silently
discard startup data. Separate forward/reverse runs if you need their means
compared independently.

For each complete raw dump it also computes:

1. A double-precision rectangular-window power-parabola estimate.
2. A fitted sinusoid with free frequency, phase, amplitude and DC offset. It
   finds the spectral peak from the samples, without using the tuning-word
   frequency as the answer or search centre.
3. Residual RMS in ADC/sample units. Harmonics, clipping, gaps, transients or
   multiple tones can violate the single-sinusoid model; inspect residuals and
   repeat captures rather than treating its result as ground truth.

Interpretation:

| Observation | Next investigation |
|---|---|
| Float parabola agrees with firmware; sine fit agrees with expected beat | Estimator bias; compare improved estimators offline using the same samples. |
| Float parabola differs materially from firmware on the same record | Fixed-point FFT/scaling/rounding. |
| Sine fit also disagrees, or has large residuals | Acquisition, actual routing/tuning words, filter transients, distortion or missing samples. |
| Capture rejected | FIFO throughput/status/clocking first; estimator tuning cannot fix a bad record. |

This loopback validates the digital path in nominal-clock units. It cannot
validate absolute board-clock frequency, physical crystal temperature, or the
external-reference compensation loop.

## Local checks

```sh
python3 2.soc-litex/2.sw/tests/test_loopback.py
```

The tests compile the production capture/estimator functions against a mocked
FIFO and actual 16-bit KISS FFT with undefined-behavior checks. Board timing,
clock-domain crossings and the loaded gateware still require the retest above.
