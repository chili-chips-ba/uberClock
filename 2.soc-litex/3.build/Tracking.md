# 3 Tone tracking 

In this file the tracking of the three tones/modes of our oscillator is presented. Each tone can drift in frequnecy as time passes, or temperature and other parameters change. 
The challange is to lock onto these modes and always track how and where they move. To discover and roughly find the resonant frequency of single modes a `track3` [algortihm](https://chili-chips-ba.github.io/uberClock/examples/tracking_algorithm.html) is used.
It positions the CPU genarated tones and the CORDIC mixer frequencies near the resonant frequnecy so that `trackQ` [algortihm](https://chili-chips-ba.github.io/uberClock/examples/tracking_algorithm.html) can take over from that starting point.  

## C300 Mode

From our lab data this tone is expected to be found in frequency range: 10.003840 to 10.004000 MHz. 
Using the serial console inteface of the algorithm: `track3 <ch> <start_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]`
We try and find this tone in few iterations. 

Firts iteration:
```
uberClock> track3 1 10002840 20 10 2048 1000 20
track3: ch=1 start=10002840 Hz step=20 Hz max_steps=10 N=2048 center=1000 Hz delta=20 Hz Fs=10000 Hz sig3={980,1000,1020} Hz
track3 step=0 phase_down_1=10002840 Hz inc=10327372 bins={201,205,209} pwr={3535,554,323}
track3 step=1 phase_down_1=10002860 Hz inc=10327393 bins={201,205,209} pwr={188,114,66}
track3 step=2 phase_down_1=10002880 Hz inc=10327414 bins={201,205,209} pwr={51,33,42}
track3 step=3 phase_down_1=10002900 Hz inc=10327434 bins={201,205,209} pwr={18,16,25}
track3 step=4 phase_down_1=10002920 Hz inc=10327455 bins={201,205,209} pwr={15,25,25}
track3 step=5 phase_down_1=10002940 Hz inc=10327476 bins={201,205,209} pwr={20,16,13}
track3 step=6 phase_down_1=10002960 Hz inc=10327496 bins={201,205,209} pwr={16,32,17}
track3 lock: phase_down_1=10002960 Hz inc=10327496 center=1000 left=980 right=1020
```
Then the second and final iteration:
```
uberClock> track3 1 10002940 1 20 2048 1000 10
track3: ch=1 start=10002940 Hz step=1 Hz max_steps=20 N=2048 center=1000 Hz delta=10 Hz Fs=10000 Hz sig3={990,1000,1010} Hz
track3 step=0 phase_down_1=10002940 Hz inc=10327476 bins={203,205,207} pwr={60067,136499,18232}
track3 step=1 phase_down_1=10002941 Hz inc=10327477 bins={203,205,207} pwr={75089,110155,16630}
track3 step=2 phase_down_1=10002942 Hz inc=10327478 bins={203,205,207} pwr={67740,57706,12101}
track3 step=3 phase_down_1=10002943 Hz inc=10327479 bins={203,205,207} pwr={74213,33618,10973}
track3 step=4 phase_down_1=10002944 Hz inc=10327480 bins={203,205,207} pwr={94839,25360,12059}
track3 step=5 phase_down_1=10002945 Hz inc=10327481 bins={203,205,207} pwr={129631,16819,4922}
track3 step=6 phase_down_1=10002946 Hz inc=10327482 bins={203,205,207} pwr={4352,25171,68193}
track3 step=7 phase_down_1=10002947 Hz inc=10327483 bins={203,205,207} pwr={6651,34312,72923}
track3 step=8 phase_down_1=10002948 Hz inc=10327484 bins={203,205,207} pwr={9542,69438,91629}
track3 step=9 phase_down_1=10002949 Hz inc=10327485 bins={203,205,207} pwr={8819,71914,90698}
track3 step=10 phase_down_1=10002950 Hz inc=10327486 bins={203,205,207} pwr={8676,69903,84677}
track3 step=11 phase_down_1=10002951 Hz inc=10327487 bins={203,205,207} pwr={10158,62017,76351}
track3 step=12 phase_down_1=10002952 Hz inc=10327488 bins={203,205,207} pwr={15947,54087,63095}
track3 step=13 phase_down_1=10002953 Hz inc=10327489 bins={203,205,207} pwr={24303,54736,49692}
track3 lock: phase_down_1=10002953 Hz inc=10327489 center=1000 left=990 right=1010
```



## A100 Mode

From our lab data this tone is expected to be found in frequency range: 6.261641 to 6.271641 MHz.

Iteration for this mode:

```
uberClock> track3 2 6261000 1000 10 2048 1000 100
track3: ch=2 start=6261000 Hz step=1000 Hz max_steps=10 N=2048 center=1000 Hz delta=100 Hz Fs=10000 Hz sig3={900,1000,1100} Hz
track3 step=0 phase_down_2=6261000 Hz inc=6464132 bins={184,205,225} pwr={0,108,0}
track3 step=1 phase_down_2=6262000 Hz inc=6465164 bins={184,205,225} pwr={0,74,0}
track3 step=2 phase_down_2=6263000 Hz inc=6466197 bins={184,205,225} pwr={0,106,0}
track3 step=3 phase_down_2=6264000 Hz inc=6467229 bins={184,205,225} pwr={0,93,0}
track3 step=4 phase_down_2=6265000 Hz inc=6468262 bins={184,205,225} pwr={0,99,0}
track3 step=5 phase_down_2=6266000 Hz inc=6469294 bins={184,205,225} pwr={0,105,0}
track3 step=6 phase_down_2=6267000 Hz inc=6470326 bins={184,205,225} pwr={0,95,0}
track3 step=7 phase_down_2=6268000 Hz inc=6471359 bins={184,205,225} pwr={77,95,97}
track3 step=8 phase_down_2=6269000 Hz inc=6472391 bins={184,205,225} pwr={96,69,121}
track3 step=9 phase_down_2=6270000 Hz inc=6473424 bins={184,205,225} pwr={105,130,67}
track3 lock: phase_down_2=6270000 Hz inc=6473424 center=1000 left=900 right=1100


```

## C100 Mode
From our lab data this tone is expected to be found in frequency range: 3.388438 to 3.388594MHz.

This is mode is found as:

```
uberClock> track3 3 3387400 10 10 2048 1000 20
track3: ch=3 start=3387400 Hz step=10 Hz max_steps=10 N=2048 center=1000 Hz delta=20 Hz Fs=10000 Hz sig3={980,1000,1020} Hz
track3 step=0 phase_down_3=3387400 Hz inc=3497301 bins={201,205,209} pwr={14621,15243,11202}
track3 step=1 phase_down_3=3387410 Hz inc=3497311 bins={201,205,209} pwr={13495,13321,9311}
track3 step=2 phase_down_3=3387420 Hz inc=3497321 bins={201,205,209} pwr={14570,10784,7988}
track3 step=3 phase_down_3=3387430 Hz inc=3497331 bins={201,205,209} pwr={12620,9838,7987}
track3 step=4 phase_down_3=3387440 Hz inc=3497342 bins={201,205,209} pwr={10305,8390,8350}
track3 step=5 phase_down_3=3387450 Hz inc=3497352 bins={201,205,209} pwr={10714,9075,7482}
track3 step=6 phase_down_3=3387460 Hz inc=3497362 bins={201,205,209} pwr={8867,9387,6490}
track3 lock: phase_down_3=3387460 Hz inc=3497362 center=1000 left=980 right=1020
```


# Tracking

Leaning on the found tones and the CLI interface: `trackq_start <C300_mixer_hz> <A100_mixer_hz> <C100_mixer_hz> [N] [center_hz] [delta_ch1_hz] [delta_ch2_hz] [delta_ch3_hz]`,
we start the tracking and print out the state every 5 seconds. All frequnecies are rough estimations of the current mixer frequnecy value + the 1kHz offset that is chosen as the point of the vertex.

```
uberClock> trackq_start 10002953 6270000 3387460 2048 1000 10 100 20
trackq_start: ch1=10002953 Hz ch2=6270000 Hz ch3=3387459 Hz N=2048 center=1000 Hz delta={10,100,20} Hz sig3={{990,1000,1010},{900,1000,1100},{980,1000,1020}} Hz interval=2 s
trackq hf vertex: ch1=10003952.591Hz ch2=6270999.743Hz ch3=3388454.227Hz
trackq hf vertex: ch1=10003952.591Hz ch2=6270989.468Hz ch3=3388449.384Hz
trackq hf vertex: ch1=10003952.591Hz ch2=6270977.848Hz ch3=3388445.509Hz
trackq hf vertex: ch1=10003951.622Hz ch2=6270966.016Hz ch3=3388444.541Hz
trackq hf vertex: ch1=10003950.059Hz ch2=6270960.719Hz ch3=3388443.572Hz
trackq hf vertex: ch1=10003949.441Hz ch2=6270955.188Hz ch3=3388442.604Hz
trackq hf vertex: ch1=10003949.540Hz ch2=6270953.199Hz ch3=3388441.635Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270951.865Hz ch3=3388441.635Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270945.502Hz ch3=3388437.761Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270945.502Hz ch3=3388436.792Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270945.502Hz ch3=3388436.792Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270941.062Hz ch3=3388435.824Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270935.811Hz ch3=3388434.855Hz


...after some time...

trackq hf vertex: ch1=10003949.685Hz ch2=6270890.293Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270885.451Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270889.236Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270880.839Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270877.096Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270871.984Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270871.891Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270870.922Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270869.953Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270869.953Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270868.985Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270868.985Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270868.985Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270868.985Hz ch3=3388412.578Hz
```


 Using the [lowspeed debug signal capture](https://chili-chips-ba.github.io/uberClock/examples/capture_low_speed.html) we can look at the three tones in three channels, shown in the pictures below. We observe that [the algorithm](https://chili-chips-ba.github.io/uberClock/examples/capture_low_speed.html) will try and stay near the interpolated vertex with the central 1kHz tone.  

 
<img width="1295" height="555" alt="image" src="https://github.com/user-attachments/assets/82087cb3-5908-431e-bf4a-6906a9e5142a" />
<img width="1295" height="555" alt="image" src="https://github.com/user-attachments/assets/40db3eaf-0d2a-4aa4-aaa0-b3759c2dc776" />
<img width="1295" height="555" alt="image" src="https://github.com/user-attachments/assets/994bdcf5-0955-47ed-9a82-b9ecc0bc589a" />


All three modes (9 tones) can be seen coming out of the XTAL into the FPGA board with [high-speed debug capture](https://chili-chips-ba.github.io/uberClock/examples/capture_high_speed_ddr3.html).

<img width="1139" height="1023" alt="image" src="https://github.com/user-attachments/assets/3cdd1d4f-3b93-4424-8c77-231ab8801f41" />


 


### Reference measurement update (2026-10-03)

`trackq` now measures Y_ref with the validated Hann-window/log-power FFT64
estimator. The 64 samples are taken from the center of the shared modal capture
(indices 992–1055 for N=2048). The reference window therefore shares the modal
record's center, but has a shorter duration. `trackq_start` requires N >= 64.
Its startup message identifies the estimator; existing `ref`, `fs`, temperature,
and NCO diagnostic fields retain their meanings.

Reference recovery and the existing reference-driven output NCO correction are
unchanged. Modal regulation, modal bin fitting, and all temperature estimators
are unchanged. The standalone `ref_fft_start` centered FFT64/FFT2048 comparison
remains available. The old rectangular-window reference interpolator was removed
because it no longer has callers. This update has not been built or tested on
hardware; the next run should compare reference, fs, and temperature diagnostics
through several board-clock modulation cycles.

### Observer diagnostics: board-clock information in modal detuning (2026-10-04)

`trackq_observer_diag 0|1` enables coherent per-capture observer-v1 records,
independently of `trackq_diag`. Default is off. It snapshots LO and tone
increments before capture, reuses modal powers, reports unfiltered detuning,
reference/crystal estimates and nominal tick boundaries, and completes missing
power diagnostics after the existing control commit. It does not change output
source, calibration, or regulator state. Extra computation/logging can affect
cadence; the emitted ticks quantify CPU timing, not source sample time.

Use `trackq_diag 0` and `trackq_observer_diag 1` to collect at least ten complete
board-clock cycles, followed by a separate repeat recording. The offline tool
`analyze_trackq_observer.py` compares existing raw frequencies, excitation
frequencies alone, and excitation plus inferred detuning. Optional three-gain
calibration uses chronological whole-cycle train/validation/test splits and a
frozen model for the repeat. The reference is a training target/comparator,
never an input to the crystal candidate inference.

See [OBSERVER_IMPLEMENTATION.md](OBSERVER_IMPLEMENTATION.md) for log schema,
commands, equations, acceptance criteria and limitations. No firmware build or
hardware validation was performed as part of this implementation.

### Quiet detuning validation sweep (2026-10-04)

`trackq_sweep <channel 1..3> <span_Hz> <step_Hz> [settle_ms=1000]` runs a finite up/down mixer sweep with the current symmetric three-tone settings. It stops tracking, services the CPU-fed excitation during settling/capture, drains DS backlog, checks both DS and UPS flags, and buffers results until the entire sweep completes. It restores the selected mixer and leaves tracking stopped; the output NCO is unchanged. Example for A100 near its current resonance: `trackq_sweep 2 300 20 1000` (62 points, about 80 seconds or more; synchronous console). Save the QS1 header and QS rows together. See [QUIET_SWEEP.md](QUIET_SWEEP.md) for the format, bounds and interpretation; `python3 analyze_quiet_sweep.py sweep_A100.txt --out sweep_A100_analysis` makes reference-corrected plots and diagnostic slopes. Board validation and firmware build remain to be done by the operator.

`trackq_hold <ch> [samples=90] [wait_ms=2000]` adds a quiet fixed-mixer experiment using the same fresh-capture path. Example: `trackq_hold 2 90 2000` with external reference enabled and A100 already near resonance. It stops tracking, keeps the LO and NCO fixed, records three powers plus centered/first/last FFT64 reference measurements, and prints QH1/QH records only at the end (roughly 3.5 minutes). UPS fault flags persist across the run. See QUIET_SWEEP.md for details; `python3 -B analyze_quiet_hold.py hold_A100.txt` analyzes one run without generating files.

`trackq_check <ch 1..3>` performs two quiet captures (about 5 seconds) at the current mixer and symmetric tone settings, then prints PL/PC/PR, the strongest tone and `center_max=yes/no/unverified`. Channels: 1=C300, 2=A100, 3=C100. It stops tracking and leaves it stopped. It does not adjust frequency. Acquisition/excitation flags make the result unverified; reference-only failures do not prevent comparing measured powers. This is a short local-maximum check, not a resonance calibration or automatic lock. Firmware build/board verification remains pending.

### A100 starting point and interval diagnostics (2026-10-04)

At approximately 19 C room temperature, the latest successful `trackq_check 2` showed central power about 5.1–5.6e12 and side powers about 2.5–2.9e12. The preceding tracking excerpt placed raw A100 near **6 270 960 Hz** (last explicitly valid raw reading 6 270 961.644 Hz, with a later `(na)` fallback 6 270 959.062 Hz). Use about **6 269 960 Hz mixer + 1000 Hz central BB tone** as a restart approximation, not a measured exact resonance. The old check did not log the exact LO increment, and the attempted `trackq_start ... 6270030 ...` returned Unknown command, so it must not be treated as the actual setting. Preserve the current board setting if possible. New check headers print exact `lo_inc` and `drive_raw_mHz` for future reproducibility. Nominal firmware calibration constants are unchanged.

Hold/check now allow 2 seconds of serviced recovery after initial UART output. Old/recovery UPS flags are reported separately as `startup_ups`; they do not automatically invalidate later captures. Flags are then sampled/cleared at interval boundaries: `ups_wait`, `ups_capture`, `ups_process` (4=overflow, 8=underflow). Row `flags` conservatively combines those stages plus existing DS/reference diagnostics. `history_ups` retains the OR across measured intervals for the final report. Clear/status synchronization is allowed at each boundary; these are interval indications, not event counters or exact timestamps. A capture following a processing interruption still requires physical settling; use the default 2-second wait and review neighbouring records.

`trackq_check` now prints observed peak/central dominance independently of `measurement=ok/unverified`. Current check duration is about 7 seconds. Hold format is **QH2**: append `ups_wait,ups_capture,ups_process` to the QH1 record. `analyze_quiet_hold.py` reads both versions. A diagnostic footer reports startup/history. No log is emitted during recovery, waiting, acquisition or processing. Firmware has not been built or tested on the board in this change.

### Servicing excitation during processing (2026-10-04)

Repeated A100 checks at exact LO increment **6473382**, BB tones **900/1000/1100 Hz**, amplitude 10900 showed central dominance, with `ups_wait=0 ups_capture=0 ups_process=8`. Correlation loops now service up to four pending generator events every four samples (previously one every sixteen). The Hann-window loop also services the generator every sample, and the spectral-peak scan services it periodically. Before power processing and potentially long reference FFT work, a bounded helper queues up to 512 excitation frames, checking FIFO writable before DDS phase advances. This provides a nominal 51.2 ms reserve per fill before accounting for fill time; it does not guarantee that an arbitrary FFT size can never underflow. Queued frames remain phase-contiguous; the FIFO's existing capacity bounds queueing. Board measurements must verify the improvement.

Processing is now split into `ups_power` (three band powers and vertex arithmetic) and `ups_ref` (reference FFTs); `ups_process` is their OR. Check output shows both. Hold format **QH3** appends those two fields to QH2; the analyzer supports QH1/QH2/QH3 and verifies consistency. All output remains buffered. The fixed test has unchanged frequencies throughout, so prefilling does not queue obsolete settings within the test. Changes to software tone frequencies outside the test may take time to propagate through an already populated FIFO.

Next: after loading updated firmware, restore the A100 ±100 Hz tones and mixer 6473382, then `trackq_check 2`. If both samples have `center_max=yes measurement=ok`, proceed to `trackq_hold 2 90 2000` with reference enabled. If a subphase still reports 8, retain the short log before another long run. 24 host analysis tests pass; firmware was not built or board-tested, and no staging/commit was performed.

### DAC2/ADC1 loopback relative to external reference (2026-10-04)

`loopback_check <ch 1..3> [samples=90] [wait_ms=2000]` uses the quiet fixed capture path and adds Hann/log FFT2048 estimates of the received mode's central tone and reference from the same 2048-frame record. The central-tone search is bounded to +/- one third of the configured side spacing, with a post-fit range check. This command requires input_select=0 (physical ADC1), upsampler_input_mux=1 (software excitation), DAC2 selector=channel+4 (6 for A100), and tone spacing >=40 Hz. It stops all tracking and leaves the mixer/NCO fixed. The external 10 MHz reference remains on ADC2.

A100 setup after loading the updated firmware:
```
ref_fft_stop
trackq_diag 0
trackq_observer_diag 0
input_select 0
upsampler_input_mux 1
trackq_start 10002949 6269960 3387403 2048 1000 10 100 20
trackq_stop
phase_down_2 6473382
output_select_ch2 6
```
Disconnect crystal-board output from ADC1; connect DAC2 to ADC1 (electrical compatibility approved by operator). DAC1 may be disconnected from the analog crystal board: software excitation with mux=1 and stopped tracking does not depend on the crystal feedback. That disconnect changes crystal excitation, which is irrelevant to this loopback test, and may change analog loading; it does not change digital DAC2 frequency settings. Leave the external reference connected and enabled on ADC2. Do not run tracking after rewiring.

First run `loopback_check 2 4 2000`; inspect flag-free estimates before the long `loopback_check 2 90 2000` run. The console is synchronous and silent until completion; allow several minutes for the long run (large FFT processing adds time; use logged timestamps). Save LB1 header, LB records and diagnostic footer to `loopback_A100.txt`. Headers include exact LO/reference/tone increments. LB1 uses the 20 QH3 fields plus `dac_bb2048_mHz,ref_bb2048_mHz`; flags 512 and 1024 denote unavailable DAC/reference FFT2048 estimates. Existing transport/excitation/ref64 flags remain. Diagnostic footer retains QH labels for the shared capture path.

`python3 -B analyze_loopback.py loopback_A100.txt` prints the received RF frequency derived from the measured BB and the measured reference, versus a prediction using the programmed tone. It generates no artifacts. At nominal 65 MHz RF clock and 10 kHz decimated sample rate:

    measured_dac_Hz = ref_known_Hz * (lo_inc*65e6/2^26 + measured_dac_bb_Hz)
                                      / (ref_lo_inc*65e6/2^26 + measured_ref_bb_Hz)

This checks real DAC/ADC signal transmission and estimator consistency against the existing external-reference chain. Because DAC/ADC share a clock and the same reference estimator is reused, agreement alone cannot independently prove that all reference-estimator modulation originates in the board oscillator. Nonstationary windows need separate review using the beginning/center/end reference diagnostics. Three-tone zero crossing is not used.

For FFT2048 setup/execution, the bounded excitation reserve is enlarged to up to 8192 queued frames before each non-yielding block (within the existing 16384-frame FIFO), with normal servicing between blocks. No guarantee of uninterrupted excitation is assumed; UPS flags remain checked. Host-only tests (27) pass, including tests that the reconstructed DAC result responds to the measured BB rather than only the programmed increment. Firmware/FPGA build and board validation were not run; no staging or commit.

### Single-tone resonance sweep (2026-10-04)

`trackq_single_sweep <ch 1..3> <span_Hz> <step_Hz> [settle_ms]`
uses the quiet sweep capture path, with only the selected channel's central
software tone contributing to excitation. Other channels and side tones are
muted by a temporary generator override; their configuration is preserved.
This is one real baseband tone, hence RF LO +/- tone sidebands still exist.
It is not single-sideband synthesis. Tracking remains stopped afterwards.
The mixer and original tone selection are restored even after capture timeout.
Four seconds of serviced recovery before capture and after restoration allow
queued generator frames to leave the 16384-frame FIFO. No per-row UART output
occurs during acquisition. The reference LO and output NCO are not changed.

Connect DAC1 to the crystal board, its response to ADC1, and stable 10 MHz to
ADC2. Required routing: input_select 0, upsampler_input_mux 1,
output_select_ch1 15. Use sig3_start, sig3_enable_ch 2 and
sig3_freqs 2 900 1000 1100 to establish the A100 configuration.
Suggested first run: phase_down_2 6473629, then
trackq_single_sweep 2 300 20 2000. This spans approximately
6270898..6271498 nominal Hz in 62 points, including the return sweep.
Allow about three minutes; use logged t_ms, not a presumed sample interval.

ST1 header uses QS1 fields. ST rows contain:
index,dir,offset_Hz,LO_inc,t_ms,read_us,PL,PC,PR,ref64_mHz,det_mHz,
vertex_ok,flags,ref_first_mHz,ref_last_mHz,ups_wait,ups_capture,
ups_process,ups_power,ups_ref,ref2048_mHz.
PC is the driven central-band power; PL/PR are unexcited diagnostic bands.
Detuning and vertex are disabled (zero means not applicable).
Flags match QH3 plus 1024 for invalid FFT2048 reference. UPS flags cover
settling, capture and processing. Endpoint reference windows expose transitions.
ST is deliberately a separate format; existing QS/LB parsers do not accept it.

Offline frequency reconstruction, with b=ref2048_mHz/1000:
scale = 10000000 / (reflo*65000000/2^26 + b)
f_drive = (LO_inc*65000000/2^26 + central_tone_inc*10000/2^32)*scale.
Compare PC maxima on both nominal and reference-corrected axes. Endpoint
reference changes and flags must be inspected before fitting a static peak.
A changed peak relative to a three-tone run can reflect excitation dependence;
this test does not establish an unloaded, amplitude-independent resonance.

Validation: 14 existing Python QS/QH/LB analysis regression tests pass.
The new firmware path was source-reviewed; no firmware/FPGA build or hardware
validation performed. No git staging or commits.

### Single-tone DAC1 loopback sweep (2026-10-04)

`loopback_single_sweep <ch 1..3> <span_Hz> <step_Hz> [settle_ms]`
uses the same temporary central-tone override, acquisition, bidirectional LO
sweep, FIFO diagnostics and restoration as trackq_single_sweep. Connect DAC1
(sum output, output_select_ch1 15) directly to ADC1 instead of the crystal
response, with appropriate level/attenuation; disconnect the analog board
from that route. Keep stable 10 MHz reference on ADC2. DAC2 is unused.
This retains the actual output path used in the preceding crystal experiment.

SL1 header has the ST1 configuration fields. SL rows have QH3's first 20
numeric columns followed by dac_bb2048_mHz and ref_bb2048_mHz. Both FFTs use
the same fresh 2048-frame record. The received central tone is searched within
+/- one third of the configured side spacing, with a post-fit range check.
Require spacing >=40 Hz. Bits 512/1024 indicate unavailable signal/reference
FFT estimates. Detuning and vertex stay disabled; all powers remain logged.
Missing estimates must not be interpreted as zero frequency.

Suggested comparison: phase_down_2 6473629 and
loopback_single_sweep 2 300 20 2000 (62 rows, roughly four minutes).
For a short check use loopback_single_sweep 2 20 20 2000 (six rows).
Inspect flags/history_ups and a received baseband near 1000 Hz before the
long run. Do not interpret shared-clock baseband stability as independent
proof of absolute RF stability. Use the measured tone and reference for:
f_RF = (LO_inc*65000000/2^26 + dac_bb2048_mHz/1000)
       *10000000/(reflo*65000000/2^26 + ref_bb2048_mHz/1000).
Compare power shape with the crystal sweep; absolute powers may differ with
loading/attenuation. Existing QS/ST/LB formats remain unchanged. SL is new
and needs explicit handling rather than feeding it to existing parsers.

No firmware build or hardware validation was performed for this addition.

### Hybrid synthesis shadow experiment (2026-10-04)

`trackq_hybrid 1` enables a shadow calculation inside trackq. It does not write
NCO settings: the existing reference-derived output update remains active.
`trackq_hybrid 0` disables the experiment and restores TIMER0 ownership.
`trackq_stop` also disables it. Enable AFTER starting trackq. Restarting trackq
resets the filter. Quiet tests and ref_fft_start require hybrid disabled first.
Do not reprogram TIMER0 through other commands during the experiment.

The same capture supplies raw C300, raw-joint absolute temperature and centered
Hann/log FFT64 reference. C300 is reference-scaled into physical Hz, divided by
its linear temperature factor relative to 25 C, and low-pass filtered. The
filter uses backward Euler with coefficient dt/(30 s+dt), measured with TIMER0
at nominal CPU frequency. First valid sample initializes it; a valid sample
after >10 s without accepted modal data reinitializes it. This is not an
independent timebase, but does not count serviced interrupts as elapsed time.
The candidate target is 10 MHz times filtered C300 / nominal C300. Candidate
increment is rounded to nearest using full floating-point reference clock
recovery; the existing applied path's integer-Hz conversion is unchanged.
No unsigned 64-bit to double conversions are introduced.

Current cadence remains the existing trackq cadence (about 1 s plus workload),
not the proposed future independent 100–200 ms reference loop. A 2048-point
modal record alone spans about 205 ms. Computation is on every completed
trackq capture; logs occur on the first such step after 5 s elapsed. No replay
or repeated update with stale samples. If a modal estimate is absent, the slow
value may support a candidate for up to 10 s with a valid current reference;
after that candidate validity is false. FIFO flags reject this step. Status
is cleared after the experimental step, so old sticky faults need not veto
all future steps. These are status checks, not proof of resonance lock or
precise analog sample timestamps.

HY1 fields: t_ms since reset, fresh (new accepted modal/temperature sample),
valid (candidate usable), age_ms since last accepted modal sample (4294967295
means never initialized), flags (DSov=1 DSuf=2 UPSov=4 UPSuf=8), ok/reject
counts since preceding log, T (absolute C), C25 (unfiltered compensated C300
Hz), slow (filtered Hz), fs (reference-derived Hz), cand (shadow increment),
ref_round (reference-only candidate using the same precision/rounding),
applied (current NCO CSR value from existing reference path), shadow=1.
A displayed slow value can be stale; consult valid/age. Invalid current T/C25
are printed as (na). Initial raw-joint validity and broad arithmetic guards
are used, not a newly calibrated temperature-range or residual acceptance test.
The 5-iteration legacy temperature/crystal log is suppressed while hybrid is
active; verbose control and observer diagnostics remain separately selectable
and should be off for this experiment.

Suggested experiment: reconnect crystal response (not loopback), start tracking
near verified modal maxima, disable trackq_diag and trackq_observer_diag, then
trackq_hybrid 1. Collect 5–10 minutes, allowing several time constants for the
filter. Output source selectors are not changed by this command. This phase
assesses candidate behavior only; it does not validate physical output clock
accuracy, and does not resolve the earlier modal/reference inconsistency.

Validation: standalone numerical checks for ideal thermal/clock cancellation,
NCO correction sign/rounding and irregular-dt filtering; 14 pre-existing Python
QS/QH/LB regression tests; git diff --check. No firmware build or hardware test.

### Hybrid HY2 fresh-capture correction

The first HY1 board run rejected every sample with flags=1. That check used
DS status after processing, conflating backlog/unused incoming samples with
faults in the record being estimated. Existing trackq capture also consumed
only its settle count before collecting, rather than draining all old backlog.

When hybrid is enabled, preparation now saves `pre` status, performs 2 s of
serviced recovery if historical UPS flags exist, and drains DS to empty with
a bounded 65536-frame limit. Failure stops tracking explicitly. It then clears
status with CDC settling, captures settle+N frames, saves `cap`, and clears
status again before processing. `proc` is sampled before the HY logger.
Normal non-hybrid capture behavior is unchanged. Observer acquisition flags
are sampled before the processing-boundary clear.

HY2 acceptance flags = cap OR (proc AND 12). Thus historical DS overflow and
DS overflow from unconsumed samples during processing are reported but do not
veto the completed record. Capture DS/UPS faults and processing UPS faults
still reject it. Processing history is retained for next-step recovery; no
end-of-log blanket clear hides it. Historical UPS recovery does not prove
analog settling for every resonator; inspect results and configure accordingly.

HY2 adds modes (valid modal fit bits C300=1,A100=2,C100=4, all=7), reason
(bit 1=joint modal estimate unavailable, 2=reference unavailable/out of guard,
4=blocking FIFO flags, 8=uninitialized/stale slow state, 16=compensation math
rejected), pre, cap, proc. modes does not distinguish not-due from failed fit;
use existing detailed diagnostics if necessary. reason describes current input
conditions; a missing current modal estimate may coexist with valid=1 while
a previous slow value is within the 10 s age limit. HY1's flags meaning is
superseded by the above; old HY1 logs should not be reinterpreted as HY2.

Validation: FIFO acceptance truth-table cases, hybrid numerical model checks,
14 existing Python analysis tests and diff whitespace check. No firmware build,
no staging/commit; new capture behavior still needs board validation.

### Extended HY2 raw modal log for relative analysis

HY2 now appends raw_C300, raw_A100 and raw_C100 (Hz with three decimals) from
the same capture's nominal-scale modal-fit array. Validity follows modes bits;
missing fits print (na), not stale fallback values. Existing flags/fresh fields
still determine capture acceptance. No estimator, filter, NCO write, cadence or
routing change. Startup config includes raw_modes=1. The additional serial
payload should be checked for new UPS faults in the next board run.

Save one 5–10 minute session including the config as h_relative_log.txt.
Run `python3 analyze_hybrid_relative.py h_relative_log.txt --out hybrid_relative_analysis --t0 19.5`.
T0 is the approximate initial crystal temperature used for local sensitivities;
choose the appropriate value for the run. It is not fitted to the reference.
The new analyzer is separate from analyze_relative_clock.py, which preserves
the older legacy/HY2 partial comparison. It rejects restarted/duplicate times,
requires >=3 complete clean triples, uses logged TIMER0 time, and outputs
CSV, JSON, PDF/PNG and README. It never reconstructs missing A100/C100.
Reference enters only the comparison, not the modal fit. Historical pre flags
and processing DS-only overflow do not invalidate a clean acquired record.

Validation: 18 Python tests (4 new parser/model tests plus 14 existing analysis
regressions), diff whitespace check. Firmware was not built; no git staging or
commit. Hardware acquisition and analysis of the new run remain pending.

### ST2: distinguish driven spectral line from resonance

trackq_single_sweep now emits ST2 header (ST rows). It adds two measurements
of the response from the same modal capture: response_bb2048_mHz (Hann/log)
and trackq_binfit_bb_mHz (the exact trackq_bin_vertex_estimate_mhz function).
The existing reference FFT2048 column retains its position; the two new columns
follow it. Bits 512 and 2048 indicate invalid response FFT and tracking binfit,
respectively; zero numeric placeholders under those flags are not measurements.
The Hann response search is bounded to +/- spacing/3, with post-fit range check;
require configured spacing >=40 Hz. Neither estimator is a full-band search
for an unforced natural mode. ST2 tests the driven line near the programmed
baseband center. No transfer-phase estimate is claimed: excitation phase/source
sample alignment is not captured. NCO and tracking behavior are unchanged.

Suggested short test, crystal path connected: select central nominal frequency
near the previous single-tone maximum using phase_down_2 6473423, with sig3_freqs
2 900 1000 1100 and amplitude 10900. Run
trackq_single_sweep 2 100 100 4000.
This measures offsets -100,0,+100,+100,0,-100 Hz (six captures), about 40 s.
Only central software tone of A100 is active; real excitation still produces
LO+/-1000 RF sidebands. Restore mixer and tone selection on completion.
Repeat 2–3 runs to sample different reference phases; retain each header.

Use logged actual LO and tone increments, not requested offsets, to reconstruct
nominal drive RF. Reconstruct measured response RF by adding the same nominal
LO to each measured response baseband. Their difference cancels that LO and
should remain near the estimator's bias if it measures the driven line.
Do not require zero difference: raw-power bin interpolation can be biased.
Reference-correct both drive and response with the same per-record scale for
physical-Hz comparison. If RF response follows drive while PC changes across
the three points, that supports driven-line measurement rather than a direct
resonance-frequency reading. It does not alone establish a calibrated resonance
or transfer-phase estimator. If the three amplitudes do not bracket a maximum,
recenter or use the longer sweep; no resonance conclusion from equal powers.

No firmware build. 18 existing Python regression tests pass; these do not
simulate the new firmware estimator calls. Hardware ST2 validation pending.

## Quiet repeated single-tone experiment (2026-10-10)

Command: `trackq_single_cycle <ch 1..3> <span_Hz> <cycles 1..21> [settle_ms]`.
Default settling is 1000 ms; allowed range 100..5000 ms. Each cycle has six
records: offsets -span, 0, +span, +span, 0, -span around the current selected
channel LO. Actual integer LO increments are logged; offset_Hz is the requested
nominal offset, not an independently measured RF frequency.

Only the selected channel's central software tone is emitted during the test.
Other tones/channels are temporarily muted. The test stops tracking, requires
hybrid/reference-only tests disabled, and leaves tracking stopped afterwards.
It restores the original LO and configured tone selection, with four seconds
of serviced FIFO recovery before measurement and after restoration. The output
NCO is never written. No UART output occurs between measurement points.

Each point settles, drains old capture data, then acquires 2048 samples of the
selected response and reference from the same FIFO frames. It measures central
response power PC and reference Hann/log FFT64 at the beginning, center and end
of that record. No modal FFT or reference FFT2048 is calculated in this mode.
The existing row buffer is reused (maximum 126 records), without new buffers.

CY1 header records channel, base LO, reference LO, NCO, amplitude, software tone
frequency/increment, span, cycles, settling, sample count and nominal rates.
CY rows contain: index, cycle (zero based), direction (0 ascending triplet,
1 descending triplet), requested offset Hz, actual LO increment, capture start
ms, capture duration us, PC, reference center/first/last mHz, aggregate flags,
and UPS wait/capture/power/reference flags. Reference frequencies are baseband
values. Flags: 1 DS overflow, 2 DS underflow, 4 UPS overflow, 8 UPS underflow,
16 timeout, 32 drain failure, 64 center reference invalid, 128 first reference
invalid, 256 last reference invalid. Failed measurements must not be treated as
zero-valued measurements. Startup FIFO history is reported separately at end.
Timestamps use TIMER0 with the nominal CPU clock, not an independent timebase.

Fit the amplitude maximum offline for each three-point pass, using actual LO
increments and PC. Reject passes with capture/excitation faults, missing rows,
flat/non-concave response or a maximum outside the sampled interval. Also fit
on the reference-corrected frequency axis using each point's own centered
reference measurement. These are sequential measurements, so each triplet
assumes sufficiently slow resonance drift; reversed ordering exposes drift
and settling effects. Large first/last reference changes identify records near
clock transitions. A fitted maximum is a local amplitude estimate, not proof
of a clock-independent natural resonance. Compare repeated maxima on both axes.

Board connection: DAC1 excitation -> crystal analog board -> ADC1 response;
stable external 10 MHz reference -> ADC2. Keep the crystal connected (no direct
DAC/ADC loopback). After loading the changed firmware:

```text
trackq_hybrid 0
ref_fft_stop
trackq_stop
trackq_diag 0
trackq_observer_diag 0
input_select 0
upsampler_input_mux 1
output_select_ch1 15
phase_down_ref 10323408
sig3_start
sig3_enable_ch 2
sig3_amp 2 10900
sig3_freqs 2 900 1000 1100
phase_down_2 6473320
trackq_single_cycle 2 40 2 4000
```

This short check gives 12 rows. If power brackets the maximum and flags are
clean, run `trackq_single_cycle 2 40 10 4000` and save the entire CY1/CY log.
The long test takes at least four minutes plus acquisition/processing/recovery;
console output is intentionally delayed until completion. The center is based
on the previous A100 experiment and may need recentering after days or a
change in crystal temperature. Use the logged times for analysis.

### CY2 capture fingerprints (2026-10-10)

The same `trackq_single_cycle` command now emits a CY2 header and appends three
fields to each CY row: `modal_hash_hex`, `ref_hash_hex`, `samples_read`.
Hashes are 32-bit FNV-1a over the ordered raw int16 sample bits (low byte then
high byte for each sample), accumulated while reading the same FIFO frame used
for power/reference estimation. No additional acquisition or UART output is
introduced. The row buffer grows by 1536 bytes; no extra sample buffer is used.
A complete capture has `samples_read=2048`. Timeout/drain flags still invalidate
incomplete records; do not compare their hashes with complete captures.

Interpretation: equal PC with different modal hashes means the raw records
are different despite identical power. Equal modal hashes with different
reference hashes suggests modal repetition, including possible periodic,
quantized waveforms; it does not establish stale FIFO data. Repeated pairs of
both hashes warrant checking the entire capture path. Hash collisions remain
possible; different hashes prove different records, whereas equal hashes do
not prove either duplication or freshness. These hashes are diagnostics, not
hardware acquisition sequence numbers.

After loading the firmware, re-establish the physical crystal-response and
reference routing and sig3 configuration as above. Set `phase_down_2 6473290`
and run `trackq_single_cycle 2 40 2 4000`. Save the CY2 header and all CY rows.
Check that the central power still brackets the peak, all flags are zero,
and all sample counts are 2048. The center was selected at approximately 20 C
and is not a universal resonance setting.

### CY3 direct modal sample comparison

`trackq_single_cycle` now emits CY3, retaining the CY2 fields and appending:
`compare_row`, `compare_n`, `modal_diff_n`, `modal_max_abs_diff`.
Three static int16 arrays (12 KiB total) retain the most recent complete modal
record at each LO position. Baseline row indices reset at every command;
comparisons never cross sessions, channels or changes in the base LO/span.
Comparison occurs during acquisition before each saved sample is overwritten.
No additional capture, FFT, UART output or stack-sized sample buffer is used.
Extra row fields occupy 2 KiB in the existing 128-row buffer.

`compare_row=-1` means first visit/no complete baseline. Otherwise it identifies
the preceding complete capture at exactly the same LO position; `compare_n`
is the number of sample pairs checked, normally 2048. `modal_diff_n` counts
unequal raw int16 pairs, and `modal_max_abs_diff` is the largest absolute
sample difference in ADC counts (0..65535, computed in int32 arithmetic).
An interrupted capture invalidates that slot's baseline. Complete captures
with FIFO flags are retained for diagnostics, so check flags of BOTH the
current row and compare_row before interpreting their comparison.

With both records complete, compare_n=2048 and modal_diff_n=0 establishes
sample-for-sample identity without relying on hashes. It does not by itself
identify the cause: periodic quantized waveforms and repeated/stale data remain
possible. Nonzero modal_diff_n proves the modal records differ. Reference hash
comparison remains probabilistic; direct reference sample comparison is not
implemented in CY3.

At amplitude 5500 the latest A100 maximum drifted left of base 6473259. Keep
that base for this diagnostic so repeating LO positions can be compared; it
is not a peak-estimation run. After reloading the firmware, set:

```text
sig3_freqs 2 900 1000 1100
sig3_amp 2 5500
phase_down_2 6473259
trackq_single_cycle 2 40 2 4000
```

Capture the CY3 header and all 12 rows. In a complete run the comparison rows
should be -1,-1,-1,2,1,0,5,4,3,8,7,6. If the suspected repetition does not occur
in two cycles, extend the same diagnostic to five cycles; no recentering is
needed to test sample identity.

## Adaptive quiet amplitude tracking experiment: AT1

`trackq_single_follow <ch 1..3> <span_Hz 1..2000> <cycles 1..21> [settle_ms]`
reuses quiet single-tone acquisition, centered/endpoint reference FFT64 and
CY3 fingerprints. Six records per cycle form ascending then descending
triplets. Center stays fixed within each triplet, and is reconsidered after
all three captures. The descending triplet uses the UPDATED center, so it is
not a repeat of the ascending triplet's exact RF positions. The test is finite,
stops normal trackq, buffers all results and restores initial LO and normal
tone selection. Output NCO is never written.

Steering uses only central response powers at three RF LO positions:
- Strictly largest middle power: a concave three-point power parabola gives
  a local peak offset, bounded by geometry to less than half a span. Round
  its LO correction to the nearest integer accumulator increment.
- Strictly largest left/right power: move center half a span toward that side.
- Tied maxima, zero power or capture/excitation faults: hold center.
- Reject moves beyond +/-2000 nominal Hz from initial LO or RF Nyquist limits.

Reference faults do not block amplitude steering, but invalidate corresponding
reference comparisons offline. Steering tests bits 1..32 across all three rows;
reference transitions must also be considered offline. A local parabola assumes
response approximately stationary across the sequential triplet; status 1 is
not a calibrated resonance uncertainty or proof of lock. No amplitude
normalization/phase fit/noise threshold is introduced in this diagnostic.

AT1 header has the same configuration fields as CY3. AT rows have all CY3
columns and append `center_LO_inc`, the center used for that capture. The
requested offset is relative to this moving center. An AP row follows each
complete triplet in the buffered output:
`AP,last_index,center_LO_inc,next_center_LO_inc,status,peak_offset_mHz`.
Status 0=bad power/capture, 1=bracketed parabola, 2=left strongest,
3=right strongest, 4=tie, 5=travel bound. Peak offset is meaningful only for
status 1 and is relative to center on the nominal RF axis. Side moves are
reacquisition steps, not measured resonance offsets. Use actual integer LO
spacing for offline fits on nominal and per-point reference-corrected axes.
AT captures only compare modal samples with the stored same-position record
when its ACTUAL LO increment also matches; otherwise compare_row=-1. This
prevents claiming a repeated record comparison across different RF positions.
Additional row metadata occupies 1536 bytes; previous CY3 sample buffers reused.

Board connection remains DAC1 -> crystal board -> ADC1 and stable 10 MHz
reference -> ADC2. After freshly loading firmware, normal startup routing and
sig3 enable apply. Run:

```text
sig3_freqs 2 900 1000 1100
sig3_amp 2 5500
phase_down_2 6473279
trackq_single_follow 2 40 2 4000
```

Save AT1, all AT and AP rows plus final diagnostics. First check clean capture
flags, sample counts, AP directions and whether bracketed fits are obtained.
After validation, repeat `trackq_single_follow 2 40 10 4000` from the same
initial LO (the command restores it). Ten cycles give 60 records and 20 fits/
reacquisition decisions over roughly four minutes. Do not enter commands during
acquisition. Test may follow a changing amplitude maximum, which is still not
proof of natural resonant frequency or independent clock reconstruction.

## Source audit of identical modal records (2026-10-10)

The adaptive AT1 log directly confirmed identical modal int16 records in rows
11 and 12 (2048 comparisons, zero differences) at LO 6473201, with different
reference records. This establishes modal sample identity, not its cause.

Read path checked:
- uberclock.c ds_fifo_read_frame writes one pop, then reads all 12 status
  registers. A100 is frame.x[1], reference measurements use frame.y[5].
- uberclock_core.py writes all six I/Q pairs into ONE 192-bit AsyncFIFO, depth
  16384, on rx1's ce_down. One readable pop both advances FIFO and latches the
  entire old FIFO head in ds_data_sys. CSR fields are slices of this one latch.
- The generated build/alinx_ax7203 gateware and generated CSR header show the
  same packing, latch and separate CSR addresses (A100 X offset 0x10a8,
  reference Y offset 0x10cc). This checks local artifacts, not the identity of
  the bitstream currently loaded on the board.
- The CE interrupt increments counters and disables its event; it does not pop
  DS FIFO. Generator background servicing feeds UPS and reads magnitude only.
  The synchronous quiet command does not run the regular poll/capture loop.
- ds_fifo_clear clears sticky fault flags, NOT FIFO contents. Quiet acquisition
  explicitly drains FIFO to empty before capture; clear alone is not flushing.

No source-level mechanism was found that advances reference FIFO records while
reusing only the A100 FIFO component. Empty pops hold the WHOLE latched frame
and raise underflow; that is not a selective modal replay mechanism. This is
not a formal proof of all runtime timing or FPGA behavior.

Producer path: ADC1 signed conversion -> selected_input -> rx2 CORDIC -> X/Y
filters -> packed FIFO. Reference uses ADC2 -> otherwise matching rx_ref chain.
The software single-tone test stores rx2 X only, not both modal quadratures.
All downsampler controls depend on common clock/reset/enables, not sample data.
Only rx1 CE is connected to the shared FIFO; matching channels are expected to
have aligned strobes, but there is no per-channel valid assertion/check.
The halfband output register updates on the same CE edge at which FIFO writes,
so synchronous logic captures its prior registered sample. With matched chains
this is a common one-output-period delay, not a replay of a 2048-sample block.

Important alternative hypothesis: TX uses the SAME rx2 phase accumulator as RX
(26-bit RX phase, top 23 bits for TX). DAC/ADC and filters use the shared DSP
clock. The 32-bit software DDS increment is 429496729 at nominal 10 kHz, almost
exactly one tenth of a cycle per sample, with a finite sine table. The driven,
demodulated waveform can therefore be nearly ten-sample periodic in board-clock
coordinates. A different reference record is expected because the external
10 MHz reference is independent. ADC, 12-bit RX CORDIC output, 12-bit CIC output
and subsequent rounding/truncation provide quantization that can make matching
periodic records exactly identical. This is a plausible explanation, not yet
validated by checking periodicity of the raw captured arrays.

Conclusion: sample identity alone does not establish stale capture. Do not
change FIFO/pop timing speculatively or treat it as an explained clock-model
failure. A compact raw-periodicity check (lag 10 differences across the modal
record, plus sample range) would distinguish highly periodic/quantized records
from an assumption of arbitrary independent noisy records. Conclusive FIFO
freshness instrumentation would require a UC-side acquisition sequence number
packed in each frame, which entails gateware/CSR changes and is not implemented
by this source audit. No firmware or gateware behavior changed in the audit.

## Within-record lag-10 periodicity: CY4 / AT2

Both `trackq_single_cycle` and `trackq_single_follow` append five trailing
fields: `modal_min,modal_max,lag10_n,lag10_diff_n,lag10_max_abs_diff`.
Headers are CY4 and AT2 respectively. Existing columns remain in order;
AT's center_LO_inc remains before these five new fields, AP rows unchanged.
Min/max are signed raw modal sample values. For each i>=10 during capture,
compare x[i] against x[i-10] from the same unmodified record. A full 2048-sample
record gives 2038 pairs. Count unequal pairs and maximum absolute difference
in ADC counts, using int32 arithmetic to handle the full int16 difference.
No extra record buffer, acquisition or post-capture scan is required; extra
metadata occupies 2560 bytes in the shared row buffer.

Zero lag10_diff_n with 2038 pairs proves repetition every ten samples within
that record (possibly a smaller period dividing ten); min=max instead indicates
constant data. Small differences suggest nearly periodic, quantized response;
they do not prove capture freshness or explain cross-record identity by
themselves. At center_Hz=1000 and fs_nom=10000 this is the relevant expected
period. For other tone frequencies lag 10 is still logged but may not correspond
to their period. Inspect only complete captures with clean FIFO flags.

After firmware reload, for a fixed-position diagnostic retaining the most
recent converged A100 center:

```text
sig3_freqs 2 900 1000 1100
sig3_amp 2 5500
phase_down_2 6473247
trackq_single_cycle 2 40 2 4000
```

Save CY4 header and all 12 rows. Fixed positions allow both within-record
periodicity checks and direct comparisons between repeated LO settings.

## Reference-controlled physical RF sweep: RS1

Added `trackq_ref_sweep <ch> <physical_center_RF_Hz> <span_Hz> <step_Hz>
[settle_ms=4000]`. Unlike nominal sweeps, it updates LO from fresh centered
reference FFT64 Hann/log during settling, accounting for baseband DDS rate
scaling with DSP clock /6500. Final 250 ms and modal acquisition hold LO.
Response PC and three same-record reference windows verify inferred achieved
RF, with flags for control failures, endpoint jumps and >5 Hz target error.
This is a finite single-tone experiment, restoring initial LO and tone selection;
no output NCO control or changes to regular trackq. Added reference scratch
array: 128 bytes; additional shared row metadata: 2048 bytes with 32-bit alignment.
See REFERENCE_STABILIZED_SWEEP.md for setup, RS columns and evaluation limits.
The physical-axis model must be checked on board; inferred achievement is not
an independent output measurement. No firmware build performed by agent.

## Reference time-domain comparator in ref_fft_start

Standalone reference measurement now includes `ref_zc2048`, a time-domain
rising zero-crossing estimator on the SAME fresh 2048 Y_ref samples used by
FFT2048. Subtract sample mean to remove DC, linearly interpolate each rising
crossing, and least-squares fit sample positions against crossing ordinal.
The slope is samples per period. No spectral bins, FFT, extra acquisition or
sample buffer are used for this method. Mean and crossing loops service the
software excitation. Existing centered FFT64 and Hann/log FFT2048 remain.

New fields: ref_zc2048, diff_zc_fft2048, fs64, fs2048, fs_zc, zc_crossings,
zc_flags, zc_min, zc_max, period_min_msamples, period_max_msamples, reflo.
Periods are sample counts multiplied by 1000, NOT milliseconds. zc_flags:
1 invalid/missing record or fewer than 8 crossings, 2 peak-to-peak raw range
less than 8 counts, 4 max/min crossing period ratio exceeds 1.25, 8 invalid
regression slope. Flag 4 retains the frequency for inspection but suppresses
fs_zc; no periods are silently rejected. The 8-count and 25% guards are
provisional signal-quality checks, not calibrated uncertainty limits.

All fs values use the same 10 MHz/reference LO reconstruction model, nominal
65 MHz DSP clock and positive reference beat convention. They are displayed
only when fft_fs_hz=10000 and reconstructed clock is in 64..66 MHz. The helper
is shared with the RS reference-controlled sweep so their clock arithmetic is
consistent. No ZC estimate controls RS, trackq or output NCO.

This compares different estimators, not independent hardware timebases. They
share ADC/filter/FIFO and nominal sample-rate interpretation. Agreement narrows
the issue beyond FFT interpolation; it does not validate absolute reference
frequency, capture timing or the clock model. ZC's full-record weighting differs
from Hann FFT windows, so disagreements near modulation transitions are not
alone evidence of estimator failure. Period spread and reference trace identify
these records. Model-implied clock comparisons remain self-consistency checks.

After reloading, connect only stable external 10 MHz to ADC2 (crystal optional;
no oscillator tracking required). With startup defaults:

```text
phase_down_ref 10323408
ref_fft_start
```

If hybrid/ref_fft was previously enabled, first use `trackq_hybrid 0` and
`ref_fft_stop`. Record at least 150 seconds, then `ref_fft_stop`. Save header
and complete ref_fft lines, including skipped_slots, e.g. new_ref_zc_log.txt.
No additional room temperature sweep or RF amplitude changes are needed.
Check zc_flags, crossing counts/range, estimator differences away from resets,
and whether sawtooth appears in all frequency/clock reconstructions.

Validation: 14 existing quiet-test Python regressions pass; a separate numerical
simulation of 32 quantized sinusoid cases (700..1200 Hz, DC offset, four phases)
checks the regression method with worst frequency error about 0.000301 Hz.
This simulation validates idealized method arithmetic, not firmware execution,
noise tolerance, harmonics or board-clock modulation. Firmware not built by agent.

## RS2 modal X/Y response diagnostics

Reference-controlled sweep now measures modal Y from the same FIFO frames as
X and reference, reusing another stopped mode buffer. Existing PC=PX, appended
PY/PIQ, means, min/max, DC-removed full-record variances, X/Y time-domain
frequencies with quality flags, X/Y hashes and sample count. Upper-RF servo
behavior unchanged; no normal tracking, WLS or output NCO changes.
See IQ_RESPONSE_EXPERIMENT.md for source audit, RS2 field units, flag semantics,
setup and hypothesis interpretation. TX is real modulation with both RF
sidebands, not SSB; RX produces two phase projections. I/Q sum is a diagnostic
of projection dependence, not yet a calibrated resonance estimator.
