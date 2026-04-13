Overview
========

``uberclock_soc`` is the Python integration layer for the UberClock AX7203 SoC.
It wraps the board/platform setup, clock generation, DDR3 capture block, and
UberClock DSP integration into importable modules instead of one large target
script.

Why this package exists
-----------------------

- Keep the AX7203 target entry point small and stable.
- Make custom LiteX integration blocks reusable and testable in isolation.
- Separate hardware-facing concerns such as clocks, wishbone bridges, CSRs, and
  DDR capture plumbing.
- Provide Sphinx API documentation for the Python layer, not just the RTL.

Package map
-----------

The main modules are split by responsibility:

- ``soc.py``: top-level SoC assembly and CLI build entry point.
- ``clocking.py``: CRG and exact clock-domain generation.
- ``uberclock_core.py``: Verilog UberClock integration, CSR wiring, IRQ wiring,
  and UC-domain control snapshotting.
- ``uberclock_csrs.py``: software-visible CSR bank for the DSP/control path.
- ``ubddr3.py``: DDR3 controller wrapper plus DMA capture path.
- ``streams.py``: UC-domain stream generators and packers for capture.
- ``wishbone.py``: classic-to-pipelined bridge and 2-master/1-slave crossbar.
- ``rtl_sources.py`` and ``rtl_filelist.py``: RTL discovery and manifest helpers.
- ``csr_snapshot_fifo.py``: atomic SYS->UC CSR transfer mechanism.

High-level system composition
-----------------------------

At a high level, the SoC is made of five cooperating layers:

1. **Platform and clocks**
   AX7203 platform setup, input clock buffering, MMCM setup, and domain naming.
2. **LiteX base system**
   CPU, CSR bus, timer, optional Ethernet/Etherbone, optional HDMI, and either
   integrated RAM or external memory.
3. **UberClock DSP block**
   Multi-channel DSP datapath in the ``uc`` clock domain, plus configuration,
   debug selection, and interrupt/capture plumbing.
4. **UberDDR3 capture path**
   Custom DDR3 controller and DMA writer that can capture UC-domain samples into
   side memory mapped into the SoC.
5. **Host and firmware tooling**
   Build scripts, demo software, and UDP-based data extraction scripts under
   ``3.build`` and ``2.soc/2.sw``.


Typical use cases
-----------------

- Build a bitstream with ``--with-uberclock`` to exercise the DSP/control path.
- Build with ``--with-uberddr3`` to enable high-speed capture into DDR3.
- Use the CSR bank to tune NCOs, gains, routing, and debug paths.
- Trigger DMA capture, then inspect or stream the captured memory externally.

Where the docs fit
------------------

- The **User Guide** explains structure and intent.
- The **How-to** pages cover repeatable workflows.
- The **Reference** pages summarize software-visible contracts.
- The **API** section exposes the Python classes and functions directly from the
  source code.
