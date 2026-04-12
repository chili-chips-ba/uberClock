DSP RTL Blocks
==============

The reusable DSP RTL lives under ``1.dsp/rtl``. This page summarizes the
current block inventory and the role each group plays in the overall signal
path.

Signal-path view
----------------

At a high level, the reusable DSP chain is built from:

- ADC interface blocks
- CORDIC and polar conversion blocks
- decimation / interpolation filters
- DAC interface blocks

The active SoC pulls these blocks into ``2.soc/1.hw/uberclock`` and connects
them with CSR-controlled routing and capture logic.

ADC
---

``adc/adc.v``
   Board-facing ADC interface wrapper. This is the reusable ingress block that
   turns sampled converter data into the internal datapath representation used
   by the SoC wrappers.

DAC
---

``dac/dac.v``
   Main DAC interface used by the active SoC output path.

``dac/dac_control.v``
   DAC-side helper/control logic associated with the output path timing and
   write interface.

``dac/dac_only.v``
   Reduced DAC-oriented variant useful for isolated output-path experiments.

Filters
-------

``filters/cic.v`` and ``filters/cic_int.v``
   Core CIC structures used for efficient sample-rate change.

``filters/downsamplerFilter.v`` and ``filters/upsamplerFilter.v``
   Top-level rate-change filter wrappers used by the SoC datapath.

``filters/hb_down_mac.v`` and ``filters/hb_up_mac.v``
   Half-band filter MAC implementations for decimation and interpolation.

``filters/hb_down_opt.v`` and ``filters/cic_comp_down_opt.v``
   Optimized variants for selected downsampling stages.

``filters/cic_comp_down_mac.v`` and ``filters/cic_comp_up_mac.v``
   CIC compensation stages that restore passband quality after coarse CIC rate
   conversion.

Coefficient memories:

- ``coeffs.mem``
- ``coeffs_comp.mem``
- ``comp_down_coeffs.mem``
- ``hb_down_coeffs.mem``

CORDIC
------

Full-precision CORDIC blocks under ``cordic/``:

- ``cordic.v``
- ``cordic_logic.v``
- ``cordic_pipeline_stage.v``
- ``cordic_pre_rotate.v``
- ``cordic_round.v``
- ``cordic_top.v``
- ``gain_and_saturate.v``

These files implement the main rotation/vectoring datapath used for NCO and
frequency-conversion style operations in the SoC.

CORDIC16
--------

Reduced-width CORDIC blocks under ``cordic16/``:

- ``cordic16.v``
- ``cordic_pipeline_stage.v``
- ``cordic_pre_rotate_16.v``
- ``cordic_round.v``
- ``gain_and_saturate.v``

This set provides a narrower variant of the same basic algorithmic structure,
useful where the full-width implementation is unnecessary.

Polar conversion
----------------

``to_polar/to_polar.v``
   Converts Cartesian-style I/Q style data into a polar-domain representation.
   This bridges the CORDIC-oriented math path with magnitude/phase style
   processing.

Placement rule
--------------

The rule for ``1.dsp/rtl`` is:

- reusable DSP or converter-interface blocks belong here,
- SoC-specific wrappers, memory/control logic, and board integration do not.

Those SoC-specific parts belong in ``2.soc/1.hw`` instead.
