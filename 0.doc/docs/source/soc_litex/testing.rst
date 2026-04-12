Testing and Experiments
=======================

The SoC tree keeps older integration experiments and datapath test tops for
reference, even when they are not part of the active build. These live under
``2.soc/1.hw/testing``.

What belongs here
-----------------

The testing area is for:

- legacy top-level RTL used to try alternative datapaths,
- standalone integration experiments,
- historical reference designs that help explain how the current structure
  evolved,
- one-off validation flows that should not be confused with the main SoC build.

Examples currently kept there include:

- ``adc-dac``
- ``adc-dsp-dac``
- ``adc_cordic_dsp_dac``
- ``cordic-dac``
- ``cordic_dsp_dac``
- ``ledmem``
- ``input-mux``

What does not belong here
-------------------------

- reusable DSP blocks that belong in ``1.dsp/rtl``
- active SoC-specific RTL that belongs in ``2.soc/1.hw``
- build scripts and host tools that belong in ``3.build``

Why this section exists
-----------------------

Without a documented testing area, legacy experiments look like active parts of
the design. Keeping them under ``testing`` and documenting that boundary makes
the active SoC easier to understand.

See also
--------

- :doc:`verilog_ip`
- :doc:`firmware`
