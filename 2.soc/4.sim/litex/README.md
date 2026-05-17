# uberClock LiteX Simulation

This directory contains simulation support files for the LiteX-level SoC
simulation target in `uberclock_soc.sim`.

The LiteX simulation target uses the canonical SoC RTL from:

- `2.soc/1.hw/uberclock`
- `1.dsp/rtl`

It does not use the standalone RTL snapshot in `2.soc/4.sim/uberclock/src`.

## Build without running

From the repository root:

```sh
PYTHONPATH=2.soc/8.python/src:$PYTHONPATH \
CFLAGS=-Wno-error=incompatible-pointer-types \
/home/hamed/FPGA/Tools/litex-hub/litex/litex-venv/bin/python3 -m uberclock_soc.sim \
  --with-uberclock \
  --build \
  --no-run \
  --output-dir=3.build/build/sim
```

## Build and run

```sh
PYTHONPATH=2.soc/8.python/src:$PYTHONPATH \
CFLAGS=-Wno-error=incompatible-pointer-types \
/home/hamed/FPGA/Tools/litex-hub/litex/litex-venv/bin/python3 -m uberclock_soc.sim \
  --with-uberclock \
  --build \
  --non-interactive \
  --trace \
  --finish-after-cycles=100000 \
  --output-dir=3.build/build/sim
```

Use `--ram-init=<firmware.bin>` to preload firmware into integrated main RAM.
Use `--trace` and `--finish-after-cycles=<cycles>` for non-interactive waveform
captures; a run that exits before any dump calls are emitted will produce a
zero-length VCD.
