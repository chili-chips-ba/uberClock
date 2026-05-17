<!--
SPDX-FileCopyrightText: 2026 Tarik Hamedovic
SPDX-License-Identifier: CC-BY-SA-4.0
-->

uberClock RTL Simulation
========================

The standalone uberClock RTL simulation bundle is stored in
``2.soc/4.sim/uberclock``. It archives the imported ``tb_uberclock`` testbench,
the coefficient memories used by the filters, a simulation-local RTL snapshot,
the captured text traces, and plotting scripts.

The simulation-local ``src`` directory is intentionally treated as a snapshot.
The canonical production RTL remains in ``1.dsp/rtl`` and
``2.soc/1.hw/uberclock``. Use the canonical tree for source changes, then
refresh the simulation snapshot when the standalone testbench needs to be
reproduced with those changes.

Simulation Layout
-----------------

``tb_uberclock.v``
   Drives the standalone ``uberclock`` top, generates a 200 MHz differential
   reference clock, applies reset, programs five downconversion phase
   increments, and captures NCO, receive, transmit, and summed output traces.

``filelist.f``
   Lists the simulation RTL snapshot and testbench in compile order.

``*.mem``
   Filter coefficient memories. The RTL loads these with bare filenames through
   ``$readmemb()``, so run the simulator from ``2.soc/4.sim/uberclock``.

``results/``
   Imported reference traces, plots, and plotting scripts.

Running the Testbench
---------------------

From ``2.soc/4.sim/uberclock``:

.. code-block:: console

   iverilog -g2012 -o tb_uberclock.vvp -f filelist.f
   vvp tb_uberclock.vvp

The testbench writes fresh ``*.txt`` traces in the current working directory.
Keep the imported reference traces under ``results/`` separate from newly
generated output when comparing runs.

Reference Plots
---------------

The imported result bundle includes plots generated from the captured receive
and transmit traces.

.. image:: ../../../../2.soc/4.sim/uberclock/results/Figure_1.png
   :alt: Imported uberClock simulation plot figure 1
   :width: 100%

.. image:: ../../../../2.soc/4.sim/uberclock/results/Figure_2.png
   :alt: Imported uberClock simulation plot figure 2
   :width: 100%
