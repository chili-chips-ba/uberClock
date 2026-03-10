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
    "uberclock/uberclock.v",
    "uberclock/rx_channel.v",
    "uberclock/tx_channel.v",

    # ------------------------------------------------------------------
    # ADC / DAC interfaces
    # ------------------------------------------------------------------
    "adc/adc.v",
    "dac/dac.v",

    # ------------------------------------------------------------------
    # Filters + coefficient memories
    # ------------------------------------------------------------------
    "filters/cic.v",
    "filters/cic_comp_down_mac.v",
    "filters/comp_down_coeffs.mem",
    "filters/hb_down_mac.v",
    "filters/hb_down_coeffs.mem",
    "filters/downsamplerFilter.v",
    "filters/upsamplerFilter.v",
    "filters/hb_up_mac.v",
    "filters/coeffs.mem",
    "filters/cic_comp_up_mac.v",
    "filters/coeffs_comp.mem",
    "filters/cic_int.v",

    # ------------------------------------------------------------------
    # Polar conversion
    # ------------------------------------------------------------------
    "to_polar/to_polar.v",

    # ------------------------------------------------------------------
    # CORDIC (full precision)
    # ------------------------------------------------------------------
    "cordic/cordic_pre_rotate.v",
    "cordic/cordic_pipeline_stage.v",
    "cordic/cordic_round.v",
    "cordic/cordic.v",
    "cordic/cordic_logic.v",
    "cordic/gain_and_saturate.v",

    # ------------------------------------------------------------------
    # CORDIC16 variant
    # ------------------------------------------------------------------
    "cordic16/cordic16.v",
    "cordic16/cordic_pre_rotate_16.v",
)

UBERDDR3_RTL_FILES: Sequence[str] = (
    "memory/ddr3_top.v",
    "memory/ddr3_controller.v",
    "memory/ddr3_phy.v",
    "memory/wbc2pipeline.v",
    "memory/wbxbar.v",
    "memory/skidbuffer.v",
    "memory/addrdecode.v",
    "memory/zipdma_s2mm.v",
)