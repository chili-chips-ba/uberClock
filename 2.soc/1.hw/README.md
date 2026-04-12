# 1.hw

This directory contains the active SoC-specific RTL used by the LiteX-based
UberClock design.

It is no longer a catch-all hardware folder. The repository now uses a clearer
split:

- `1.dsp/rtl/`
  Reusable DSP and math-oriented IP blocks such as ADC/DAC helper blocks,
  filters, CORDIC variants, and polar conversion.

- `2.soc/1.hw/`
  SoC-specific integration RTL such as the UberClock top-level wrappers,
  memory/controller logic, and other modules that are tightly coupled to the
  LiteX/UberDDR3 system architecture.

- `2.soc/1.hw/testing/`
  Legacy and experimental datapath tops that were used to test alternative
  signal-processing arrangements during development.

## Active subdirectories

### `uberclock/`

Current top-level UberClock integration RTL used by the active LiteX build.

Important files:

- `uberclock.v`
  Main SoC-facing UberClock top-level.
- `rx_channel.v`
  Receive-side channel path.
- `tx_channel.v`
  Transmit/upconversion-side channel path.

### `memory/`

Active DDR3/controller/data-movement RTL used for the UberDDR3 side-memory
integration.

See [`memory/README.md`](./memory/README.md) for the block-level breakdown.

### `testing/`

Historical or experimental hardware variants kept for reference.

These are not the primary source of truth for the current SoC build, but they
are useful for understanding how different datapaths were explored.

Examples include:

- ADC-to-DAC loopback tops
- older combined ADC/DSP/DAC experiments
- CORDIC test tops
- LED memory experiments
- other legacy datapath wrappers

## Build expectations

The active Python/LiteX flow under `2.soc/8.python/` and the older Migen
flow under `2.soc/6.migen/` both pull RTL from:

- `1.dsp/rtl/` for reusable DSP blocks
- `2.soc/1.hw/uberclock/` for the current top-level UberClock wrappers
- `2.soc/1.hw/memory/` for UberDDR3 and bus/memory integration

If you add new reusable DSP IP, it should usually go into `1.dsp/rtl/`.
If you add new LiteX/UberDDR3-specific integration RTL, it should usually stay
in `2.soc/1.hw/`.
