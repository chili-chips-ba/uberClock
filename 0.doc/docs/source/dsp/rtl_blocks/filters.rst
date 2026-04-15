.. SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
.. SPDX-License-Identifier: CC-BY-SA-4.0

Filters
=======

The ``filters/`` directory contains the sample-rate conversion and filtering
blocks used in the DSP datapath. These modules implement efficient decimation
and interpolation pipelines using CIC filters, half-band filters, and
compensation stages.

Overview
--------

The filtering chain is composed of:

- CIC filters for coarse rate conversion,
- CIC compensation filters to correct passband distortion,
- half-band filters for efficient factor-of-2 rate changes,
- top-level wrappers assembling complete upsampling and downsampling chains.

Modules
-------

Core CIC filters
^^^^^^^^^^^^^^^^

- ``cic.v``
- ``cic_int.v``

These modules implement Cascaded Integrator-Comb (CIC) filters used for
efficient large-factor sample-rate conversion without multipliers.

Responsibilities:

- perform integration and comb filtering stages,
- support decimation and interpolation,
- provide high-efficiency rate change at low hardware cost.

Implementation notes:

CIC filters consist of:

- integrator stages operating at the input sample rate,
- comb stages operating at the output sample rate.

They introduce passband droop, which is corrected by compensation filters.

Top-level filter wrappers
^^^^^^^^^^^^^^^^^^^^^^^^^

- ``downsamplerFilter.v``
- ``upsamplerFilter.v``

These modules assemble full filter pipelines used in the SoC datapath.

Responsibilities:

- connect CIC stages, compensation filters, and half-band filters,
- manage sample-rate transitions,
- provide clean interfaces to the rest of the DSP chain.

Implementation notes:

These wrappers define the actual system-level filtering behavior by combining
multiple primitive filter blocks into a complete processing chain.

Half-band filters
^^^^^^^^^^^^^^^^^

- ``hb_down_mac.v``
- ``hb_up_mac.v``

These modules implement half-band FIR filters using multiply-accumulate (MAC)
structures.

Responsibilities:

- perform efficient factor-of-2 decimation and interpolation,
- reduce sample rate in stages while maintaining signal quality.

Implementation notes:

Half-band filters have:

- symmetric coefficients,
- approximately half of the taps equal to zero,

which reduces the number of required multiplications.

Optimized variants
^^^^^^^^^^^^^^^^^^

- ``hb_down_opt.v``
- ``cic_comp_down_opt.v``

These modules provide optimized implementations of selected filter stages.

Responsibilities:

- reduce resource usage,
- improve timing performance,
- specialize specific parts of the filter chain.

CIC compensation filters
^^^^^^^^^^^^^^^^^^^^^^^^

- ``cic_comp_down_mac.v``
- ``cic_comp_up_mac.v``

These modules correct the amplitude distortion introduced by CIC filters.

Responsibilities:

- restore passband flatness,
- improve frequency response after coarse rate conversion.

Implementation notes:

CIC filters introduce a sinc-shaped attenuation in the passband. These
compensation filters apply an FIR response that approximates the inverse of
that distortion.

Coefficient memories
^^^^^^^^^^^^^^^^^^^^

The following files provide filter coefficients used by FIR-based stages:

- ``coeffs.mem``
- ``coeffs_comp.mem``
- ``comp_down_coeffs.mem``
- ``hb_down_coeffs.mem``

Responsibilities:

- store precomputed FIR coefficients,
- provide data for MAC-based filter implementations.

Role in the datapath
--------------------

The filtering blocks implement the full sample-rate conversion pipeline:

- coarse rate change using CIC filters,
- passband correction using compensation filters,
- fine rate adjustment using half-band filters.

Structure summary
-----------------

Conceptually, a downsampling chain follows:

.. code-block:: text

   input signal
        ↓
   CIC decimation
        ↓
   CIC compensation
        ↓
   half-band filtering stages
        ↓
   output signal

Similarly, the upsampling chain follows the reverse structure:

.. code-block:: text

   input signal
        ↓
   half-band interpolation
        ↓
   CIC interpolation
        ↓
   output signal
