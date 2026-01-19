#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
uberclock_block.py

Use-case
--------
This file integrates the Verilog top-level `uberclock` DSP core into a LiteX SoC.

It does four things:

1) Adds all required RTL sources (.v + .mem) to the LiteX build.
2) Exposes the UberClock configuration surface as CSRs in the SYS clock domain.
3) Transfers configuration snapshots atomically SYS->UC via an Async FIFO
   (so the UC domain sees stable/consistent configuration updates).
4) Bridges UC->SYS events/data:
   - `ce_down` pulse becomes a LiteX interrupt (EventManager) in SYS.
   - optional low-speed capture readback returns to SYS CSRs
   - optional high-speed capture sample is wired into UberDDR3 (if present)

Expected SoC conventions
------------------------
- SoC provides clock domains:
    * sys : CPU/CSR domain
    * uc  : UberClock domain (e.g. exact 65 MHz)
- The platform defines IOs:
    * adc_clk_ch0/1, adc_data_ch0/1
    * da1_clk/da1_wrt/da1_data and da2_clk/da2_wrt/da2_data
    * user_led (or you pass a Cat(...) of pads via `leds`)
- `uberclock.v` matches the port names wired in `ports` below.

Notes
-----
- `CSRConfigAFIFO` produces UC-domain signals named `out_<field>_uc`.
  This module simply maps those to the Verilog instance inputs.
- All logic and signal wiring is unchanged from your current implementation;
  this is a readability + correctness refactor (e.g. `platform` defined, consistent naming).
