#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
# uberclock_soc/rtl_filelist.py
RTL manifest for the UberClock DSP core.

This file only describes which RTL files belong to UberClock project.
"""

from __future__ import annotations
from typing import Sequence

UBERCLOCK_RTL_FILES: Sequence[str] = (
    # ------------------------------------------------------------------
    # UberClock top + channels
    # ------------------------------------------------------------------
    "2.soc/1.hw/uberclock/uberclock.v",
    "2.soc/1.hw/uberclock/rx_channel.v",
    "2.soc/1.hw/uberclock/tx_channel.v",

    # ------------------------------------------------------------------
    # ADC / DAC interfaces
    # ------------------------------------------------------------------
    "1.dsp/rtl/adc/adc.v",
    "1.dsp/rtl/dac/dac.v",

    # ------------------------------------------------------------------
    # Filters + coefficient memories
    # ------------------------------------------------------------------
    "1.dsp/rtl/filters/cic.v",
    "1.dsp/rtl/filters/cic_comp_down_mac.v",
    "1.dsp/rtl/filters/comp_down_coeffs.mem",
    "1.dsp/rtl/filters/hb_down_mac.v",
    "1.dsp/rtl/filters/hb_down_coeffs.mem",
    "1.dsp/rtl/filters/downsamplerFilter.v",
    "1.dsp/rtl/filters/upsamplerFilter.v",
    "1.dsp/rtl/filters/hb_up_mac.v",
    "1.dsp/rtl/filters/coeffs.mem",
    "1.dsp/rtl/filters/cic_comp_up_mac.v",
    "1.dsp/rtl/filters/coeffs_comp.mem",
    "1.dsp/rtl/filters/cic_int.v",

    # ------------------------------------------------------------------
    # Polar conversion
    # ------------------------------------------------------------------
    "1.dsp/rtl/to_polar/to_polar.v",

    # ------------------------------------------------------------------
    # CORDIC (full precision)
    # ------------------------------------------------------------------
    "1.dsp/rtl/cordic/cordic_pre_rotate.v",
    "1.dsp/rtl/cordic/cordic_pipeline_stage.v",
    "1.dsp/rtl/cordic/cordic_round.v",
    "1.dsp/rtl/cordic/cordic.v",
    "1.dsp/rtl/cordic/cordic_logic.v",
    "1.dsp/rtl/cordic/gain_and_saturate.v",

    # ------------------------------------------------------------------
    # CORDIC16 variant
    # ------------------------------------------------------------------
    "1.dsp/rtl/cordic16/cordic16.v",
    "1.dsp/rtl/cordic16/cordic_pre_rotate_16.v",
)

UBERDDR3_RTL_FILES: Sequence[str] = (
    "2.soc/1.hw/memory/ddr3_top.v",
    "2.soc/1.hw/memory/ddr3_controller.v",
    "2.soc/1.hw/memory/ddr3_phy.v",
    "2.soc/1.hw/memory/wbc2pipeline.v",
    "2.soc/1.hw/memory/wbxbar.v",
    "2.soc/1.hw/memory/skidbuffer.v",
    "2.soc/1.hw/memory/addrdecode.v",
    "2.soc/1.hw/memory/zipdma_s2mm.v",
)
