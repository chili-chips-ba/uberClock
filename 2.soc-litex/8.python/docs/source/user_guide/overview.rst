Overview
========

UberClock is a LiteX-based SoC integrating a multi-channel DSP block, a DDR3 capture path,
and a host-side UDP streaming workflow.

High-level components:

- **DSP core (UC domain)**: processes incoming samples, produces high-speed capture data.
- **CDC / FIFO**: safely crosses UC -> SYS domain.
- **DDR3 writer (SYS domain)**: stream-to-memory (S2MM) DMA into DDR3.
- **Wishbone fabric**: CPU access + DMA share the DDR3 controller.

