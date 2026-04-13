Capture to DDR3
===============

This workflow uses the UberDDR3 side-memory path to capture high-speed samples
originating in the ``uc`` domain and write them into DDR3 through the DMA path
in ``ubddr3.py``.

What the hardware does
----------------------

At capture start:

1. software programs the DMA base address and mode,
2. software programs the capture source and beat count,
3. software strobes the DMA request CSR,
4. a SYS->UC pulse starts the UC-side stream source,
5. captured beats cross into ``sys`` through an async FIFO,
6. ``zipdma_s2mm`` writes those beats to the DDR3 controller through the shared
   wishbone crossbar.

Key CSRs
--------

- ``cap_enable``: select external high-speed capture instead of the internal ramp.
- ``cap_beats``: number of DMA-width beats to emit.
- ``dma_addr0`` / ``dma_addr1``: 64-bit destination address split into two registers.
- ``dma_req``: start capture/write.
- ``dma_busy`` / ``dma_err``: observe progress and faults.

Typical workflow
----------------

1. Enable capture and configure beat count
2. Trigger DMA request
3. Stream data out using UDP tools

Example CSR sequence
--------------------

.. code-block:: text

   dma_addr0 0xA0000000
   dma_addr1 0x00000000
   dma_inc 1
   dma_size 0
   cap_enable 1
   cap_beats 4096
   dma_req 1

Ramp-mode validation
--------------------

Before trusting the real capture source, validate the memory path using the
internal ramp source:

.. code-block:: text

   cap_enable 0
   cap_beats 1024
   dma_req 1

Operational notes
-----------------

- ``cap_beats`` counts DMA-width beats, not raw ADC samples.
- The base address should live inside the UberDDR3 side-memory window.
- If ``dma_err`` asserts, inspect address alignment, memory window, and whether
  the DDR controller finished calibration.
