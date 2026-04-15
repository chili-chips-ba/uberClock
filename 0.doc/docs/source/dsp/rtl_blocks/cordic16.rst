.. SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
.. SPDX-License-Identifier: CC-BY-SA-4.0

CORDIC16
========

The ``cordic16/`` directory contains a reduced-width implementation of the
CORDIC algorithm. It follows the same architectural structure as the full
CORDIC datapath but operates on a narrower datapath, making it suitable for
resource-constrained or lower-precision applications.

Overview
--------

The CORDIC16 path is composed of the following modules:

- ``cordic16.v``
- ``cordic_pipeline_stage.v``
- ``cordic_pre_rotate_16.v``
- ``cordic_round.v``
- ``gain_and_saturate.v``

These modules collectively implement:

- a pipelined CORDIC rotation engine,
- phase preprocessing,
- iterative shift-add micro-rotations,
- output rounding,
- gain correction and saturation.

Compared to the full CORDIC implementation, this version reduces datapath width
and complexity while maintaining the same fundamental algorithm.

``cordic16.v``
--------------

This is the main reduced-width CORDIC core. It accepts signed Cartesian inputs
and rotates them according to a phase input, producing rotated outputs with
lower precision than the full-width implementation.

Responsibilities
^^^^^^^^^^^^^^^^

The module:

- pipelines the input vector through multiple rotation stages,
- applies initial pre-rotation,
- performs iterative CORDIC rotations,
- reduces precision via rounding,
- applies gain correction and saturation.

Interface summary
^^^^^^^^^^^^^^^^^

Key signals:

- ``i_clk`` -- pipeline clock,
- ``i_reset`` -- synchronous reset,
- ``i_ce`` -- clock enable,
- ``i_xval`` and ``i_yval`` -- signed input vector,
- ``i_phase`` -- phase command,
- ``o_xval`` and ``o_yval`` -- rotated output vector.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The structure mirrors the full CORDIC pipeline:

1. pre-rotation,
2. iterative rotation stages,
3. rounding,
4. gain correction.

The difference lies in the reduced bit widths, which lower resource usage and
pipeline complexity at the cost of reduced numerical precision.

``cordic_pipeline_stage.v``
---------------------------

This module implements a single CORDIC pipeline stage.

Responsibilities
^^^^^^^^^^^^^^^^

Each stage:

- performs a conditional rotation based on the sign of the phase,
- applies shift-based arithmetic operations,
- updates the residual phase,
- registers intermediate results.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The rotation is performed using shift-add operations:

- right shifts based on the stage index,
- addition/subtraction depending on the phase sign.

This avoids multipliers and keeps the implementation efficient.

``cordic_pre_rotate_16.v``
--------------------------

This module performs the initial coarse rotation before the iterative pipeline.

Responsibilities
^^^^^^^^^^^^^^^^

The block:

- sign-extends the input vector to the working width,
- determines the quadrant/octant from the phase,
- applies a coarse rotation (e.g., 90°, 180°),
- adjusts the phase to a smaller residual angle.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

By reducing the input angle range before the iterative stages, this block
improves convergence and simplifies downstream processing.

``cordic_round.v``
------------------

This module reduces the working-width results to the final output width.

Responsibilities
^^^^^^^^^^^^^^^^

The block:

- applies rounding before truncation,
- produces reduced-width outputs,
- registers the result.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

Rounding is performed by adding a bias before truncation, improving accuracy
compared to simple bit slicing.

``gain_and_saturate.v``
-----------------------

This module compensates for the inherent CORDIC gain and applies saturation.

Responsibilities
^^^^^^^^^^^^^^^^

The block:

- multiplies the outputs by a fixed gain correction constant,
- removes fractional bits,
- applies saturating scaling,
- registers the corrected outputs.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

As with the full CORDIC implementation, gain correction is required because the
CORDIC rotation introduces a constant amplitude scaling factor.

The result is then safely constrained within the valid output range.

Role in the datapath
--------------------

The CORDIC16 modules are used when:

- full precision is not required,
- FPGA resource usage must be minimized,
- a smaller datapath better matches surrounding logic.

They provide:

- sine/cosine generation,
- vector rotation,
- phase-to-I/Q conversion.

Structure summary
-----------------

Conceptually, the processing flow is identical to the full CORDIC:

.. code-block:: text

   input vector
        ↓
   pre-rotation
        ↓
   iterative CORDIC stages
        ↓
   rounding
        ↓
   gain correction and saturation
        ↓
   output vector
