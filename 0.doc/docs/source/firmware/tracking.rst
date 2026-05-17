<!--
SPDX-FileCopyrightText: 2026 Ahmed Imamovic
SPDX-FileCopyrightText: 2026 Tarik Hamedovic
SPDX-License-Identifier: CC-BY-SA-4.0
-->

Tracking Firmware
=================

The tracking code in ``2.soc/2.sw/uberclock.c`` closes a software control loop
around the FPGA DSP datapath.  The FPGA moves samples at full rate, decimates
them into the firmware-visible FIFO, and exposes phase-increment CSRs.  The
firmware captures low-rate samples, estimates spectral power with KISS FFT, and
updates the downconversion phase increments.

Two tracking flows are implemented:

``track3``
   Coarse acquisition.  It sweeps a downconversion frequency until the expected
   three-tone pattern appears in the downsampled FFT.

``trackq_start`` / ``trackq_stop``
   Fine background refinement.  It periodically measures left, center, and
   right tone power, estimates the peak position, and nudges the tracked
   downconversion frequency while the main firmware loop continues running.

Spectral Pattern
----------------

The tracker assumes the downsampled spectrum contains three useful tones:

- a left tone below the target center,
- a center tone at the target,
- a right tone above the target.

The firmware compares power in those three bins or small bin bands.  A good
coarse lock has a strong center bin and side bins inside the expected balance
range.  Fine tracking uses the side-power difference as an error signal.

Coarse Acquisition: ``track3``
------------------------------

``track3`` is intended to find a lock point from an approximate starting
frequency.  The command:

1. writes a candidate ``phase_down_<channel>`` value,
2. waits for the DSP path to settle,
3. captures downsampled FIFO samples,
4. runs a fixed-point KISS FFT,
5. measures left, center, and right tone power,
6. repeats until ``track3_triplet_match`` reports a valid pattern.

Console form:

.. code-block:: text

   track3 <channel> [start_hz] [step_hz] [max_steps] [N] [center_hz] [delta_hz]

Typical workflow:

.. code-block:: text

   fft_fs 10000
   track3 1 10002950 10 400 2048 1000 20

Fine Tracking: ``trackq``
-------------------------

``trackq`` is the background tracker used after the coarse point is close.  It
runs from ``uberclock_poll`` through ``trackq_step``.  Each interval it:

1. captures a short synchronized sample set,
2. computes power around each channel's left/center/right tones,
3. calculates an error from the side-bin imbalance,
4. clamps the correction to configured limits,
5. writes the updated ``phase_down_1`` ... ``phase_down_3`` values.

Console form:

.. code-block:: text

   trackq_start <f1> <f2> <f3> [N] [center] [delta1] [delta2] [delta3]
   trackq_probe
   trackq_stop

``trackq_probe`` captures and prints one tracking snapshot without relying only
on the periodic background update.

Supporting Firmware Blocks
--------------------------

``capture_ds_fft_channel``
   Captures one channel of downsampled samples into the FFT input buffer.

``capture_ds_track_multi``
   Captures synchronized samples for the multi-channel ``trackq`` path.

``track_power_at_hz`` and ``track_band_power_at_hz``
   Convert requested frequencies into FFT bins and compute power estimates.

``track3_triplet_match``
   Applies the coarse-lock three-bin acceptance rule.

``trackq_step``
   Implements the non-blocking background control step.

``phase_down_read`` / ``phase_down_write``
   Abstract channel-indexed access to the generated phase-increment CSRs.

Design Split
------------

The tracking logic is deliberately split across hardware and firmware:

- RTL performs deterministic high-rate downconversion, filtering, and FIFO
  movement.
- Firmware performs adaptive decisions, FFT analysis, command parsing, and CSR
  updates.

This keeps the data path timing-critical and predictable while allowing the
tracking algorithm to evolve without resynthesizing the FPGA for every tuning
change.
