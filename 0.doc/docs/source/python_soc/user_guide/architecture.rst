Architecture
============

This package deliberately mirrors the hardware boundary lines in the design. The
result is easier to maintain than a monolithic LiteX target script because the
clocking, CSR, stream, wishbone, and DDR concerns can evolve independently.

Top-level build structure
-------------------------

The top-level entry point is ``uberclock_soc.soc:build_main()``. It parses the
LiteX arguments, creates ``BaseSoC``, and delegates to ``Builder`` for build or
load operations.

Inside ``BaseSoC`` the main assembly order is:

1. create the AX7203 platform,
2. instantiate the CRG,
3. initialize ``SoCCore``,
4. add standard LiteX peripherals,
5. optionally add LiteDRAM or UberDDR3,
6. optionally integrate the UberClock DSP block.

Clock domains
-------------

- ``sys``: CPU, CSR bus, Wishbone fabric, DMA
- ``uc``: UberClock DSP / sampling domain
- ``ub_4x`` / ``ub_4x_dqs`` / ``idelay``: DDR3 PHY clocks

The clocking module fixes the UberClock domain to an exact 65 MHz while keeping
the standard system domain at 100 MHz. That separation is important because the
DSP/control logic and the software/CSR infrastructure have very different timing
and data-movement requirements.

Control path
------------

Software writes configuration CSRs in the ``sys`` domain. Those values are not
driven directly into the ``uc`` domain. Instead, ``CsrConfigSnapshotFIFO``
packs the selected fields into one frame and transfers them atomically over an
async FIFO. This avoids partial updates where some DSP control fields change a
cycle before others.

The control path therefore looks like this:

1. CPU writes LiteX CSRs in ``UberClockCSRBank``.
2. CPU strobes the config commit CSR.
3. ``csr_snapshot_fifo.py`` enqueues one packed control frame.
4. The ``uc`` domain pops the frame and updates all shadow registers together.
5. ``uberclock_core.py`` wires those UC-side shadow registers into the Verilog
   ``uberclock`` instance.

Data path (capture)
-------------------

1. UC domain generates capture stream (ramp / HS / LS)
2. Async FIFO crosses UC -> SYS
3. DMA (S2MM) writes stream into DDR3
4. Host reads/streams data via UDP tooling

There are two UC-side data sources used by ``ubddr3.py``:

- ``RampSource`` for deterministic validation traffic,
- ``SamplePackerStream`` for packing one sample per UC cycle into DMA-width beats.

``UCStreamMux`` selects between them and drives the UC->SYS FIFO that feeds the
DMA writer.

Memory architecture
-------------------

The SoC supports two external-memory patterns:

- standard LiteDRAM as main RAM when integrated main RAM is disabled and
  UberDDR3 is not used,
- UberDDR3 as side memory mapped at ``0xA0000000``.

UberDDR3 is not just a raw controller wrapper. It combines:

- a CPU-visible Wishbone path,
- a classic-to-pipelined bridge,
- a 2-master crossbar shared by CPU and DMA,
- a UC capture stream source,
- a SYS-domain zipdma S2MM write engine.

Interrupt and observability path
--------------------------------

``uberclock_core.py`` also bridges important UC-domain events back to software:

- ``ce_down`` becomes a LiteX interrupt source through ``EventManager``,
- low-speed debug data is surfaced through CSRs,
- high-speed capture samples can be routed into UberDDR3.

Module dependency view
----------------------

The most important dependency edges are:

- ``soc.py`` depends on ``clocking.py``, ``ubddr3.py``, and ``uberclock_core.py``.
- ``ubddr3.py`` depends on ``wishbone.py``, ``streams.py``, and ``rtl_sources.py``.
- ``uberclock_core.py`` depends on ``uberclock_csrs.py``, ``csr_snapshot_fifo.py``,
  and ``rtl_sources.py``.
- ``rtl_filelist.py`` and ``rtl_sources.py`` define what RTL gets pulled into
  the LiteX build.
