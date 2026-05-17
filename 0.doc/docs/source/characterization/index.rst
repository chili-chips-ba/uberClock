<!--
SPDX-FileCopyrightText: 2026 Minela Sultanovic
SPDX-FileCopyrightText: 2026 Nedim Osmic
SPDX-License-Identifier: CC-BY-SA-4.0
-->

Characterization
================

The ``5.characterization`` directory contains the measurement notes, lab
photographs, and modeling tools used to characterize the quartz resonator across
temperature, orientation, and vibration mode.

The characterization work is the empirical input to the DSP and tracking
algorithms: it identifies the useful resonant modes, estimates their movement
over temperature, and provides a multi-mode equivalent circuit model for
simulation.

Measurement Dataset
-------------------

Measurements were taken with a Vector Network Analyzer and stored as Touchstone
``.s2p`` files.  The analysis focuses on ``S21`` insertion loss: frequency is
read from column 1 and ``S21`` magnitude from column 4.

The raw data is organized as:

.. code-block:: text

   5.characterization/
   ├── 0.doc/        lab images and orientation diagrams
   ├── 1.raw_data/   raw VNA .s2p measurements
   ├── 2.analysis/   generated plots and summaries
   └── 3.model/      BVD extraction and crystal model scripts

Temperature and Modes
---------------------

The measurements cover 20 C to 65 C in 5 C steps.  The expected vibration
modes are:

.. list-table::
   :header-rows: 1

   * - Mode
     - Approximate frequency
   * - C100
     - 3.333 MHz
   * - B100
     - 3.660 MHz
   * - A100
     - 6.224 MHz
   * - C300
     - 10.000 MHz
   * - B300
     - 10.981 MHz

Orientation
-----------

The board was measured in six orientations to expose gravity and packaging
effects.  The orientation name identifies which side of the enclosure points
upward.

.. image:: ../../../../5.characterization/0.doc/OrientationDiagram.png
   :width: 420
   :align: center

Lab Setup
---------

.. image:: ../../../../5.characterization/0.doc/VNA--1.jpg
   :width: 620
   :align: center

.. image:: ../../../../5.characterization/0.doc/TempChamber--1.jpg
   :width: 620
   :align: center

.. image:: ../../../../5.characterization/0.doc/XTAL-Enclosure-1.jpg
   :width: 420
   :align: center

Multi-Mode BVD Model
--------------------

The ``5.characterization/3.model`` scripts extract a Butterworth-Van Dyke
equivalent circuit from the measured data.  The model keeps a static shunt
capacitance in parallel with one motional RLC branch per measured vibration
mode.

.. image:: ../../../../5.characterization/3.model/bvd_single_mode.png
   :width: 420
   :align: center

The extracted model currently uses five motional branches.  The model scripts
report a globally averaged shunt capacitance of about 8.36 pF and median branch
parameters for C100, B100, A100, C300, and B300.

.. image:: ../../../../5.characterization/3.model/crystal_model.png
   :width: 700
   :align: center

Scripts
-------

``bvd_extractor.py``
   Walks the raw ``.s2p`` dataset, finds resonant peaks, fits BVD branch
   parameters, and writes the extracted parameter set.

``crystal_model.py``
   Instantiates the multi-mode quartz model and plots impedance magnitude over
   the measured frequency span.

Run the model scripts from ``5.characterization/3.model``:

.. code-block:: bash

   python bvd_extractor.py
   python crystal_model.py
