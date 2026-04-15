.. SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
.. SPDX-License-Identifier: CC-BY-SA-4.0

DSP RTL Blocks
==============

The DSP RTL used in this design resides under ``1.dsp/rtl``.
This section documents the reusable RTL modules that form the core signal
processing path.

At a high level, the reusable DSP RTL is composed of:

- ADC interface blocks
- CORDIC and polar conversion blocks
- decimation and interpolation filters
- DAC interface blocks

The active SoC integrates these modules in ``2.soc/1.hw/uberclock``, where
they are connected through CSR-controlled routing, buffering, and capture
logic.

.. toctree::
   :maxdepth: 2
   :caption: Contents

   adc
   dac
   filters
   cordic
   cordic16
   to_polar
   placement
