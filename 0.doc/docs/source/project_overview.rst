Project Overview
==========================

Clocks can be extracted from GPS satellite signals, or locally generated with
MEMS oscillators, SAW resonators, quartz crystal (XTAL, XO) or piezo
resonators, often set in ovens (TCXO, OCXO), derived from atomic properties
(like Cesium Beam, Hydrogen Maser, Rubidium, Strontium or Ytterbium), or
obtained in another way.

.. image:: ../../../0.doc/artwork/uberClock.sticker.png
   :width: 250
   :align: center

They differ in absolute accuracy, long-term frequency stability, short-term
frequency stability, phase noise, physical size, complexity, immunity to
external interference, power consumption, cost, and other characteristics.
These differences are categorized as clock strata, whereby a clock source must
meet a standardized set of requirements for each stratum level.

    This work is about researching and exploiting the properties of multi-mode
    crystal oscillators in order to achieve stability comparable to a Stratum 2
    Rubidium clock, all at a fraction of the total cost of ownership. We plan
    on collecting large empirical datasets, constructing experimental
    prototypes, and using DSP / numerical methods to mitigate (1) temperature
    variations, (2) dynamic acceleration and (3) static gravity effects. The
    project aims for XTAL frequency stability by means of numerous mathematical
    calculations performed in FPGA, using open-source tools, including
    CflexHDL+PipelineC HLS flow.

This is a Proof-of-Concept (PoC) and stepping stone for future applied research
projects on this theme, possibly extending into the field of Artificial
Intelligence. In addition to a working prototype (PCBs, FPGA gateware and
embedded firmware), the project will deliver a series of scientific papers.

References
----------

- `Unleashing the Mystery of Crystal Cuts <https://xoprof.com/2023/09/unleashing-the-mystery-of-crystal-cuts>`_
- `Quartz Crystal Cuts: AT, BT, SC, CT <https://www.electronics-notes.com/articles/electronic_components/quartz-crystal-xtal/crystal-resonator-cuts-at-bt-sc-ct.php>`_
- `It's All about the Angle - The AT-Cut for Quartz Crystals <https://www.jauch.com/blog/en/its-all-about-the-angle-the-at-cut-for-quartz-crystals>`_
- `Oscillator aging and its importance in precision timing <https://www.sitime.com/company/newsroom/articles/oscillator-aging-and-its-importance-in-precision-timing>`_
- `Python tool for filter design <https://github.com/chipmuenk/pyfda>`_ with `video demo <https://www.youtube.com/watch?v=IDKKr-ry9tc>`_
- `DSP Filters in AmaranthHDL <https://github.com/amaranth-farm/amlib/tree/main/amlib/dsp>`_
- `DSP Filters in Verilog <https://github.com/ZipCPU/dspfilters>`_
- Papers are stored in ``0.doc/Quartz/papers`` in the repository.

Hardware Platform
-----------------

- **Physics Package**: full-custom `analog board <https://github.com/jdbrinton/uberclock>`_ with multi-mode quartz crystal

.. image:: ../../../0.doc/Quartz/Analog-Card.1.jpg
   :width: 250
   :align: center

.. image:: ../../../0.doc/Quartz/Analog-Card.2.png
   :width: 200
   :align: center

- `AX7203 Artix7-200 FPGA Board <https://www.en.alinx.com/Product/FPGA-Development-Boards/Artix-7/AX7203.html>`_

.. image:: ../../../0.doc/Alinx/FPGA-Board--Artix7-200--AX7203.jpg
   :width: 600
   :align: center

- `AN9238 2xADC, 65 MSPS, 12-bit <https://www.en.alinx.com/Product/Add-on-Modules/AN9238.html>`_

.. image:: ../../../0.doc/Alinx/2xADC--65MSPS-12bit--AN9238.jpg
   :width: 300
   :align: center

- `AN9767 2xDAC, 125 MSPS, 14-bit <https://www.en.alinx.com/Product/Add-on-Modules/AN9767.html>`_

.. image:: ../../../0.doc/Alinx/2xDAC--125MSPS-14bit--AN9767.jpg
   :width: 300
   :align: center

Project Status
--------------

1. Acquisition of Hardware Platform

   - [x] Design, manufacture and debug the Physics Package card.
   - [x] Procure and distribute FPGA, ADC and DAC cards.

2. Digital Infrastructure

   - [x] Familiarize with ALINX boards.

     - Toggle LEDs.
     - Write RTL for interfaces to ADC and DAC chips.
     - Write RTL to test their operation.
     - Perform testing, debug, and fix the problems as they arise.

   - [x] Create CPU hardware subsystem based on an open-source RISC-V core,
     memories, UART and debug port.
   - [ ] Create a bare-metal software skeleton as the foundation for writing
     future DSP applications, then create and test the software build flow.
   - [ ] Test operation of CPU subsystem and profile its performance.
   - [ ] Map ADCs and DACs into CPU memory space and test software
     communication with them.

3. DSP Model and Documentation

   - [ ] Model quartz crystal and DSP datapath in C or Python.
   - [ ] Create a *Theory of Operation* document with explanation of concepts,
     tradeoffs and criteria used to devise solutions.
   - [ ] Post the *Executive Summary* here.

4. Integration and Characterization

   - [ ] Bring up the complete system with digital and analog card connected to
     each other.
   - [ ] Perform manual characterization of individual crystals.
   - [ ] Develop a semi- or fully-automated crystal characterization procedure.

5. Implementation of DSP Algorithms

   - [ ] Implement the hardware side of the DSP algorithm on FPGA.
   - [ ] Implement the software side of the DSP algorithm in the RISC-V CPU.
   - [ ] Integrate DSP hardware and software into a complete system.

6. Benchmarking

   - [ ] Test the DSP together with crystal.

     - A reliable reference clock source is needed, preferably Stratum 0.
     - A good spectrum analyzer is needed.

   - [ ] Fine-tune the DSP algorithm based on the obtained measurements.
   - [ ] Conduct additional experiments with the corrected DSP algorithm in
     simulation and on hardware.

7. Port from Vivado to openXC7

   - [ ] Port from Vivado to openXC7.

DSP Theory of Operation
-----------------------

- WIP

Bit-Accurate Models
-------------------

Multi-mode Quartz Crystal
~~~~~~~~~~~~~~~~~~~~~~~~~

- WIP

DSP Datapath
~~~~~~~~~~~~

- WIP

Bit-Accurate Simulation of the Entire Algorithm
-----------------------------------------------

- WIP

Hardware Architecture
---------------------

- WIP

.. image:: ../../../0.doc/HW_architecture.png
   :width: 800
   :align: center

Software Architecture
---------------------

- WIP

Acknowledgements
----------------

We are grateful to NLnet Foundation for their sponsorship of this development
activity.

.. image:: https://github.com/chili-chips-ba/openeye/assets/67533663/18e7db5c-8c52-406b-a58e-8860caa327c2
   :align: center

.. image:: https://github.com/chili-chips-ba/openeye-CamSI/assets/67533663/013684f5-d530-42ab-807d-b4afd34c1522
   :width: 115
   :align: center

Public Posts
------------

- `2025-05-25 <https://www.linkedin.com/posts/chili-chips_adc-dac-riscv-activity-7332575086975078400-41q6/?utm_source=share&utm_medium=member_desktop&rcm=ACoAAAJv-TcBSi_5ff0VNMrInrT-xg44YF3jnyU>`_
- `2025-02-10 <https://www.linkedin.com/posts/chili-chips_dsp-adc-cordic-activity-7294943218167689216-ZzOs?utm_source=share&utm_medium=member_desktop>`_
