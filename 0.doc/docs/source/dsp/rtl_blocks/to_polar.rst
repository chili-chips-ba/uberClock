.. SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
.. SPDX-License-Identifier: CC-BY-SA-4.0

Polar Conversion
================

The ``to_polar/`` directory contains logic for converting Cartesian (I/Q)
signals into polar form. This typically involves computing the magnitude and
phase of a complex input signal.

``to_polar/to_polar.v``
-----------------------

This module converts signed Cartesian input samples into a polar
representation consisting of magnitude and phase.

Overview
--------

The module accepts I/Q-style inputs:

- ``I`` (in-phase component),
- ``Q`` (quadrature component),

and produces:

- magnitude (signal amplitude),
- phase (signal angle).

Responsibilities
^^^^^^^^^^^^^^^^

The block performs:

- conversion from Cartesian coordinates to polar coordinates,
- magnitude estimation,
- phase computation,
- alignment of outputs with the input data stream.

Interface summary
^^^^^^^^^^^^^^^^^

Typical signals:

- ``i_clk`` -- system clock,
- ``i_reset`` -- synchronous reset,
- ``i_ce`` -- clock enable,
- ``i_x`` -- in-phase (I) input component,
- ``i_y`` -- quadrature (Q) input component,
- ``o_mag`` -- output magnitude,
- ``o_phase`` -- output phase.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The conversion from Cartesian to polar form follows:

.. code-block:: text

   magnitude = sqrt(I^2 + Q^2)
   phase     = atan2(Q, I)

In hardware, this is typically implemented using a CORDIC-based vectoring
algorithm rather than direct multiplication and square-root operations.

Depending on the implementation, the module may:

- reuse an existing CORDIC pipeline in vectoring mode,
- operate as a pipelined block with fixed latency,
- propagate a valid/auxiliary signal alongside the data.

Latency and alignment
^^^^^^^^^^^^^^^^^^^^^

As with other DSP blocks, the conversion is typically pipelined. Any auxiliary
or valid signals should be delayed to match the latency of the magnitude and
phase outputs.

Role in the datapath
--------------------

The polar conversion block is typically used:

- after downconversion (I/Q generation),
- before magnitude/phase analysis,
- in feedback or tracking loops.

It provides a bridge between:

- Cartesian-domain DSP processing, and
- magnitude/phase-domain processing.

Structure summary
-----------------

Conceptually, the processing flow is:

.. code-block:: text

   I/Q input
      ↓
   vectoring (CORDIC or equivalent)
      ↓
   magnitude + phase
      ↓
   downstream processing