"""

from __future__ import annotations

from migen import *
from litex.gen import *

from litex.soc.interconnect.csr_eventmanager import EventManager, EventSourcePulse
from migen.genlib.cdc import PulseSynchronizer, MultiReg

from .uberclock_csrs import UberClockCSRBank
from .csr_snapshot_fifo import CsrConfigSnapshotFIFO
from .rtl_sources import add_sources


# -----------------------------------------------------------------------------
# RTL file list (relative to your RTL root, e.g. ../1.hw/)
# -----------------------------------------------------------------------------
UBERCLOCK_RTL_FILES = [
    # UberClock top + channels
    "uberclock/uberclock.v",
    "uberclock/rx_channel.v",
    "uberclock/tx_channel.v",

    # ADC/DAC interface blocks
    "adc/adc.v",
    "dac/dac.v",

    # Filters + coefficient memories
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

    # Polar conversion
    "to_polar/to_polar.v",

    # CORDIC (full precision)
    "cordic/cordic_pre_rotate.v",
    "cordic/cordic_pipeline_stage.v",
    "cordic/cordic_round.v",
    "cordic/cordic.v",
    "cordic/cordic_logic.v",
    "cordic/gain_and_saturate.v",

    # CORDIC16 variant
    "cordic16/cordic16.v",
    "cordic16/cordic_pre_rotate_16.v",
]


def add_uberclock_fullrate(soc, leds):
    """
    Integrate UberClock into the SoC (UC domain).

    Parameters
    ----------
    soc:
        LiteX SoC instance (must have `platform`, `irq`, `add_csr`, and clock domains).
    leds:
        A Cat(...) of LED pads (or any signal vector) used for optional activity indicator.

    Side effects
    ------------
    - Adds RTL sources
    - Adds CSRs:
        * main      : UberClock configuration & capture controls
        * evm       : interrupt source (ce_down)
        * cfg_link  : CSR snapshot commit FIFO
    - Instantiates the Verilog `uberclock` module.
    - Optionally wires high-speed capture into `soc.ubddr3` if it exists.
    """
    platform = soc.platform  # needed by add_sources() and request()

    # -------------------------------------------------------------------------
    # Add required Verilog sources for UberClock
    # -------------------------------------------------------------------------
    add_sources(platform, UBERCLOCK_RTL_FILES)

    # -------------------------------------------------------------------------
    # Pads
    # -------------------------------------------------------------------------
    adc_clk_ch0  = platform.request("adc_clk_ch0")
    adc_clk_ch1  = platform.request("adc_clk_ch1")
    adc_data_ch0 = platform.request("adc_data_ch0")
    adc_data_ch1 = platform.request("adc_data_ch1")

    da1_clk  = platform.request("da1_clk")
    da1_wrt  = platform.request("da1_wrt")
    da1_data = platform.request("da1_data")

    da2_clk  = platform.request("da2_clk")
    da2_wrt  = platform.request("da2_wrt")
    da2_data = platform.request("da2_data")

    # -------------------------------------------------------------------------
    # CSRs: UberClock configuration surface (SYS domain)
    # -------------------------------------------------------------------------
    soc.submodules.main = UberClockCSRBank()
    soc.add_csr("main")
    m = soc.main

    # -------------------------------------------------------------------------
    # EventManager: UC->SYS `ce_down` pulse becomes a LiteX interrupt
    # -------------------------------------------------------------------------
    soc.submodules.evm = EventManager()
    soc.evm.ce_down = EventSourcePulse(description="Downsample ready pulse (uc->sys).")
    soc.evm.finalize()
    soc.irq.add("evm")
    soc.add_csr("evm")

    # -------------------------------------------------------------------------
    # SYS->UC configuration snapshot FIFO
    # -------------------------------------------------------------------------
    # Each dictionary entry becomes an `out_<name>_uc` Signal in UC domain.
    cfg_sys = {
        # Muxing / IO routing
        "input_select":         m.input_select.storage,
        "output_sel_ch1":       m.output_select_ch1.storage,
        "output_sel_ch2":       m.output_select_ch2.storage,
        "upsampler_input_mux":  m.upsampler_input_mux.storage,

        # Main NCO (shared)
        "phase_inc_nco":        m.phase_inc_nco.storage,
        "nco_mag":              m.nco_mag.storage,

        # Downsample NCOs
        "phase_inc_down_1":     m.phase_inc_down_1.storage,
        "phase_inc_down_2":     m.phase_inc_down_2.storage,
        "phase_inc_down_3":     m.phase_inc_down_3.storage,
        "phase_inc_down_4":     m.phase_inc_down_4.storage,
        "phase_inc_down_5":     m.phase_inc_down_5.storage,
        "phase_inc_down_ref":   m.phase_inc_down_ref.storage,

        # CPU-driven NCOs (per-channel)
        "phase_inc_cpu1":       m.phase_inc_cpu1.storage,
        "phase_inc_cpu2":       m.phase_inc_cpu2.storage,
        "phase_inc_cpu3":       m.phase_inc_cpu3.storage,
        "phase_inc_cpu4":       m.phase_inc_cpu4.storage,
        "phase_inc_cpu5":       m.phase_inc_cpu5.storage,

        "mag_cpu1":             m.mag_cpu1.storage,
        "mag_cpu2":             m.mag_cpu2.storage,
        "mag_cpu3":             m.mag_cpu3.storage,
        "mag_cpu4":             m.mag_cpu4.storage,
        "mag_cpu5":             m.mag_cpu5.storage,

        # Debug selection
        "lowspeed_dbg_select":  m.lowspeed_dbg_select.storage,
        "highspeed_dbg_select": m.highspeed_dbg_select.storage,

        # DSP gains
        "gain1":                m.gain1.storage,
        "gain2":                m.gain2.storage,
        "gain3":                m.gain3.storage,
        "gain4":                m.gain4.storage,
        "gain5":                m.gain5.storage,

        # CPU-fed upsampler injection
        "ups_in_x":             m.upsampler_input_x.storage,
        "ups_in_y":             m.upsampler_input_y.storage,
        "final_shift":          m.final_shift.storage,

        # High-speed DDR capture control (used if soc.ubddr3 exists)
        "cap_enable":           m.cap_enable.storage,
        "cap_beats":            m.cap_beats.storage,

        # Low-speed capture RAM control (if present in uberclock.v)
        "cap_arm":              m.cap_arm.storage,
        "cap_idx":              m.cap_idx.storage,
    }

    soc.submodules.cfg_link = CsrConfigSnapshotFIFO(cfg_sys, cd_write="sys", cd_read="uc", fifo_depth=4)
    soc.add_csr("cfg_link")
    uc = soc.cfg_link  # convenience alias

    # -------------------------------------------------------------------------
    # UC->SYS: `ce_down` pulse crossing to trigger EventManager
    # -------------------------------------------------------------------------
    ce_down_uc  = Signal(name="ce_down_uc")
    ce_down_sys = Signal(name="ce_down_sys")

    soc.submodules.ps_down = PulseSynchronizer("uc", "sys")
    soc.comb += [
        soc.ps_down.i.eq(ce_down_uc),
        ce_down_sys.eq(soc.ps_down.o),
        soc.evm.ce_down.trigger.eq(ce_down_sys),
    ]

    # -------------------------------------------------------------------------
    # UberClock instance (UC domain)
    # -------------------------------------------------------------------------
    ds_x_uc    = Signal(16, name="downsampled_x_uc")
    ds_y_uc    = Signal(16, name="downsampled_y_uc")
    cap_sel_uc = Signal(12, name="cap_selected_uc")  # selected sample for HS capture

    # Low-speed capture outputs (UC domain)
    cap_done_uc = Signal(name="ls_cap_done_uc")
    cap_data_uc = Signal(16, name="ls_cap_data_uc")

    # Small helper for nicer attribute errors if a field name changes
    def _uc_out(name: str):
        return getattr(uc, f"cfg_{name}_uc")

    ports = dict(
        # Clock/reset (UC domain)
        i_sys_clk=ClockSignal("uc"),
        i_rst=ResetSignal("uc"),

        # ADC IO
        o_adc_clk_ch0=adc_clk_ch0,
        o_adc_clk_ch1=adc_clk_ch1,
        i_adc_data_ch0=adc_data_ch0,
        i_adc_data_ch1=adc_data_ch1,

        # DAC IO
        o_da1_clk=da1_clk,
        o_da1_wrt=da1_wrt,
        o_da1_data=da1_data,

        o_da2_clk=da2_clk,
        o_da2_wrt=da2_wrt,
        o_da2_data=da2_data,

        # Config / control (UC domain snapshot outputs)
        i_input_select=_uc_out("input_select"),
        i_output_select_ch1=_uc_out("output_sel_ch1"),
        i_output_select_ch2=_uc_out("output_sel_ch2"),
        i_upsampler_input_mux=_uc_out("upsampler_input_mux"),

        i_phase_inc_nco=_uc_out("phase_inc_nco"),
        i_nco_mag=_uc_out("nco_mag"),

        i_phase_inc_down_1=_uc_out("phase_inc_down_1"),
        i_phase_inc_down_2=_uc_out("phase_inc_down_2"),
        i_phase_inc_down_3=_uc_out("phase_inc_down_3"),
        i_phase_inc_down_4=_uc_out("phase_inc_down_4"),
        i_phase_inc_down_5=_uc_out("phase_inc_down_5"),
        i_phase_inc_down_ref=_uc_out("phase_inc_down_ref"),

        i_phase_inc_cpu1=_uc_out("phase_inc_cpu1"),
        i_phase_inc_cpu2=_uc_out("phase_inc_cpu2"),
        i_phase_inc_cpu3=_uc_out("phase_inc_cpu3"),
        i_phase_inc_cpu4=_uc_out("phase_inc_cpu4"),
        i_phase_inc_cpu5=_uc_out("phase_inc_cpu5"),

        i_mag_cpu1=_uc_out("mag_cpu1"),
        i_mag_cpu2=_uc_out("mag_cpu2"),
        i_mag_cpu3=_uc_out("mag_cpu3"),
        i_mag_cpu4=_uc_out("mag_cpu4"),
        i_mag_cpu5=_uc_out("mag_cpu5"),

        i_lowspeed_dbg_select=_uc_out("lowspeed_dbg_select"),
        i_highspeed_dbg_select=_uc_out("highspeed_dbg_select"),

        i_gain1=_uc_out("gain1"),
        i_gain2=_uc_out("gain2"),
        i_gain3=_uc_out("gain3"),
        i_gain4=_uc_out("gain4"),
        i_gain5=_uc_out("gain5"),

        i_upsampler_input_x=_uc_out("ups_in_x"),
        i_upsampler_input_y=_uc_out("ups_in_y"),
        i_final_shift=_uc_out("final_shift"),

        # Outputs
        o_ce_down=ce_down_uc,
        o_downsampled_data_x=ds_x_uc,
        o_downsampled_data_y=ds_y_uc,

        # High-speed capture sample out
        o_cap_selected_input=cap_sel_uc,

        # Low-speed capture RAM control + readback
        i_cap_arm=_uc_out("cap_arm"),
        i_cap_idx=_uc_out("cap_idx"),
        o_cap_done=cap_done_uc,
        o_cap_data=cap_data_uc,
    )

    soc.specials += Instance("uberclock", **ports)

    # -------------------------------------------------------------------------
    # UC->SYS: low-speed capture readback into CSRs (safe CDC)
    # -------------------------------------------------------------------------
    cap_done_sys = Signal(name="ls_cap_done_sys")
    cap_data_sys = Signal(16, name="ls_cap_data_sys")

    soc.specials += MultiReg(cap_done_uc, cap_done_sys, "sys")
    soc.specials += MultiReg(cap_data_uc, cap_data_sys, "sys")

    soc.comb += [
        m.cap_done.status.eq(cap_done_sys),
        m.cap_data.status.eq(cap_data_sys),
    ]

    # -------------------------------------------------------------------------
    # Optional: wire high-speed capture sample into UberDDR3 (if present)
    # -------------------------------------------------------------------------
    if hasattr(soc, "ubddr3"):
        soc.comb += [
            soc.ubddr3.cap_sample.eq(cap_sel_uc),
            soc.ubddr3.cap_enable_uc.eq(_uc_out("cap_enable")),
            soc.ubddr3.cap_beats_uc.eq(_uc_out("cap_beats")),
        ]

    # -------------------------------------------------------------------------
    # Optional: LED activity indicator (ce_down pulse stretched in SYS domain)
    # -------------------------------------------------------------------------
    ce_stretch = Signal(8, name="ce_down_led_stretch")
    soc.sync.sys += [
        If(ce_down_sys,
            ce_stretch.eq(0xFF)
        ).Elif(ce_stretch != 0,
            ce_stretch.eq(ce_stretch - 1)
        )
    ]
    soc.comb += leds[2].eq(ce_stretch != 0)
