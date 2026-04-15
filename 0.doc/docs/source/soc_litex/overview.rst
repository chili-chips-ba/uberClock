SoC Integration with LiteX
==========================

This project uses `LiteX <https://github.com/enjoy-digital/litex>`_ as the FPGA
integration framework and instantiates a
`VexRiscV <https://github.com/SpinalHDL/VexRiscv>`_ soft-core CPU to control
the ADC → DSP → DAC signal path.

The signal-processing pipeline is implemented in reusable Verilog modules,
while the CPU provides runtime configuration, control, and data interaction
through memory-mapped registers and a UART console.

Overview
--------

The system is divided into two main domains:

- **Hardware datapath**: ADC, DSP, and DAC blocks implemented in Verilog
- **Software control**: CPU-driven configuration and monitoring via LiteX CSRs

The CPU interacts with the datapath through memory-mapped registers, enabling
dynamic reconfiguration without requiring FPGA resynthesis.

Directory Structure
-------------------

The ``2.soc`` directory organizes the design into hardware, software, and
integration layers:

``1.hw``
~~~~~~~~

Contains the SoC-side hardware RTL used to integrate and connect the DSP
pipeline within the system.

- Top-level design (``uberclock.v``) and channel modules
- Memory, DMA, FIFO, and interconnect infrastructure
- Standalone test designs for subsystem validation

This directory represents the bridge between reusable DSP blocks
(``1.dsp/rtl``) and the LiteX-based SoC.

``2.sw``
~~~~~~~~

Contains the embedded software running on the VexRiscV CPU.

- Based on a modified LiteX demo application
- Integrates generated CSR definitions into ``main.c``
- Provides a UART-driven interface for configuring and controlling the datapath

``5.docker``
~~~~~~~~~~~~

Contains containerization support for the build environment.

- Intended to provide a reproducible FPGA toolchain setup
- Integration of Vivado is currently under development

``6.migen``
~~~~~~~~~~~

Contains the LiteX/Migen-based SoC integration code.

- ``platforms``: board definitions (e.g., Alinx AX7203 pin mappings)
- ``targets``: top-level SoC configurations combining CPU, memory, and custom RTL

``8.python``
~~~~~~~~~~~~

Contains Python-side SoC construction and control utilities.

- LiteX-based SoC generation code
- APIs for interacting with the hardware design

System Interaction
------------------

At runtime, the system operates as follows:

- The CPU configures DSP blocks via CSRs
- Input data flows from the ADC through the DSP pipeline
- Processed data is sent to the DAC
- The UART console provides user interaction and debugging access

Conceptually:

.. code-block:: text

   ADC → DSP → DAC
          ↑
        CSRs
          ↑
     VexRiscV CPU
          ↑
          UART
