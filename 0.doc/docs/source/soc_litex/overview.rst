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

The ``2.soc-litex`` directory is structured around the hardware design,
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

3.build
~~~~~~~

The ``3.build`` directory contains the Makefile-driven workflow. Running
``make help`` exposes the main commands:

.. code-block:: text

   Usage:
     make open-target    file=<name>
     make open-example   file=<proj>
     make build-board    [FREQ=..] [OPTIONS='..']
     make load           [FREQ=..] [OPTIONS='..']
     make term                [PORT=..]
     make build-sw            [DEMO_FLAGS='..']
     make sim
     make view-sim
     make clean-sim
     make setup-ethernet      [ETH_IFACE=..] [STATIC_IP=..]
     make start-server
     make stop-server
     make litescope
     make clean
     make copy-migen

Important targets:

- ``make build-board`` builds the FPGA image, currently through Vivado. The
  default flow includes ``--with-etherbone`` and ``--with-uberclock`` and uses
  a 65 MHz clock unless overridden.
- ``make build-sw`` compiles the bare-metal software and depends on generated
  files from the hardware build.
- ``make load`` programs the FPGA.
- ``make term`` opens the UART console, using ``/dev/ttyUSB0`` by default.

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

At the moment these files must be copied manually into a LiteX installation, or
the local Makefile helper can be used.

CPU-Driven Data-Path Control
----------------------------

Using the CPU and UART console, the system can steer every stage of the signal
chain without rebuilding the FPGA bitstream.

Running ``help`` in the UART console prints:

.. code-block:: text

   Available commands:
     help                  - Show this command
     reboot                - Reboot CPU
     phase_nco <val>       - Set input CORDIC NCO phase increment (0-524287)
     phase_down <val>      - Set downconversion CORDIC phase increment (0-524287)
     output_select_ch1 <val> - Choose DAC1 output source
     output_select_ch2 <val> - Choose DAC2 output source
     input_select <val>    - Set main input select register (0=ADC, 1=NCO)
     gain1 <val>           - Set Gain1 register (Q format value)
     gain2 <val>           - Set Gain2 register (Q format value)

Command roles:

- ``input_select`` chooses between the external ADC path and an internally
  generated sine wave from the CORDIC NCO.
- ``phase_nco`` and ``phase_down`` adjust the CORDIC phase increments.
- ``output_select_ch1`` and ``output_select_ch2`` map internal nodes to either
  DAC channel at runtime.
- ``gain1`` and ``gain2`` set channel gain values.

The current implementation is effectively single-channel while the design is
being generalized into a modular N-channel form.

CPU-side Sample Processing
--------------------------

The CPU has also been used to read samples from the downsampler, process them,
and feed them back into the upsampler path. Three polling approaches were
evaluated:

- A simple blocking loop, which stalls the UART console.
- A periodic 10 kHz timer, which works but adds fixed latency.
- An event-driven interrupt, which triggers on new data and is preferred.

The original README also references oscilloscope captures that demonstrate this
interrupt-driven round trip.
