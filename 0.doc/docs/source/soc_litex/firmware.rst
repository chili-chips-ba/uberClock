Firmware and C Software
=======================

The SoC firmware lives under ``2.soc/2.sw``. This is the C-side control
layer that runs on the VexRiscV soft core after the LiteX build has generated
the hardware support files.

Role
----

The firmware is responsible for:

- consuming the generated CSR definitions from the LiteX build,
- exposing the runtime control interface for the datapath,
- driving experiments without rebuilding the FPGA bitstream,
- supporting demo and bring-up workflows from the UART console.

Code layout
-----------

The important firmware-side files under ``2.soc/2.sw`` are:

- ``main.c`` for startup and top-level application flow,
- ``uberclock.c`` for most console commands and SoC-specific control logic,
- ``ubddr3.c`` for side-memory / DDR helper routines,
- ``console.c`` for the UART command shell plumbing,
- ``kiss_fft.c`` / ``kiss_fft.h`` for in-firmware FFT analysis.

In practice, ``uberclock.c`` is the operational center of the firmware. It owns
the command handlers, CSR writes, FIFO servicing, spectral analysis, and
tracking routines.

Relationship to the Python layer
--------------------------------

The Python package under ``2.soc/8.python`` assembles the hardware and
generates the LiteX build outputs. The C firmware then uses those generated
outputs to talk to the SoC at runtime. They are two parts of the same SoC flow:

- Python defines and builds the system,
- C configures and exercises the running system.

Typical workflow
----------------

1. Build the SoC and generate the board-specific LiteX output tree.
2. Build the software against ``3.build/build/<board>``.
3. Load the bitstream.
4. Open the UART terminal and use the firmware commands to configure the DSP
   path and captures.

Console-driven control model
----------------------------

The firmware exposes the SoC through a command-oriented UART shell. The command
set covers:

- NCO and phase control,
- gain and routing updates,
- FIFO status and sample movement,
- low-speed and DDR-backed capture control,
- FFT-based inspection,
- tracking routines such as ``track3`` and ``trackq_start``.

This is the main reason the firmware matters architecturally: it turns the
hardware into an interactive instrument instead of a fixed bitstream.

Tracking and analysis
---------------------

The firmware does more than register writes. It also performs algorithmic work
in software when that is easier or more flexible than implementing it in RTL.

The current examples include:

- ``fft_ds`` style FFT inspection over downsampled FIFO data,
- coarse search with ``track3`` for a target three-bin spectral pattern,
- background quadratic correction with ``trackq_start`` / ``trackq_stop``.

That makes the firmware the adaptive layer above the RTL datapath: hardware
moves and produces data, firmware interprets it and retunes the system.

See also
--------

- :doc:`../python_soc/reference/csr_map`
- :doc:`../python_soc/reference/memory_map`
