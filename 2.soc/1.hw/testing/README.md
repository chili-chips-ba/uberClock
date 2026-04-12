# Testing RTL

This directory contains legacy and experimental RTL top-levels that were used to
test different datapath arrangements during development.

These designs are kept for reference and comparison, but they are not part of
the current primary SoC build flow.

Typical contents include:

- ADC to DAC loopback experiments
- ADC + DSP + DAC pipeline variants
- CORDIC-driven DAC test tops
- older combined datapath prototypes
- LED memory experiments and formal collateral

Current status:

- preserved for historical context and targeted experiments
- not the source of truth for the main LiteX/UberClock build
- may still be referenced by older legacy scripts

Active reusable DSP blocks now live under ``1.dsp/rtl``.
Active SoC-specific integration RTL now lives under ``2.soc/1.hw`` outside
this ``testing`` directory.
