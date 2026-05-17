# uberClock RTL Simulation

This directory contains the imported `tb_uberclock` simulation bundle used to
exercise the standalone uberClock RTL datapath.

The simulation snapshot is kept here because it uses a self-contained RTL
source set with shared CORDIC helper module names. The canonical project RTL
remains under `1.dsp/rtl` and `2.soc/1.hw/uberclock`; update those locations
for production changes, then refresh this simulation snapshot when needed.

## Contents

- `tb_uberclock.v` drives the standalone `uberclock` top with a differential
  200 MHz reference clock and records NCO, receive-channel, transmit-channel,
  and summed-output text traces.
- `src/` contains the simulation RTL snapshot imported with the testbench.
- `*.mem` are filter coefficient memories loaded by the snapshot with bare
  `$readmemb()` filenames, so run the simulation from this directory.
- `results/` contains the captured text traces, generated plots, and plotting
  scripts from the imported run.
- `filelist.f` lists the RTL snapshot and testbench in compile order.

## Example

From this directory:

```sh
iverilog -g2012 -o tb_uberclock.vvp -f filelist.f
vvp tb_uberclock.vvp
```

The testbench writes new `*.txt` traces in the current working directory.
Move or rename the existing `results/*.txt` files before rerunning if you want
to keep the imported reference outputs separate from a fresh run.
