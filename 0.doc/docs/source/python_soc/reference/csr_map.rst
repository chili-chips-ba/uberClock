CSR Map
=======

This page summarizes the most important software-visible registers exposed by the
Python/LiteX integration. Exact names and placement still depend on the final
LiteX CSR generation, but these groups match the intended control surface.

UberClock main control bank
---------------------------

The UberClock CSR bank lives in ``uberclock_csrs.py`` and covers:

- NCO/reference phase increments:
  ``phase_inc_nco``, ``phase_inc_down_*``, ``phase_inc_cpu*``
- magnitude and gain controls:
  ``nco_mag``, ``mag_cpu*``, ``gain*``
- routing and debug selection:
  ``input_select``, ``output_select_ch1``, ``output_select_ch2``,
  ``lowspeed_dbg_select``, ``highspeed_dbg_select``
- CPU-fed sample injection:
  ``upsampler_input_x``, ``upsampler_input_y``, ``final_shift``
- low-speed capture:
  ``cap_arm``, ``cap_idx``, ``cap_done``, ``cap_data``
- FIFO-backed low-speed readback and upsampler feed paths.

Atomic SYS->UC configuration link
---------------------------------

``csr_snapshot_fifo.py`` exposes a small commit/status register set used to
transfer DSP configuration atomically into the UC domain:

- ``commit``: enqueue one packed configuration frame,
- ``overflow``: sticky frame-drop indicator,
- ``fifo_flags``: readable/writable status bits.

DMA control
-----------

- ``dma_req``: start a DMA write (strobe)
- ``dma_addr0/1``: base address
- ``dma_inc``: increment address enable
- ``dma_size``: transfer size encoding

DMA status
----------

- ``dma_busy``: DMA in progress
- ``dma_err``: error sticky flag

Capture control
---------------

- ``cap_enable``: select external capture stream instead of the internal ramp
- ``cap_beats``: number of DMA-width beats to capture
- ``calib_done``: DDR initialization/calibration complete

Interrupts and events
---------------------

The UberClock integration also exposes an event manager where ``ce_down`` is
promoted to a LiteX interrupt source.
