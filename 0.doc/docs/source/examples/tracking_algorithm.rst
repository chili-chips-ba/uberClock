Tracking Workflow
=================

The SoC firmware includes two related tracking flows for tuning the
``phase_down_1`` downconversion setting from live captured data:

- ``track3`` performs a sweep search until a target three-tone pattern is found.
- ``trackq_start`` / ``trackq_stop`` run a background quadratic tracker once a
  lock point is already close.

These commands live in ``2.soc/2.sw/uberclock.c`` and are intended to be used
from the UART console after the bitstream and firmware are running.

What the tracker is looking for
-------------------------------

The tracking code assumes the captured downsampled spectrum contains three
relevant tones:

- one below the expected center frequency,
- one at the center frequency,
- one above the center frequency.

The firmware computes an FFT over downsampled FIFO data, inspects the power in
three bins, and adjusts the downconversion phase until the center bin dominates
in the expected way.

``track3`` sweep
----------------

``track3`` is the coarse acquisition step. It:

1. sweeps ``phase_down_1`` in frequency-space,
2. captures FIFO samples,
3. runs a KISS FFT in firmware,
4. measures the left / center / right bins,
5. stops when the three-bin power pattern matches the expected lock condition.

Console form:

.. code-block:: text

   track3 <start_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]

Typical use:

.. code-block:: text

   fft_fs 10000
   track3 900 5 400 2048 1000 20

``trackq`` background refinement
--------------------------------

``trackq_start`` is the fine-tracking stage. It does not sweep across a broad
range. Instead, it periodically:

1. captures a new FFT frame,
2. reads the same left / center / right bins,
3. fits a quadratic peak across those three points,
4. computes a correction in Hertz,
5. rewrites ``phase_down_1`` with the refined value.

Console form:

.. code-block:: text

   trackq_start [N] [center_hz] [delta_hz]
   trackq_stop

The background step runs on a timed interval derived from ``ce_ticks`` and is
meant to keep the lock centered while the system runs.

Related firmware blocks
-----------------------

The tracking flow depends on several parts of the firmware:

- ``capture_ds_fft_ch1()`` captures FIFO samples for FFT input,
- ``kiss_fft`` provides the spectral estimate,
- ``fft_band_power_at()`` computes the band power around the expected bins,
- ``track3_triplet_match()`` decides whether the coarse lock condition is met,
- ``trackq_step()`` performs the quadratic interpolation update in the
  background.

How it fits into the system
---------------------------

- The SoC and gateware generate downsampled FIFO data.
- The firmware performs the search and refinement in software.
- The result is pushed back into the CSR-controlled downconversion setting.

That makes this a good example of the hybrid control model used throughout
uberClock: high-rate data movement in RTL, adaptive tuning in firmware.
