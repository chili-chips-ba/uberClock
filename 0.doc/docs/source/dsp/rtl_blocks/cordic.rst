.. SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
.. SPDX-License-Identifier: CC-BY-SA-4.0

CORDIC
======

The ``cordic/`` directory contains the full-precision CORDIC datapath used for
rotation-based signal generation and coordinate transformation. In the current
design, these modules implement a pipelined CORDIC engine together with the
supporting blocks needed for phase preprocessing, iterative rotation, output
rounding, and gain correction.

Overview
--------

The full CORDIC path is built from the following modules:

- ``cordic.v``
- ``cordic_logic.v``
- ``cordic_pipeline_stage.v``
- ``cordic_pre_rotate.v``
- ``cordic_round.v``
- ``cordic_top.v``
- ``gain_and_saturate.v``

Together, these files provide:

- a pipelined rotation engine,
- a phase-accumulator driven sine/cosine generator,
- stage-by-stage CORDIC micro-rotations,
- final output rounding,
- gain compensation and saturation.

``cordic.v``
------------

This is the main pipelined CORDIC core. It accepts signed Cartesian input
values ``i_xval`` and ``i_yval``, rotates them according to ``i_phase``, and
produces rotated outputs ``o_xval`` and ``o_yval``.

Responsibilities
^^^^^^^^^^^^^^^^

The module performs the following operations:

- pipelines an auxiliary valid-style signal through the CORDIC latency,
- applies an initial quadrant-based pre-rotation,
- passes the intermediate vector through ``NSTAGES`` iterative rotation stages,
- rounds the final working-width result down to the output width,
- applies gain correction and saturation before driving the outputs.

Interface summary
^^^^^^^^^^^^^^^^^

Key parameters:

- ``IW`` -- input width,
- ``OW`` -- output width,
- ``NSTAGES`` -- number of CORDIC pipeline stages,
- ``WW`` -- internal working width,
- ``PW`` -- phase width.

Key signals:

- ``i_clk`` -- pipeline clock,
- ``i_reset`` -- synchronous reset,
- ``i_ce`` -- clock enable,
- ``i_xval`` and ``i_yval`` -- signed input vector,
- ``i_phase`` -- phase command,
- ``i_aux`` -- auxiliary valid-style input,
- ``o_xval`` and ``o_yval`` -- signed rotated outputs,
- ``o_aux`` -- delayed auxiliary output aligned to the pipeline.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The datapath is composed of four conceptual stages:

1. **Pre-rotation**
   The input vector is mapped into a suitable angular region before entering the
   iterative stages.

2. **Iterative CORDIC rotation**
   The vector passes through ``NSTAGES`` instances of
   ``cordic_pipeline_stage``, each applying a shift-add micro-rotation.

3. **Rounding**
   The internal working-width results are reduced to the output width by
   ``cordic_round``.

4. **Gain compensation**
   The inherent CORDIC gain is corrected by ``gain_and_saturate``.

The module also defines a table of per-stage elementary angles in fixed-point
phase format.

``cordic_logic.v``
------------------

This module wraps the CORDIC core with a phase accumulator and output scaling,
forming a practical sine/cosine generation block for system use.

Responsibilities
^^^^^^^^^^^^^^^^

The block:

- accumulates phase by adding ``phase_inc`` on each cycle,
- feeds the accumulated phase into the CORDIC core,
- uses a fixed initial vector aligned with the positive x-axis,
- scales the signed CORDIC sine and cosine outputs,
- converts them into 14-bit offset-binary style outputs,
- exports the phase accumulator and auxiliary-valid signal.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The phase accumulator advances continuously:

.. code-block:: verilog

   phase_acc <= phase_acc + phase_inc;

The CORDIC core is driven with a fixed starting vector:

- ``I_XINIT = 12'sd1000``
- ``I_YINIT = 12'sd0``

After the CORDIC outputs become valid, the results are:

- shifted left by two bits,
- offset by ``8192``,
- registered into 14-bit outputs ``sin_out`` and ``cos_out``.

This makes the block suitable for driving downstream unsigned DAC-oriented
logic.

