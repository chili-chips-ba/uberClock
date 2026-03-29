#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
uberclock_core.py

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
from migen.genlib.cdc import PulseSynchronizer, MultiReg, ClockDomainsRenamer
from migen.genlib.fifo import AsyncFIFO

from .uberclock_csrs import UberClockCSRBank
from .csr_snapshot_fifo import CsrConfigSnapshotFIFO
from .rtl_sources import add_sources
from .rtl_filelist import UBERCLOCK_RTL_FILES

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
    platform = soc.platform


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
        "ups_in_x1":            m.upsampler_input_x1.storage,
        "ups_in_y1":            m.upsampler_input_y1.storage,
        "ups_in_x2":            m.upsampler_input_x2.storage,
        "ups_in_y2":            m.upsampler_input_y2.storage,
        "ups_in_x3":            m.upsampler_input_x3.storage,
        "ups_in_y3":            m.upsampler_input_y3.storage,
        "ups_in_x4":             m.upsampler_input_x4.storage,
        "ups_in_y4":            m.upsampler_input_y4.storage,
        "ups_in_x5":            m.upsampler_input_x5.storage,
        "ups_in_y5":            m.upsampler_input_y5.storage,
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
    mag_uc   = Signal(16, name="magnitude_uc")
    phase_uc = Signal(25, name="phase_uc")
    # -------------------------------------------------------------------------
    # UberClock instance (UC domain)
    # -------------------------------------------------------------------------
    ds_x_uc1    = Signal(16, name="downsampled_x_uc1")
    ds_y_uc1    = Signal(16, name="downsampled_y_uc1")
    ds_x_uc2    = Signal(16, name="downsampled_x_uc2")
    ds_y_uc2    = Signal(16, name="downsampled_y_uc2")
    ds_x_uc3    = Signal(16, name="downsampled_x_uc3")
    ds_y_uc3    = Signal(16, name="downsampled_y_uc3")
    ds_x_uc4    = Signal(16, name="downsampled_x_uc4")
    ds_y_uc4    = Signal(16, name="downsampled_y_uc4")
    ds_x_uc5    = Signal(16, name="downsampled_x_uc5")
    ds_y_uc5    = Signal(16, name="downsampled_y_uc5")
    cap_sel_uc = Signal(12, name="cap_selected_uc")  # selected sample for HS capture

    # UC-domain upsampler FIFO hold registers
    ups_x_uc1 = Signal(16, name="upsampler_fifo_x1_uc")
    ups_y_uc1 = Signal(16, name="upsampler_fifo_y1_uc")
    ups_x_uc2 = Signal(16, name="upsampler_fifo_x2_uc")
    ups_y_uc2 = Signal(16, name="upsampler_fifo_y2_uc")
    ups_x_uc3 = Signal(16, name="upsampler_fifo_x3_uc")
    ups_y_uc3 = Signal(16, name="upsampler_fifo_y3_uc")
    ups_x_uc4 = Signal(16, name="upsampler_fifo_x4_uc")
    ups_y_uc4 = Signal(16, name="upsampler_fifo_y4_uc")
    ups_x_uc5 = Signal(16, name="upsampler_fifo_x5_uc")
    ups_y_uc5 = Signal(16, name="upsampler_fifo_y5_uc")
    ups_have_sample_uc = Signal(name="upsampler_fifo_has_sample_uc")
    ups_in_x_uc1 = Signal(16, name="upsampler_in_x1_uc")
    ups_in_y_uc1 = Signal(16, name="upsampler_in_y1_uc")
    ups_in_x_uc2 = Signal(16, name="upsampler_in_x2_uc")
    ups_in_y_uc2 = Signal(16, name="upsampler_in_y2_uc")
    ups_in_x_uc3 = Signal(16, name="upsampler_in_x3_uc")
    ups_in_y_uc3 = Signal(16, name="upsampler_in_y3_uc")
    ups_in_x_uc4 = Signal(16, name="upsampler_in_x4_uc")
    ups_in_y_uc4 = Signal(16, name="upsampler_in_y4_uc")
    ups_in_x_uc5 = Signal(16, name="upsampler_in_x5_uc")
    ups_in_y_uc5 = Signal(16, name="upsampler_in_y5_uc")

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

        i_upsampler_input_x1=ups_in_x_uc1,
        i_upsampler_input_y1=ups_in_y_uc1,
        i_upsampler_input_x2=ups_in_x_uc2,
        i_upsampler_input_y2=ups_in_y_uc2,
        i_upsampler_input_x3=ups_in_x_uc3,
        i_upsampler_input_y3=ups_in_y_uc3,
        i_upsampler_input_x4=ups_in_x_uc4,
        i_upsampler_input_y4=ups_in_y_uc4,
        i_upsampler_input_x5=ups_in_x_uc5,
        i_upsampler_input_y5=ups_in_y_uc5,
        i_final_shift=_uc_out("final_shift"),

        # Outputs
        o_ce_down=ce_down_uc,
        o_downsampled_data_x1=ds_x_uc1,
        o_downsampled_data_y1=ds_y_uc1,
        o_downsampled_data_x2=ds_x_uc2,
        o_downsampled_data_y2=ds_y_uc2,
        o_downsampled_data_x3=ds_x_uc3,
        o_downsampled_data_y3=ds_y_uc3,
        o_downsampled_data_x4=ds_x_uc4,
        o_downsampled_data_y4=ds_y_uc4,
        o_downsampled_data_x5=ds_x_uc5,
        o_downsampled_data_y5=ds_y_uc5,

        # High-speed capture sample out
        o_cap_selected_input=cap_sel_uc,

        # Low-speed capture RAM control + readback
        i_cap_arm=_uc_out("cap_arm"),
        i_cap_idx=_uc_out("cap_idx"),
        o_cap_done=cap_done_uc,
        o_cap_data=cap_data_uc,
        o_magnitude=mag_uc,
        o_phase=phase_uc,
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
    mag_sys   = Signal(16, name="magnitude_sys")
    phase_sys = Signal(25, name="phase_sys")

    soc.specials += MultiReg(mag_uc, mag_sys, "sys")
    soc.specials += MultiReg(phase_uc, phase_sys, "sys")

    soc.comb += [
        m.magnitude.status.eq(mag_sys),
        m.phase.status.eq(phase_sys),
    ]
    # -------------------------------------------------------------------------
    # UC->SYS: downsampled data async FIFO (for CPU readback)
    # -------------------------------------------------------------------------
    DS_FIFO_DEPTH = 16384

    ds_fifo_width = 16 * 10
    ds_fifo = AsyncFIFO(width=ds_fifo_width, depth=DS_FIFO_DEPTH)
    soc.submodules.ds_fifo = ClockDomainsRenamer({"write": "uc", "read": "sys"})(ds_fifo)

    ds_overflow_uc = Signal(name="ds_fifo_overflow_uc")
    ds_overflow_sys = Signal(name="ds_fifo_overflow_sys")
    ds_underflow_sys = Signal(name="ds_fifo_underflow_sys")
    ds_clear_uc = Signal(name="ds_fifo_clear_uc")

    # UC write-side: push on ce_down
    soc.comb += [
        ds_fifo.din.eq(Cat(
            ds_x_uc1, ds_y_uc1,
            ds_x_uc2, ds_y_uc2,
            ds_x_uc3, ds_y_uc3,
            ds_x_uc4, ds_y_uc4,
            ds_x_uc5, ds_y_uc5,
        )),
        ds_fifo.we.eq(ce_down_uc & ds_fifo.writable),
    ]

    # SYS->UC clear pulse
    soc.submodules.ds_clear_ps = PulseSynchronizer("sys", "uc")
    soc.comb += soc.ds_clear_ps.i.eq(m.ds_fifo_clear.re)
    soc.comb += ds_clear_uc.eq(soc.ds_clear_ps.o)

    # UC overflow sticky
    soc.sync.uc += [
        If(ce_down_uc & ~ds_fifo.writable,
            ds_overflow_uc.eq(1)
        ),
        If(ds_clear_uc,
            ds_overflow_uc.eq(0)
        ),
    ]

    soc.specials += MultiReg(ds_overflow_uc, ds_overflow_sys, "sys")

    # SYS read-side: pop on CPU strobe
    ds_data_sys = Signal(ds_fifo_width, name="ds_fifo_data_sys")
    ds_pop_sys = Signal(name="ds_fifo_pop_sys")
    soc.comb += ds_pop_sys.eq(m.ds_fifo_pop.re)

    soc.comb += ds_fifo.re.eq(ds_pop_sys & ds_fifo.readable)

    soc.sync.sys += [
        If(ds_pop_sys & ds_fifo.readable,
            ds_data_sys.eq(ds_fifo.dout)
        ),
        If(ds_pop_sys & ~ds_fifo.readable,
            ds_underflow_sys.eq(1)
        ),
        If(m.ds_fifo_clear.re,
            ds_underflow_sys.eq(0)
        ),
    ]

    soc.comb += [
        m.ds_fifo_x1.status.eq(ds_data_sys[0:16]),
        m.ds_fifo_y1.status.eq(ds_data_sys[16:32]),
        m.ds_fifo_x2.status.eq(ds_data_sys[32:48]),
        m.ds_fifo_y2.status.eq(ds_data_sys[48:64]),
        m.ds_fifo_x3.status.eq(ds_data_sys[64:80]),
        m.ds_fifo_y3.status.eq(ds_data_sys[80:96]),
        m.ds_fifo_x4.status.eq(ds_data_sys[96:112]),
        m.ds_fifo_y4.status.eq(ds_data_sys[112:128]),
        m.ds_fifo_x5.status.eq(ds_data_sys[128:144]),
        m.ds_fifo_y5.status.eq(ds_data_sys[144:160]),
        m.ds_fifo_overflow.status.eq(ds_overflow_sys),
        m.ds_fifo_underflow.status.eq(ds_underflow_sys),
        m.ds_fifo_flags.status.eq(Cat(ds_fifo.readable, C(0, 7))),
    ]

    # -------------------------------------------------------------------------
    # SYS->UC: upsampler input async FIFO (CPU injection)
    # -------------------------------------------------------------------------
    UPS_FIFO_DEPTH = 16384

    ups_fifo_width = 16 * 10
    ups_fifo = AsyncFIFO(width=ups_fifo_width, depth=UPS_FIFO_DEPTH)
    soc.submodules.ups_fifo = ClockDomainsRenamer({"write": "sys", "read": "uc"})(ups_fifo)

    ups_overflow_sys = Signal(name="ups_fifo_overflow_sys")
    ups_underflow_uc = Signal(name="ups_fifo_underflow_uc")
    ups_underflow_sys = Signal(name="ups_fifo_underflow_sys")
    ups_clear_uc = Signal(name="ups_fifo_clear_uc")

    # SYS write-side: push on CPU strobe
    ups_push_sys = Signal(name="ups_fifo_push_sys")
    soc.comb += ups_push_sys.eq(m.ups_fifo_push.re)

    soc.comb += [
        ups_fifo.din.eq(Cat(
            m.ups_fifo_x1.storage, m.ups_fifo_y1.storage,
            m.ups_fifo_x2.storage, m.ups_fifo_y2.storage,
            m.ups_fifo_x3.storage, m.ups_fifo_y3.storage,
            m.ups_fifo_x4.storage, m.ups_fifo_y4.storage,
            m.ups_fifo_x5.storage, m.ups_fifo_y5.storage,
        )),
        ups_fifo.we.eq(ups_push_sys & ups_fifo.writable),
    ]

    soc.sync.sys += [
        If(ups_push_sys & ~ups_fifo.writable,
            ups_overflow_sys.eq(1)
        ),
        If(m.ups_fifo_clear.re,
            ups_overflow_sys.eq(0)
        ),
    ]

    soc.comb += [
        m.ups_fifo_overflow.status.eq(ups_overflow_sys),
        m.ups_fifo_underflow.status.eq(ups_underflow_sys),
        m.ups_fifo_flags.status.eq(Cat(C(0, 1), ups_fifo.writable, C(0, 6))),
    ]

    # UC read-side: pop on ce_down, hold last sample
    soc.comb += ups_fifo.re.eq(ce_down_uc & ups_fifo.readable)

    soc.sync.uc += If(ce_down_uc & ups_fifo.readable,
        ups_x_uc1.eq(ups_fifo.dout[0:16]),
        ups_y_uc1.eq(ups_fifo.dout[16:32]),
        ups_x_uc2.eq(ups_fifo.dout[32:48]),
        ups_y_uc2.eq(ups_fifo.dout[48:64]),
        ups_x_uc3.eq(ups_fifo.dout[64:80]),
        ups_y_uc3.eq(ups_fifo.dout[80:96]),
        ups_x_uc4.eq(ups_fifo.dout[96:112]),
        ups_y_uc4.eq(ups_fifo.dout[112:128]),
        ups_x_uc5.eq(ups_fifo.dout[128:144]),
        ups_y_uc5.eq(ups_fifo.dout[144:160]),
        ups_have_sample_uc.eq(1)
    )

    soc.sync.uc += [
        If(ce_down_uc & ~ups_fifo.readable & ups_have_sample_uc,
            ups_underflow_uc.eq(1)
        ),
        If(ups_clear_uc,
            ups_underflow_uc.eq(0)
        ),
    ]

    soc.specials += MultiReg(ups_underflow_uc, ups_underflow_sys, "sys")

    # SYS->UC clear pulse
    soc.submodules.ups_clear_ps = PulseSynchronizer("sys", "uc")
    soc.comb += soc.ups_clear_ps.i.eq(m.ups_fifo_clear.re)
    soc.comb += ups_clear_uc.eq(soc.ups_clear_ps.o)

    # Upsampler input always comes from FIFO; before first sample, drive zero.
    soc.comb += [
        ups_in_x_uc1.eq(Mux(ups_have_sample_uc, ups_x_uc1, C(0, 16))),
        ups_in_y_uc1.eq(Mux(ups_have_sample_uc, ups_y_uc1, C(0, 16))),
        ups_in_x_uc2.eq(Mux(ups_have_sample_uc, ups_x_uc2, C(0, 16))),
        ups_in_y_uc2.eq(Mux(ups_have_sample_uc, ups_y_uc2, C(0, 16))),
        ups_in_x_uc3.eq(Mux(ups_have_sample_uc, ups_x_uc3, C(0, 16))),
        ups_in_y_uc3.eq(Mux(ups_have_sample_uc, ups_y_uc3, C(0, 16))),
        ups_in_x_uc4.eq(Mux(ups_have_sample_uc, ups_x_uc4, C(0, 16))),
        ups_in_y_uc4.eq(Mux(ups_have_sample_uc, ups_y_uc4, C(0, 16))),
        ups_in_x_uc5.eq(Mux(ups_have_sample_uc, ups_x_uc5, C(0, 16))),
        ups_in_y_uc5.eq(Mux(ups_have_sample_uc, ups_y_uc5, C(0, 16))),
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
