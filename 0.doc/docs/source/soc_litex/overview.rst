SoC-Based Control with LiteX and VexRiscV
=========================================

This project uses `LiteX <https://github.com/enjoy-digital/litex>`_ as the FPGA
framework and instantiates a
`VexRiscV <https://github.com/SpinalHDL/VexRiscv>`_ soft-core to control the ADC
to DSP to DAC flow. The bulk of the signal-processing pipeline lives in
parameterized Verilog modules, while the CPU handles configuration and data
movement at run time through a UART console.

Workflow Overview
-----------------

The ``2.soc`` directory is structured around the hardware design,
software control path, build flow, and the LiteX integration files.

1.hw
~~~~

The ``1.hw`` directory contains Verilog blocks organized into subdirectories.
ADC, CORDIC, CIC filter, and DAC modules are developed and tested there before
being integrated into the larger design.

2.sw
~~~~

The ``2.sw`` directory contains a modified version of the LiteX demo
application. It integrates the generated CSR definitions into ``main.c`` and
provides the bare-metal control interface for the datapath.

5.docker
~~~~~~~~

The ``5.docker`` directory is still under development. Integrating Vivado into
the container is the main unresolved part.

6.migen
~~~~~~~

The ``6.migen`` directory contains:

- ``platforms`` for board pin definitions for the Alinx AX7203.
- ``targets`` for the top-level LiteX target that instantiates the SoC and
  project-specific Verilog blocks.

8.python
~~~~~~~~

The ``8.python`` directory contains the entire SoC code and litex-api.