``cordic_pipeline_stage.v``
---------------------------

This module implements one iterative CORDIC stage. Each stage conditionally
rotates the vector by a fixed elementary angle using only shifts and adds.

Responsibilities
^^^^^^^^^^^^^^^^

Each pipeline stage:

- registers its outputs,
- examines the sign of ``phase_in``,
- rotates clockwise or counterclockwise,
- updates the residual phase,
- optionally passes the values through unchanged when no valid stage operation
  is required.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

For a negative residual phase, the stage rotates in one direction; for a
positive residual phase, it rotates in the other. The shifts are determined by
the stage index:

.. code-block:: verilog

   y_in >>> (STAGE+1)
   x_in >>> (STAGE+1)

If the stage index exceeds the working width, or if the stage angle is zero,
the values are passed through unchanged.

``cordic_pre_rotate.v``
-----------------------

This module performs the initial pre-rotation before the iterative CORDIC
pipeline begins.

Responsibilities
^^^^^^^^^^^^^^^^

The block:

- sign-extends the input vector from ``IW`` to ``WW``,
- inspects the top phase bits,
- selects one of several coarse vector rotations,
- adjusts the phase so the remaining angle lies within the region handled by
  the iterative stages.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The pre-rotation uses the upper three bits of the phase word to determine the
quadrant or octant class. Depending on that value, the input vector is either:

- passed through unchanged,
- rotated by 90 degrees,
- rotated by 180 degrees,
- rotated by 270 degrees.

At the same time, the phase is reduced by a corresponding fixed offset so that
the downstream CORDIC stages only need to resolve the remaining angle.

``cordic_round.v``
------------------

This module rounds the full working-width CORDIC results down to the requested
output width.

Responsibilities
^^^^^^^^^^^^^^^^

The block:

- receives signed working-width x and y values,
- applies rounding before truncation,
- registers the reduced-width outputs.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

Rather than simply slicing off the upper bits, the module computes
``pre_xval`` and ``pre_yval`` by adding a rounding term before truncation.
The final outputs are then taken from the most significant ``OW`` bits.

This improves output accuracy compared with plain truncation.

``gain_and_saturate.v``
-----------------------

This module corrects the CORDIC gain and then applies saturating scaling.

Responsibilities
^^^^^^^^^^^^^^^^

The block:

- multiplies each output sample by a fixed gain-correction constant,
- removes the fractional portion by shifting right by 32,
- applies a saturating left shift by one,
- registers the corrected outputs.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The fixed-point gain-correction constant is:

.. code-block:: verilog

   localparam [31:0] CORDIC_GAIN = 32'hdbd95b16;

The processing is split into multiple registered stages:

1. multiplication,
2. fractional removal,
3. saturating doubling,
4. output register.

This keeps the timing structured and aligns the corrected x and y outputs.

``cordic_top.v``
----------------

This module is a higher-level CORDIC NCO wrapper. It combines a phase
accumulator with the main ``cordic`` core to produce continuous sine and cosine
outputs.

Responsibilities
^^^^^^^^^^^^^^^^

The block:

- accumulates phase using a fixed ``PHASE_INC`` parameter,
- instantiates the reusable ``cordic`` core,
- drives it with a fixed initial vector,
- outputs signed cosine and sine samples.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The relationship between output frequency and phase increment is given by:

.. code-block:: text

   f_out = f_clk * (PHASE_INC / 2^PW)

This module is useful as a standalone numerically controlled oscillator when a
self-contained CORDIC-based tone source is needed.

Role in the datapath
--------------------

These modules collectively support several important DSP functions:

- sine/cosine generation from a phase accumulator,
- vector rotation,
- Cartesian-domain frequency conversion support,
- conversion of phase commands into sampled quadrature waveforms.

Structure summary
-----------------

Conceptually, the flow is:

.. code-block:: text

   phase accumulator
        ↓
   pre-rotation
        ↓
   iterative CORDIC stages
        ↓
   rounding
        ↓
   gain correction and saturation
        ↓
   final sine/cosine outputs
