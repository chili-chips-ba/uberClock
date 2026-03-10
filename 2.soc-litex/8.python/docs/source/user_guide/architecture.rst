Architecture
============

Clock domains
-------------

- ``sys``: CPU, CSR bus, Wishbone fabric, DMA
- ``uc``: UberClock DSP / sampling domain
- ``ub_4x`` / ``ub_4x_dqs`` / ``idelay``: DDR3 PHY clocks

Data path (capture)
-------------------

1. UC domain generates capture stream (ramp / HS / LS)
2. Async FIFO crosses UC -> SYS
3. DMA (S2MM) writes stream into DDR3
4. Host reads/streams data via UDP tooling

