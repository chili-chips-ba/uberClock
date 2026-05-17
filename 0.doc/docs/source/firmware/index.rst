<!--
SPDX-FileCopyrightText: 2026 Ahmed Imamović
SPDX-FileCopyrightText: 2026 Tarik Hamedović
SPDX-License-Identifier: CC-BY-SA-4.0
-->

Firmware
========

The firmware under ``2.soc/2.sw`` runs on the VexRiscV CPU and provides runtime
control over the ADC, DSP, DAC, FIFO, capture, and DDR datapaths.

It allows:

- configure DSP blocks through LiteX CSRs,
- interact with the system through a UART console,
- push and pop low-rate I/Q samples through firmware-visible FIFOs,
- capture low-speed debug samples and high-speed DDR streams,
- run fixed-point KISS FFT analysis,
- execute ``track3`` and ``trackq`` frequency-tracking algorithms,
- generate software three-tone test signals for bring-up.

The firmware turns the hardware into an interactive system instead of a fixed
bitstream.

.. toctree::
   :maxdepth: 1

   code_layout
   tracking
   c_api
