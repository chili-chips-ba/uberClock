#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# -----------------------------------------------------------------------------
# AX7203 + LiteX SoC for UberClock + UberDDR3:
#
# - sys clock domain @ 100 MHz  (CPU/CSR, DDR controller side)
# - uc  clock domain @  65 MHz  (UberClock DSP core)
# - ub_4x / ub_4x_dqs @ 400 MHz (DDR3 PHY clocks)
#
# Features:
#   * Standard LiteX SoC (VexRiscv by default) on AX7203
#   * Optional standard LiteDRAM controller (litedram)
#   * Optional external UberDDR3 controller with:
#       - Classic WB from CPU -> width-converted + c2p bridge -> wbxbar
#       - zipdma_s2mm master writing into DDR3
#   * UberClock DSP block running in uc domain, configured via CSR snapshot FIFO
#   * Simple EventManager for downsample strobe (ce_down)
# -----------------------------------------------------------------------------


from migen import *
from litex.gen import *

from litex_boards.platforms import alinx_ax7203

from litex.soc.interconnect import wishbone as wb
from litex.soc.interconnect.csr import CSRStorage, CSRStatus
from litex.soc.interconnect.csr_eventmanager import EventManager, EventSourcePulse
from litex.soc.integration.soc_core import SoCCore
from litex.soc.integration.builder import Builder
from litex.soc.integration.soc import SoCRegion

from litex.soc.cores.clock import S7MMCM, S7IDELAYCTRL
from litex.soc.cores.led import LedChaser
from litex.soc.cores.timer import Timer
from litex.soc.cores.video import VideoS7HDMIPHY

from litedram.modules import MT41J256M16
from litedram.phy import s7ddrphy
from litedram.core.controller import ControllerSettings

from liteeth.phy.s7rgmii import LiteEthPHYRGMII

from migen.genlib.cdc import PulseSynchronizer, ClockDomainsRenamer
from migen.genlib.fifo import AsyncFIFO, SyncFIFO

import math


# --- set your repo paths ---
#   repository_dir:  root of your uberClock repo
#   verilog_dir:     directory that contains the Verilog sources used below
repository_dir = "/home/ahmed/ws/uberClock"
verilog_dir    = repository_dir + "/2.soc-litex/1.hw"


# =============================================================================
#                           CRG  (Clock/Reset Generation)
# =============================================================================
class _CRG(LiteXModule):
    """
    Clock/Reset generator for AX7203.

    - Uses on-board 200 MHz differential clock.
    - Generates:
        * cd_sys      @ 100 MHz               (LiteX system domain)
        * cd_uc       @  65 MHz               (UberClock domain)
        * cd_ub_4x    @ 400 MHz               (DDR3 internal clock)
        * cd_ub_4x_dqs@ 400 MHz phase-shifted (DDR3 DQS)
        * cd_idelay   @ 200 MHz               (IDELAY regulation)
    """
    def __init__(self, platform, need_ddr_clks=True):
        self.rst          = Signal()
        self.cd_sys       = ClockDomain()
        self.cd_uc        = ClockDomain()
        self.cd_ub_4x     = ClockDomain()
        self.cd_ub_4x_dqs = ClockDomain()
        self.cd_idelay    = ClockDomain()

        clk200    = platform.request("clk200")
        clk200_se = Signal()

        # Convert diff -> single-ended
        self.specials += Instance("IBUFDS",
            i_I  = clk200.p,
            i_IB = clk200.n,
            o_O  = clk200_se
        )

        # First MMCM: sys + DDR clocks
        self.pll0 = S7MMCM(speedgrade=-2)
        self.comb += self.pll0.reset.eq(self.rst)
        self.pll0.register_clkin(clk200_se, 200e6)

        margin = 1e-2
        self.pll0.create_clkout(self.cd_sys, 100e6, margin=margin)
        if need_ddr_clks:
            self.pll0.create_clkout(self.cd_ub_4x,     400e6,           margin=margin)
            self.pll0.create_clkout(self.cd_ub_4x_dqs, 400e6, phase=90, margin=margin)

        # Second MMCM: UberClock domain (65 MHz)
        self.pll1 = S7MMCM(speedgrade=-2)
        self.comb += self.pll1.reset.eq(self.rst)
        self.pll1.register_clkin(clk200_se, 200e6)
        self.pll1.create_clkout(self.cd_uc, 65e6, margin=margin)

        # IDELAYCTRL clock (200 MHz BUFG)
        clk200_bufg = Signal()
        self.specials += Instance("BUFG", i_I=clk200_se, o_O=clk200_bufg)
        self.comb += self.cd_idelay.clk.eq(clk200_bufg)
        self.idelayctrl = S7IDELAYCTRL(self.cd_idelay)

        # Constraints
        platform.add_false_path_constraints(self.cd_sys.clk,       self.pll0.clkin)
        platform.add_false_path_constraints(self.cd_uc.clk,        self.pll1.clkin)
        if need_ddr_clks:
            platform.add_false_path_constraints(self.cd_ub_4x.clk,     self.pll0.clkin)
            platform.add_false_path_constraints(self.cd_ub_4x_dqs.clk, self.pll0.clkin)


# =============================================================================
#                     Classic -> Pipelined Wishbone bridge
# =============================================================================
class WBC2PipelineBridge(LiteXModule):
    """
    Bridge from classic Wishbone (LiteX side) to pipelined Wishbone
    (as used by UberDDR3) via the wbc2pipeline.v module.

    - 'self.s' is a classic WB slave interface.
    - 'm_*' is the pipelined WB master side (raw signals).
    """
    def __init__(self, data_width=128, adr_width=27, clock_domain="sys"):
        # Classic WB slave interface (CPU/LiteX bus side)
        self.s = wb.Interface(data_width=data_width, adr_width=adr_width)

        # Pipelined WB master signals
        self.m_cyc   = Signal()
        self.m_stb   = Signal()
        self.m_we    = Signal()
        self.m_adr   = Signal(adr_width)
        self.m_dat_w = Signal(data_width)
        self.m_sel   = Signal(data_width//8)
        self.m_stall = Signal()
        self.m_ack   = Signal()
        self.m_dat_r = Signal(data_width)
        self.m_err   = Signal()

        # Handle optional cti/bte on the classic side
        try:
            s_cti, s_bte = self.s.cti, self.s.bte
        except AttributeError:
            s_cti, s_bte = Signal(3, reset=0), Signal(2, reset=0)

        self.specials += Instance("wbc2pipeline",
            p_AW = len(self.s.adr),
            p_DW = len(self.s.dat_w),

            i_i_clk   = ClockSignal(clock_domain),
            i_i_reset = ResetSignal(clock_domain),

            # Upstream classic WB
            i_i_scyc  = self.s.cyc,
            i_i_sstb  = self.s.stb,
            i_i_swe   = self.s.we,
            i_i_saddr = self.s.adr,
            i_i_sdata = self.s.dat_w,
            i_i_ssel  = self.s.sel,
            i_i_scti  = s_cti,
            i_i_sbte  = s_bte,
            o_o_sack  = self.s.ack,
            o_o_sdata = self.s.dat_r,
            o_o_serr  = self.s.err,

            # Downstream pipelined WB (raw signals)
            o_o_mcyc   = self.m_cyc,
            o_o_mstb   = self.m_stb,
            o_o_mwe    = self.m_we,
            o_o_maddr  = self.m_adr,
            o_o_mdata  = self.m_dat_w,
            o_o_msel   = self.m_sel,
            i_i_mstall = self.m_stall,
            i_i_mack   = self.m_ack,
            i_i_mdata  = self.m_dat_r,
            i_i_merr   = self.m_err
        )


# =============================================================================
#                         CSR snapshot/commit (sys->uc)
# =============================================================================
class CSRConfigAFIFO(LiteXModule):
    """
    Capture a snapshot of multiple CSR fields in the sys domain and forward
    them atomically to the uc domain via an AsyncFIFO.

    Software writes to `commit` (write strobe) to enqueue one config frame.
    """
    def __init__(self, fields, cd_from="sys", cd_to="uc", fifo_depth=4):
        # Software writes to commit to snapshot & enqueue config frame.
        self.commit   = CSRStorage(1, description="Write (strobe) to snapshot & enqueue config frame.")
        # Sticky overflow flag if FIFO is full on commit
        self.overflow = CSRStatus(1, description="Sticky: commit attempted while FIFO full.")
        # FIFO flags (bit0=readable, bit1=writable)
        self.level    = CSRStatus(8, description="FIFO flags: bit0=readable, bit1=writable.")

        total_w  = sum(len(sig) for sig in fields.values())
        flat_sys = Signal(total_w)  # packed data in sys domain
        flat_uc  = Signal(total_w)  # packed data in uc domain (latched per frame)

        # Pack sys fields into flat_sys (combinational)
        offs = 0
        for _, sig in fields.items():
            w = len(sig)
            self.comb += flat_sys[offs:offs+w].eq(sig)
            offs += w

        # Async FIFO bridging cd_from -> cd_to
        fifo = AsyncFIFO(width=total_w, depth=fifo_depth)
        self.submodules.fifo = ClockDomainsRenamer({"write": cd_from, "read": cd_to})(fifo)

        # Use CSR write strobe as the "commit" pulse (in sys domain)
        commit_pulse_sys = Signal()
        self.comb += commit_pulse_sys.eq(self.commit.re)

        # Write-side (sys): enqueue if writable
        self.comb += [
            fifo.din.eq(flat_sys),
            fifo.we.eq(commit_pulse_sys & fifo.writable),
        ]
        self.sync.sys += If(commit_pulse_sys & ~fifo.writable,
            self.overflow.status.eq(1)
        )

        # Status bits (safe as CSRStatus is sampled in sys)
        self.comb += self.level.status.eq(Cat(fifo.readable, fifo.writable, C(0, 6)))

        # Read-side (uc): pop ONE frame and pulse cfg_update_uc for 1 cycle
        self.cfg_update_uc = Signal()

        pop_uc = Signal()
        self.comb += pop_uc.eq(fifo.readable)  # pop whenever a frame is present (one per cycle)

        # IMPORTANT: make pop a 1-cycle pulse; AsyncFIFO's re is sampled in read domain.
        self.comb += fifo.re.eq(pop_uc)
        self.sync.uc += [
            self.cfg_update_uc.eq(0),
            If(pop_uc,
                flat_uc.eq(fifo.dout),
                self.cfg_update_uc.eq(1),
            )
        ]

        # Unpack into uc-domain outputs, updated atomically on cfg_update_uc
        offs = 0
        for name, sig in fields.items():
            w = len(sig)
            out = Signal(w, name=f"out_{name}_uc")
            setattr(self, f"out_{name}_uc", out)
            self.sync.uc += If(self.cfg_update_uc,
                out.eq(flat_uc[offs:offs+w])
            )
            offs += w


# =============================================================================
#                    Wishbone crossbar 2×1 wrapper (wbxbar)
# =============================================================================
class WbXbar2x1(LiteXModule):
    """
    Wrapper around Verilog wbxbar for:
      - NM = 2 masters (M0 = CPU path, M1 = DMA)
      - NS = 1 slave   (S0 = DDR3)
      - Pipelined Wishbone (raw signals)

    Packs/unpacks the busses into the expected vector format.
    """
    def __init__(self, AW, DW, clock_domain="sys"):
        assert DW % 8 == 0

        # -------- Master[0] (CPU path via c2p) --------
        self.m0_cyc   = Signal()
        self.m0_stb   = Signal()
        self.m0_we    = Signal()
        self.m0_adr   = Signal(AW)
        self.m0_dat_w = Signal(DW)
        self.m0_sel   = Signal(DW//8)
        self.m0_stall = Signal()
        self.m0_ack   = Signal()
        self.m0_dat_r = Signal(DW)
        self.m0_err   = Signal()

        # -------- Master[1] (zipdma_s2mm) -------------
        self.m1_cyc   = Signal()
        self.m1_stb   = Signal()
        self.m1_we    = Signal()
        self.m1_adr   = Signal(AW)
        self.m1_dat_w = Signal(DW)
        self.m1_sel   = Signal(DW//8)
        self.m1_stall = Signal()
        self.m1_ack   = Signal()
        self.m1_dat_r = Signal(DW)
        self.m1_err   = Signal()

        # -------- Slave[0] (DDR) ---------------
        self.s_cyc    = Signal()
        self.s_stb    = Signal()
        self.s_we     = Signal()
        self.s_adr    = Signal(AW)
        self.s_dat_w  = Signal(DW)
        self.s_sel    = Signal(DW//8)
        self.s_stall  = Signal()
        self.s_ack    = Signal()
        self.s_dat_r  = Signal(DW)
        self.s_err    = Signal()

        self.specials += Instance("wbxbar",
            p_NM = 2,
            p_NS = 1,
            p_AW = AW,
            p_DW = DW,

            p_SLAVE_MASK             = 0,
            p_LGMAXBURST             = 6,
            p_OPT_TIMEOUT            = 0,
            p_OPT_STARVATION_TIMEOUT = 0,
            p_OPT_DBLBUFFER          = 0,
            p_OPT_LOWPOWER           = 1,

            i_i_clk   = ClockSignal(clock_domain),
            i_i_reset = ResetSignal(clock_domain),

            # Masters packed as [M0, M1]
            i_i_mcyc  = Cat(self.m0_cyc, self.m1_cyc),
            i_i_mstb  = Cat(self.m0_stb, self.m1_stb),
            i_i_mwe   = Cat(self.m0_we,  self.m1_we),
            i_i_maddr = Cat(self.m0_adr, self.m1_adr),
            i_i_mdata = Cat(self.m0_dat_w, self.m1_dat_w),
            i_i_msel  = Cat(self.m0_sel,   self.m1_sel),

            o_o_mstall = Cat(self.m0_stall, self.m1_stall),
            o_o_mack   = Cat(self.m0_ack,   self.m1_ack),
            o_o_mdata  = Cat(self.m0_dat_r, self.m1_dat_r),
            o_o_merr   = Cat(self.m0_err,   self.m1_err),

            # Single slave packed (NS=1)
            o_o_scyc  = Cat(self.s_cyc),
            o_o_sstb  = Cat(self.s_stb),
            o_o_swe   = Cat(self.s_we),
            o_o_saddr = self.s_adr,
            o_o_sdata = self.s_dat_w,
            o_o_ssel  = self.s_sel,

            i_i_sstall= Cat(self.s_stall),
            i_i_sack  = Cat(self.s_ack),
            i_i_sdata = self.s_dat_r,
            i_i_serr  = Cat(self.s_err),
        )


# =============================================================================
#                               RampSource
# =============================================================================
class RampSource(LiteXModule):
    """
    Ramp generator with runtime-programmable length.

    MAX_BEATS is the maximum supported beats (compile-time sizing).
    length_i is the requested beats (1..MAX_BEATS).
    """
    def __init__(self, dw=256, max_beats=1<<23):
        assert dw % 16 == 0
        lanes = dw // 16

        self.start    = Signal()
        self.valid    = Signal()
        self.ready    = Signal()
        self.data     = Signal(dw)
        self.bytes    = Signal(max=dw//8 + 1)
        self.last     = Signal()

        # NEW: runtime-configurable length (beats)
        self.length_i = Signal(32, reset=256)

        running   = Signal()
        base_step = Signal(16)
        idx       = Signal(max=max_beats)

        # clamp length to [1..max_beats]
        length_clamped = Signal.like(idx)
        self.comb += [
            If(self.length_i == 0,
                length_clamped.eq(1)
            ).Elif(self.length_i >= max_beats,
                length_clamped.eq(max_beats)
            ).Else(
                length_clamped.eq(self.length_i[:len(idx)])
            )
        ]

        self.sync += [
            If(self.start,
                running.eq(1),
                idx.eq(0),
                base_step.eq(0),
            ).Elif(running & self.ready,
                base_step.eq(base_step + lanes),
                If(idx == (length_clamped - 1),
                    running.eq(0)
                ).Else(
                    idx.eq(idx + 1)
                )
            )
        ]

        self.comb += [
            self.valid.eq(running),
            self.bytes.eq(dw // 8),
            self.last.eq(running & (idx == (length_clamped - 1))),
        ]

        for i in range(lanes):
            self.comb += self.data[16*i:16*(i+1)].eq(base_step + i)

# =============================================================================
#                SampleStream: external design samples -> bus stream
# =============================================================================
class SampleStream(LiteXModule):
    """
    UC-domain sample packer: takes a 12-bit sample every uc clock and packs into DW-bit beats.

    Notes:
      - If downstream stalls long enough, internal FIFO can overflow; an overflow flag is set and samples are dropped.
      - Beats are produced when enough samples are available (DW/16 samples per beat).
    """
    def __init__(self, sample_width=12, dw=256, sample_fifo_depth=1024):
        assert dw % 16 == 0
        lanes = dw // 16

        self.sample_in = Signal(sample_width)

        self.start = Signal()
        self.valid = Signal()
        self.ready = Signal()
        self.data  = Signal(dw)
        self.bytes = Signal(max=dw//8 + 1)
        self.last  = Signal()

        self.beats = Signal(32)

        self.overflow = Signal()  # sticky in uc domain (can be exported if you want)

        # Sign-extend to 16-bit
        sample16 = Signal(16)
        self.comb += sample16.eq(
            Cat(self.sample_in,
                Replicate(self.sample_in[sample_width-1], 16 - sample_width))
        )
        self.comb += self.bytes.eq(dw // 8)

        # FIFO of 16-bit samples to decouple sampling from beat emission
        sf = SyncFIFO(width=16, depth=sample_fifo_depth)
        self.submodules.sf = sf

        running   = Signal()
        beat_cnt  = Signal(32)

        # Beat buffer and control
        buf      = Array(Signal(16) for _ in range(lanes))
        fill_idx = Signal(max=lanes)
        have_beat = Signal()

        # Pack buffer into output data
        for i in range(lanes):
            self.comb += self.data[16*i:16*(i+1)].eq(buf[i])

        # Sampling: push every cycle while running
        self.comb += [
            sf.din.eq(sample16),
            sf.we.eq(running & sf.writable),
        ]
        self.sync += If(running & ~sf.writable,
            self.overflow.eq(1)  # sticky flag; samples dropped
        )

        # Beat build: read lanes samples from sf into buf, then present a beat until accepted
        self.comb += sf.re.eq(running & ~have_beat & sf.readable)

        self.sync += [
            If(self.start,
                running.eq(1),
                beat_cnt.eq(0),
                fill_idx.eq(0),
                have_beat.eq(0),
                self.overflow.eq(0),
            ).Elif(running,
                # Fill beat buffer while we don't yet have a full beat to present
                If(~have_beat,
                    If(sf.readable,
                        buf[fill_idx].eq(sf.dout),
                        If(fill_idx == (lanes - 1),
                            fill_idx.eq(0),
                            have_beat.eq(1),
                        ).Else(
                            fill_idx.eq(fill_idx + 1),
                        )
                    )
                ).Else(
                    # We have a beat; wait for downstream accept
                    If(self.valid & self.ready,
                        have_beat.eq(0),
                        If(beat_cnt == (self.beats - 1),
                            running.eq(0),
                            beat_cnt.eq(0),
                        ).Else(
                            beat_cnt.eq(beat_cnt + 1),
                        )
                    )
                )
            )
        ]

        self.comb += [
            self.valid.eq(have_beat),
            self.last.eq(running & have_beat & (beat_cnt == (self.beats - 1))),
        ]


# =============================================================================
#                UCStreamSource: mux between ramp and external stream
# =============================================================================
class UCStreamSource(LiteXModule):
    def __init__(self, dw=256, max_beats=1<<23):
        self.start        = Signal()
        self.use_external = Signal()

        # NEW: runtime beat length for ramp
        self.length_i     = Signal(32, reset=256)

        self.valid = Signal()
        self.ready = Signal()
        self.data  = Signal(dw)
        self.bytes = Signal(max=dw//8 + 1)
        self.last  = Signal()

        self.ext_valid = Signal()
        self.ext_ready = Signal()
        self.ext_data  = Signal(dw)
        self.ext_bytes = Signal(max=dw//8 + 1)
        self.ext_last  = Signal()

        self.submodules.ramp = RampSource(dw=dw, max_beats=max_beats)

        # NEW: wire requested length into ramp
        self.comb += self.ramp.length_i.eq(self.length_i)

        self.comb += self.ramp.start.eq(self.start & ~self.use_external)

        self.comb += [
            self.valid.eq(Mux(self.use_external, self.ext_valid, self.ramp.valid)),
            self.data .eq(Mux(self.use_external, self.ext_data,  self.ramp.data)),
            self.bytes.eq(Mux(self.use_external, self.ext_bytes, self.ramp.bytes)),
            self.last .eq(Mux(self.use_external, self.ext_last,  self.ramp.last)),
        ]

        self.comb += [
            self.ext_ready.eq(self.ready & self.use_external),
            self.ramp.ready.eq(self.ready & ~self.use_external),
        ]



# =============================================================================
#                               UberDDR3 wrapper
# =============================================================================
class UberDDR3(LiteXModule):
    """
    Wrapper around the UberDDR3 Verilog DDR3 controller.

    CPU side:
      - 32-bit classic Wishbone interface (self.wb) for normal memory access.
      - Converted to wide (ub_dw) via wb.Converter.
      - Then passed through WBC2PipelineBridge to pipelined WB signals.
      - Then goes into WbXbar2x1 (master 0).

    DMA side:
      - zipdma_s2mm instance is master 1 on the same crossbar.
      - It writes a UC-domain streaming source into DDR3 via CDC FIFO.

    UC-domain capture control comes from SoC:
      - self.cap_enable_uc (1=capture external, 0=ramp)
      - self.cap_beats_uc  (# of beats)
    """
    def __init__(self, platform, pads, locked,
                 sys_clk_hz=100e6, ddr_ck_hz=400e6,
                 row_bits=15, col_bits=10, ba_bits=3,
                 byte_lanes=4, dual_rank=0, speed_bin=3,
                 sdram_capacity=5, dll_off=0, odelay_supported=0, bist_mode=0):

        ub_dw = 64 * byte_lanes
        serdes_ratio = 4

        wb_addr_bits = (
            row_bits + col_bits + ba_bits
            - int(math.log2(serdes_ratio * 2))
            + dual_rank
        )

        # ------------------------------------------------------------------
        # Classic CPU WB -> wide WB (ub_dw) via Converter
        # ------------------------------------------------------------------
        self.wb = wb.Interface(data_width=32)
        wb_wide = wb.Interface(data_width=ub_dw)
        self.submodules.wb_up = wb.Converter(self.wb, wb_wide)

        # ------------------------------------------------------------------
        # Classic -> pipelined bridge
        # ------------------------------------------------------------------
        self.submodules.c2p = WBC2PipelineBridge(
            data_width   = wb_wide.data_width,
            adr_width    = wb_wide.adr_width,
            clock_domain ="sys"
        )
        self.comb += wb_wide.connect(self.c2p.s)

        AW = len(self.c2p.m_adr)
        DW = ub_dw

        self.submodules.xbar = WbXbar2x1(AW=AW, DW=DW, clock_domain="sys")

        # M0 = CPU path (pipelined master from c2p)
        self.comb += [
            self.xbar.m0_cyc   .eq(self.c2p.m_cyc),
            self.xbar.m0_stb   .eq(self.c2p.m_stb),
            self.xbar.m0_we    .eq(self.c2p.m_we),
            self.xbar.m0_adr   .eq(self.c2p.m_adr),
            self.xbar.m0_dat_w .eq(self.c2p.m_dat_w),
            self.xbar.m0_sel   .eq(self.c2p.m_sel),

            self.c2p.m_stall   .eq(self.xbar.m0_stall),
            self.c2p.m_ack     .eq(self.xbar.m0_ack),
            self.c2p.m_dat_r   .eq(self.xbar.m0_dat_r),
            self.c2p.m_err     .eq(self.xbar.m0_err),
        ]

        # ------------------------------------------------------------------
        # zipdma_s2mm on Master[1]
        # ------------------------------------------------------------------
        WBLSB = int(math.log2(DW//8))
        ADDR_WIDTH_FOR_DMA = AW + WBLSB

        # UC-domain capture interface (wired from SoC)
        self.cap_sample    = Signal(12)
        self.cap_enable_uc = Signal()
        self.cap_beats_uc  = Signal(32, reset=256)

        # Exposed stream into DMA (sys domain side of CDC bridge)
        self.s_valid = Signal()
        self.s_ready = Signal()
        self.s_data  = Signal(DW)
        self.s_bytes = Signal(max=DW//8+1)
        self.s_last  = Signal()

        # DMA control / status CSRs
        self.dma_req   = CSRStorage(1, description="Write (strobe) to start S2MM (uses addr/size/inc).")
        self.dma_busy  = CSRStatus(1)
        self.dma_err   = CSRStatus(1)
        self.dma_inc   = CSRStorage(1, reset=1)
        self.dma_size  = CSRStorage(2, reset=0, description="00=bus,01=32b,10=16b,11=byte")
        self.dma_addr0 = CSRStorage(32)
        self.dma_addr1 = CSRStorage(32)

        # Use CSR write strobe for request pulse
        dma_req_pulse = Signal()
        self.comb += dma_req_pulse.eq(self.dma_req.re)

        dma_addr = Signal(ADDR_WIDTH_FOR_DMA)
        self.comb += dma_addr.eq(
            Cat(self.dma_addr0.storage, self.dma_addr1.storage)[:ADDR_WIDTH_FOR_DMA]
        )

        self.specials += Instance("zipdma_s2mm",
            p_ADDRESS_WIDTH      = ADDR_WIDTH_FOR_DMA,
            p_BUS_WIDTH          = DW,
            p_OPT_LITTLE_ENDIAN  = 1,
            p_LGPIPE             = 10,

            i_i_clk              = ClockSignal("sys"),
            i_i_reset            = ResetSignal("sys"),

            i_i_request          = dma_req_pulse,
            o_o_busy             = self.dma_busy.status,
            o_o_err              = self.dma_err.status,
            i_i_inc              = self.dma_inc.storage[0],
            i_i_size             = self.dma_size.storage,
            i_i_addr             = dma_addr,

            # External stream (connect to CDC consumer)
            i_S_VALID            = self.s_valid,
            o_S_READY            = self.s_ready,
            i_S_DATA             = self.s_data,
            i_S_BYTES            = self.s_bytes,
            i_S_LAST             = self.s_last,

            # Write port to DDR via xbar master[1]
            o_o_wr_cyc           = self.xbar.m1_cyc,
            o_o_wr_stb           = self.xbar.m1_stb,
            o_o_wr_we            = self.xbar.m1_we,
            o_o_wr_addr          = self.xbar.m1_adr,
            o_o_wr_data          = self.xbar.m1_dat_w,
            o_o_wr_sel           = self.xbar.m1_sel,
            i_i_wr_stall         = self.xbar.m1_stall,
            i_i_wr_ack           = self.xbar.m1_ack,
            i_i_wr_data          = self.xbar.m1_dat_r,
            i_i_wr_err           = self.xbar.m1_err
        )

        # ------------------------------------------------------------------
        # UC-domain stream source + CDC into S2MM (sys domain)
        # ------------------------------------------------------------------
        self.submodules.uc_src = ClockDomainsRenamer("uc")(
            UCStreamSource(dw=DW, max_beats=1<<23)   # pick a max you like
        )


        self.submodules.cap_stream = ClockDomainsRenamer("uc")(
            SampleStream(sample_width=12, dw=DW, sample_fifo_depth=2048)
        )

        # sys -> uc pulse: start capture when DMA request occurs
        self.submodules.ps_start = PulseSynchronizer("sys", "uc")
        self.comb += self.ps_start.i.eq(dma_req_pulse)

        self.comb += [
            self.uc_src.use_external.eq(self.cap_enable_uc),
            self.uc_src.length_i.eq(self.cap_beats_uc),
            # start ramp when request and not capturing external
            self.uc_src.start.eq(self.ps_start.o & ~self.cap_enable_uc),

            # start external capture stream when request and cap_enable_uc=1
            self.cap_stream.start.eq(self.ps_start.o & self.cap_enable_uc),
        ]

        self.comb += [
            self.cap_stream.sample_in.eq(self.cap_sample),
            self.cap_stream.beats.eq(self.cap_beats_uc),
        ]

        self.comb += [
            self.uc_src.ext_valid.eq(self.cap_stream.valid),
            self.uc_src.ext_data.eq(self.cap_stream.data),
            self.uc_src.ext_bytes.eq(self.cap_stream.bytes),
            self.uc_src.ext_last.eq(self.cap_stream.last),
            self.cap_stream.ready.eq(self.uc_src.ext_ready),
        ]

        # Async FIFO: write@uc, read@sys, carries {data, bytes, last}
        bytes_width = len(self.s_bytes)
        fifo_width  = DW + bytes_width + 1

        fifo = AsyncFIFO(width=fifo_width, depth=16)
        self.submodules.s2mm_fifo = ClockDomainsRenamer(
            {"write": "uc", "read": "sys"}
        )(fifo)

        self.comb += [
            fifo.din.eq(Cat(self.uc_src.data, self.uc_src.bytes, self.uc_src.last)),
            fifo.we.eq(self.uc_src.valid & fifo.writable),
            self.uc_src.ready.eq(fifo.writable),
        ]

        data_sys   = Signal(DW)
        bytes_sys  = Signal(bytes_width)
        last_sys   = Signal()

        self.comb += [
            Cat(data_sys, bytes_sys, last_sys).eq(fifo.dout),

            self.s_valid.eq(fifo.readable),
            self.s_data.eq(data_sys),
            self.s_bytes.eq(bytes_sys),
            self.s_last.eq(last_sys),

            fifo.re.eq(self.s_ready & fifo.readable),
        ]

        # ------------------------------------------------------------------
        # DDR3 top as slave on the xbar
        # ------------------------------------------------------------------
        self.calib_done = CSRStatus(1)

        self.specials += Instance("ddr3_top",
            p_CONTROLLER_CLK_PERIOD = int(round(1e12/float(sys_clk_hz))),
            p_DDR3_CLK_PERIOD       = int(round(1e12/float(ddr_ck_hz))),
            p_ROW_BITS              = row_bits,
            p_COL_BITS              = col_bits,
            p_BA_BITS               = ba_bits,
            p_BYTE_LANES            = byte_lanes,
            p_DUAL_RANK_DIMM        = dual_rank,
            p_SPEED_BIN             = speed_bin,
            p_SDRAM_CAPACITY        = sdram_capacity,
            p_DLL_OFF               = dll_off,
            p_ODELAY_SUPPORTED      = odelay_supported,
            p_BIST_MODE             = bist_mode,
            p_DIC                   = 0b01,
            p_RTT_NOM               = 0b001,
            p_AUX_WIDTH             = 4,

            i_i_controller_clk      = ClockSignal("sys"),
            i_i_ddr3_clk            = ClockSignal("ub_4x"),
            i_i_ddr3_clk_90         = ClockSignal("ub_4x_dqs"),
            i_i_ref_clk             = ClockSignal("idelay"),
            i_i_rst_n               = locked & ~ResetSignal("sys"),

            # Wishbone interface (pipelined) from xbar slave port
            i_i_wb_cyc              = self.xbar.s_cyc,
            i_i_wb_stb              = self.xbar.s_stb,
            i_i_wb_we               = self.xbar.s_we,
            i_i_wb_addr             = self.xbar.s_adr[:wb_addr_bits],
            i_i_wb_data             = self.xbar.s_dat_w,
            i_i_wb_sel              = self.xbar.s_sel,
            o_o_wb_stall            = self.xbar.s_stall,
            o_o_wb_ack              = self.xbar.s_ack,
            o_o_wb_err              = self.xbar.s_err,
            o_o_wb_data             = self.xbar.s_dat_r,

            i_i_aux                 = Cat(self.xbar.s_we, C(0, 3)),
            o_o_aux                 = Open(4),

            # Second WB port disabled
            i_i_wb2_cyc             = 0,
            i_i_wb2_stb             = 0,
            i_i_wb2_we              = 0,
            i_i_wb2_addr            = 0,
            i_i_wb2_data            = 0,
            i_i_wb2_sel             = 0,
            o_o_wb2_stall           = Open(),
            o_o_wb2_ack             = Open(),
            o_o_wb2_data            = Open(),

            # Physical DDR3 pins
            o_o_ddr3_clk_p          = pads.clk_p,
            o_o_ddr3_clk_n          = pads.clk_n,
            o_o_ddr3_reset_n        = pads.reset_n,
            o_o_ddr3_cke            = pads.cke,
            o_o_ddr3_cs_n           = pads.cs_n,
            o_o_ddr3_ras_n          = pads.ras_n,
            o_o_ddr3_cas_n          = pads.cas_n,
            o_o_ddr3_we_n           = pads.we_n,
            o_o_ddr3_addr           = pads.a,
            o_o_ddr3_ba_addr        = pads.ba,
            io_io_ddr3_dq           = pads.dq,
            io_io_ddr3_dqs          = pads.dqs_p,
            io_io_ddr3_dqs_n        = pads.dqs_n,
            o_o_ddr3_dm             = pads.dm,
            o_o_ddr3_odt            = pads.odt,

            o_o_calib_complete      = self.calib_done.status,
            o_o_debug1              = Open(),
            i_i_user_self_refresh   = 0,
            o_uart_tx               = Open()
        )

        # ------------------------------------------------------------------
        # Add required Verilog sources and XDC/Vivado commands
        # ------------------------------------------------------------------
        platform.add_source(f"{verilog_dir}/memory/ddr3_top.v")
        platform.add_source(f"{verilog_dir}/memory/ddr3_controller.v")
        platform.add_source(f"{verilog_dir}/memory/ddr3_phy.v")
        platform.add_source(f"{verilog_dir}/memory/wbc2pipeline.v")
        platform.add_source(f"{verilog_dir}/memory/wbxbar.v")
        platform.add_source(f"{verilog_dir}/memory/skidbuffer.v")
        platform.add_source(f"{verilog_dir}/memory/addrdecode.v")
        platform.add_source(f"{verilog_dir}/memory/zipdma_s2mm.v")

        platform.add_platform_command(
            "set_property INTERNAL_VREF 0.75 "
            "[get_iobanks -of_objects [get_ports {{ddram_dq[*] ddram_dqs_p[*] ddram_dqs_n[*]}}]]"
        )
        platform.add_platform_command(
            "set_property BITSTREAM.STARTUP.MATCH_CYCLE 6 [current_design]"
        )


# =============================================================================
# Main CSRs (UberClock configuration registers)
# =============================================================================
class MainCSRs(LiteXModule):
    def __init__(self):
        # Match PW=24 in Verilog
        self.phase_inc_nco       = CSRStorage(24)
        self.phase_inc_down_1    = CSRStorage(24)
        self.phase_inc_down_2    = CSRStorage(24)
        self.phase_inc_down_3    = CSRStorage(24)
        self.phase_inc_down_4    = CSRStorage(24)
        self.phase_inc_down_5    = CSRStorage(24)
        self.phase_inc_down_ref  = CSRStorage(24)

        # CPU NCOs (5 channels) in uberclock.v
        self.phase_inc_cpu1      = CSRStorage(24)
        self.phase_inc_cpu2      = CSRStorage(24)
        self.phase_inc_cpu3      = CSRStorage(24)
        self.phase_inc_cpu4      = CSRStorage(24)
        self.phase_inc_cpu5      = CSRStorage(24)

        # Magnitudes (signed in Verilog, stored as 2's complement here)
        self.nco_mag             = CSRStorage(12, reset=0)
        self.mag_cpu1            = CSRStorage(12, reset=0)
        self.mag_cpu2            = CSRStorage(12, reset=0)
        self.mag_cpu3            = CSRStorage(12, reset=0)
        self.mag_cpu4            = CSRStorage(12, reset=0)
        self.mag_cpu5            = CSRStorage(12, reset=0)

        self.input_select        = CSRStorage(2)
        self.upsampler_input_mux = CSRStorage(2)

        # Must be 4 bits to match your Verilog mux (0..12)
        self.output_select_ch1   = CSRStorage(4)
        self.output_select_ch2   = CSRStorage(4)

        # Debug selects (3-bit) in uberclock.v
        self.lowspeed_dbg_select = CSRStorage(3)
        self.highspeed_dbg_select= CSRStorage(3)

        self.gain1               = CSRStorage(32)
        self.gain2               = CSRStorage(32)
        self.gain3               = CSRStorage(32)
        self.gain4               = CSRStorage(32)
        self.gain5               = CSRStorage(32)

        self.upsampler_input_x   = CSRStorage(16)
        self.upsampler_input_y   = CSRStorage(16)
        self.final_shift         = CSRStorage(3)

        # DDR capture control (already present)
        self.cap_enable          = CSRStorage(1, description="1=DDR capture from design, 0=internal ramp.")
        self.cap_beats           = CSRStorage(32, reset=256, description="Number of 256-bit beats to capture into DDR.")

        # Optional: low-speed capture RAM inside uberclock.v (cap_arm/cap_idx/cap_done/cap_data)
        self.cap_arm             = CSRStorage(1, description="Pulse to start low-speed capture RAM.")
        self.cap_idx             = CSRStorage(16, description="Index for low-speed capture RAM readback.")
        self.cap_done            = CSRStatus(1, description="Low-speed capture done.")
        self.cap_data            = CSRStatus(16, description="Low-speed capture sample at cap_idx.")

# =============================================================================
# SoC
# =============================================================================
class BaseSoC(SoCCore):
    def __init__(self, toolchain="vivado",
                 with_hdmi=False, with_ethernet=False, with_etherbone=False,
                 with_spi_flash=False, with_led_chaser=False,
                 with_sdcard=False, with_spi_sdcard=False, with_pcie=False,
                 with_video_terminal=False, with_video_framebuffer=False, with_video_colorbars=False,
                 with_ledmem=False, with_uberclock=False, with_uberddr3=False,
                 **kwargs):

        kwargs.setdefault("integrated_main_ram_size", 64*1024)
        kwargs["uart_name"] = "serial"

        platform = alinx_ax7203.Platform(toolchain=toolchain)

        need_ddr_clks = with_uberddr3 or (kwargs.get("integrated_main_ram_size", 0) == 0)
        self.submodules.crg = _CRG(platform, need_ddr_clks=need_ddr_clks)

        SoCCore.__init__(self, platform, 100e6,
            ident="AX7203 UberClock65 UberDDR3 with S2MM via wbxbar",
            **kwargs)

        # ------------------------------------------------------------------
        # Heartbeat LED (LED0)
        # ------------------------------------------------------------------
        hb = Signal(24)
        self.sync.sys += hb.eq(hb + 1)
        leds = Cat(*platform.request_all("user_led"))
        self.comb += leds[0].eq(hb[23])

        # Timer1
        self.submodules.timer1 = Timer()
        self.add_csr("timer1")

        # ------------------------------------------------------------------
        # LiteDRAM path (standard SoC DRAM)
        # ------------------------------------------------------------------
        if (not self.integrated_main_ram_size) and (not with_uberddr3):
            self.ddrphy = s7ddrphy.A7DDRPHY(
                platform.request("ddram"),
                memtype="DDR3", nphases=4, sys_clk_freq=100e6
            )
            cs = ControllerSettings()
            cs.auto_precharge = False
            self.add_sdram("sdram",
                phy=self.ddrphy,
                module=MT41J256M16(100e6, "1:4"),
                size=0x40000000,
                controller_settings=cs,
                origin=self.mem_map["main_ram"],
                l2_cache_size=kwargs.get("l2_size", 8192)
            )

        # ------------------------------------------------------------------
        # UberDDR3 side-memory
        # ------------------------------------------------------------------
        if with_uberddr3:
            pads = platform.request("ddram")
            self.submodules.ubddr3 = UberDDR3(
                platform   = platform,
                pads       = pads,
                locked     = self.crg.pll0.locked,
                sys_clk_hz = 100e6,
                ddr_ck_hz  = 400e6,
                row_bits   = 15,
                col_bits   = 10,
                ba_bits    = 3,
                byte_lanes = 4,
                dual_rank  = 0,
                speed_bin  = 3,
                sdram_capacity=5,
                dll_off    = 0,
                odelay_supported=0,
                bist_mode  = 0
            )

            ub_base = 0xA000_0000
            ub_size = 0x1000_0000
            region  = SoCRegion(origin=ub_base, size=ub_size, cached=False, linker=False)

            self.bus.add_slave("ub_ram", self.ubddr3.wb, region)

            self.add_constant("UBDDR3_MEM_BASE", ub_base)
            self.add_csr("ubddr3")

            self.comb += leds[1].eq(self.ubddr3.calib_done.status)

        # ------------------------------------------------------------------
        # Ethernet / Etherbone
        # ------------------------------------------------------------------
        if with_ethernet or with_etherbone:
            self.ethphy = LiteEthPHYRGMII(
                clock_pads=platform.request("eth_clocks"),
                pads      =platform.request("eth")
            )
            if with_ethernet:
                self.add_ethernet(phy=self.ethphy)
            if with_etherbone:
                self.add_etherbone(
                    phy=self.ethphy,
                    ip_address="192.168.0.123",
                    mac_address=0x0200000000AB
                )

        # ------------------------------------------------------------------
        # SPI Flash
        # ------------------------------------------------------------------
        if with_spi_flash:
            from litespi.modules import N25Q128
            from litespi.opcodes import SpiNorFlashOpCodes as Codes
            self.add_spi_flash(
                mode        ="4x",
                module      =N25Q128(Codes.READ_1_1_1),
                rate        ="1:2",
                with_master =True
            )

        # ------------------------------------------------------------------
        # HDMI output
        # ------------------------------------------------------------------
        if with_hdmi and (with_video_colorbars or with_video_framebuffer or with_video_terminal):
            self.videophy = VideoS7HDMIPHY(platform.request("hdmi_out"), clock_domain="hdmi")

            if with_video_colorbars:
                self.add_video_colorbars(self.videophy,
                    timings="640x480@60Hz", clock_domain="hdmi")

            if with_video_terminal:
                self.add_video_terminal(self.videophy,
                    timings="640x480@60Hz", clock_domain="hdmi")

            if with_video_framebuffer:
                self.add_video_framebuffer(self.videophy,
                    timings="640x480@60Hz", clock_domain="hdmi")

        # ------------------------------------------------------------------
        # LED chaser
        # ------------------------------------------------------------------
        if with_led_chaser:
            self.leds = LedChaser(
                pads=platform.request_all("user_led"),
                sys_clk_freq=100e6
            )

        # ------------------------------------------------------------------
        # UberClock DSP integration
        # ------------------------------------------------------------------
        if with_uberclock:
            self._add_uberclock_fullrate(verilog_dir, leds)

    def _add_uberclock_fullrate(self, verilog_dir, leds):
        # Add required Verilog sources for UberClock pipeline
        for fn in [
            "adc/adc.v", "dac/dac.v",
            "filters/cic.v", "filters/cic_comp_down_mac.v",
            "filters/comp_down_coeffs.mem",
            "filters/hb_down_mac.v","filters/hb_down_coeffs.mem",
            "filters/downsamplerFilter.v",
            "filters/upsamplerFilter.v","filters/hb_up_mac.v",
            "filters/coeffs.mem","filters/cic_comp_up_mac.v",
            "filters/coeffs_comp.mem","filters/cic_int.v",
            "uberclock/uberclock.v",
            "uberclock/rx_channel.v",
            "uberclock/tx_channel.v",
            "to_polar/to_polar.v",
            "cordic/cordic_pre_rotate.v","cordic/cordic_pipeline_stage.v",
            "cordic/cordic_round.v","cordic/cordic.v",
            "cordic/cordic_logic.v","cordic/gain_and_saturate.v",
            "cordic16/cordic16.v","cordic16/cordic_pre_rotate_16.v",
        ]:
            self.platform.add_source(f"{verilog_dir}/{fn}")

        # CSRs for UberClock configuration
        self.submodules.main = MainCSRs()
        self.add_csr("main")

        # EventManager: single event 'ce_down'
        self.submodules.evm = EventManager()
        self.evm.ce_down = EventSourcePulse(description="Downsample ready")
        self.evm.finalize()
        self.irq.add("evm")
        self.add_csr("evm")

        m = self.main
        cfg_sys = {
            "input_select":         m.input_select.storage,
            "output_sel_ch1":       m.output_select_ch1.storage,
            "output_sel_ch2":       m.output_select_ch2.storage,
            "upsampler_input_mux":  m.upsampler_input_mux.storage,

            "phase_inc_nco":        m.phase_inc_nco.storage,
            "nco_mag":              m.nco_mag.storage,

            "phase_inc_down_1":     m.phase_inc_down_1.storage,
            "phase_inc_down_2":     m.phase_inc_down_2.storage,
            "phase_inc_down_3":     m.phase_inc_down_3.storage,
            "phase_inc_down_4":     m.phase_inc_down_4.storage,
            "phase_inc_down_5":     m.phase_inc_down_5.storage,
            "phase_inc_down_ref":   m.phase_inc_down_ref.storage,

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

            "lowspeed_dbg_select":  m.lowspeed_dbg_select.storage,
            "highspeed_dbg_select": m.highspeed_dbg_select.storage,

            "gain1":                m.gain1.storage,
            "gain2":                m.gain2.storage,
            "gain3":                m.gain3.storage,
            "gain4":                m.gain4.storage,
            "gain5":                m.gain5.storage,

            "ups_in_x":             m.upsampler_input_x.storage,
            "ups_in_y":             m.upsampler_input_y.storage,
            "final_shift":          m.final_shift.storage,

            "cap_enable":           m.cap_enable.storage,
            "cap_beats":            m.cap_beats.storage,
        }

        self.submodules.cfg_link = CSRConfigAFIFO(
            cfg_sys, cd_from="sys", cd_to="uc", fifo_depth=4
        )
        self.add_csr("cfg_link")

        # ce_down crossing from uc -> sys
        ce_down_uc  = Signal()
        ce_down_sys = Signal()

        self.submodules.ps_down = PulseSynchronizer("uc", "sys")
        self.comb += [
            self.ps_down.i.eq(ce_down_uc),
            ce_down_sys.eq(self.ps_down.o),
            self.evm.ce_down.trigger.eq(ce_down_sys),
        ]

        ds_x_uc   = Signal(16)
        ds_y_uc   = Signal(16)
        cap_sel_uc = Signal(12)

        uc = self.cfg_link

        self.specials += Instance(
            "uberclock",
            i_sys_clk              = ClockSignal("uc"),
            i_rst                  = ResetSignal("uc"),

            # ... ADC/DAC pins unchanged ...

            i_input_select         = getattr(uc, "out_input_select_uc"),
            i_output_select_ch1    = getattr(uc, "out_output_sel_ch1_uc"),
            i_output_select_ch2    = getattr(uc, "out_output_sel_ch2_uc"),
            i_upsampler_input_mux  = getattr(uc, "out_upsampler_input_mux_uc"),

            i_phase_inc_nco        = getattr(uc, "out_phase_inc_nco_uc"),
            i_nco_mag              = getattr(uc, "out_nco_mag_uc"),

            i_phase_inc_down_1     = getattr(uc, "out_phase_inc_down_1_uc"),
            i_phase_inc_down_2     = getattr(uc, "out_phase_inc_down_2_uc"),
            i_phase_inc_down_3     = getattr(uc, "out_phase_inc_down_3_uc"),
            i_phase_inc_down_4     = getattr(uc, "out_phase_inc_down_4_uc"),
            i_phase_inc_down_5     = getattr(uc, "out_phase_inc_down_5_uc"),
            i_phase_inc_down_ref   = getattr(uc, "out_phase_inc_down_ref_uc"),

            i_phase_inc_cpu1       = getattr(uc, "out_phase_inc_cpu1_uc"),
            i_phase_inc_cpu2       = getattr(uc, "out_phase_inc_cpu2_uc"),
            i_phase_inc_cpu3       = getattr(uc, "out_phase_inc_cpu3_uc"),
            i_phase_inc_cpu4       = getattr(uc, "out_phase_inc_cpu4_uc"),
            i_phase_inc_cpu5       = getattr(uc, "out_phase_inc_cpu5_uc"),

            i_mag_cpu1             = getattr(uc, "out_mag_cpu1_uc"),
            i_mag_cpu2             = getattr(uc, "out_mag_cpu2_uc"),
            i_mag_cpu3             = getattr(uc, "out_mag_cpu3_uc"),
            i_mag_cpu4             = getattr(uc, "out_mag_cpu4_uc"),
            i_mag_cpu5             = getattr(uc, "out_mag_cpu5_uc"),

            i_lowspeed_dbg_select  = getattr(uc, "out_lowspeed_dbg_select_uc"),
            i_highspeed_dbg_select = getattr(uc, "out_highspeed_dbg_select_uc"),

            i_gain1                = getattr(uc, "out_gain1_uc"),
            i_gain2                = getattr(uc, "out_gain2_uc"),
            i_gain3                = getattr(uc, "out_gain3_uc"),
            i_gain4                = getattr(uc, "out_gain4_uc"),
            i_gain5                = getattr(uc, "out_gain5_uc"),

            i_upsampler_input_x    = getattr(uc, "out_ups_in_x_uc"),
            i_upsampler_input_y    = getattr(uc, "out_ups_in_y_uc"),
            i_final_shift          = getattr(uc, "out_final_shift_uc"),

            o_ce_down              = ce_down_uc,
            o_downsampled_data_x   = ds_x_uc,
            o_downsampled_data_y   = ds_y_uc,

            o_cap_selected_input   = cap_sel_uc,
        )


        # Wire capture into UberDDR3 if present
        if hasattr(self, "ubddr3"):
            self.comb += [
                self.ubddr3.cap_sample.eq(cap_sel_uc),
                self.ubddr3.cap_enable_uc.eq(getattr(self.cfg_link, "out_cap_enable_uc")),
                self.ubddr3.cap_beats_uc.eq(getattr(self.cfg_link, "out_cap_beats_uc")),
            ]

        # Optional: LED2 stretch on ce_down
        ce_stretch = Signal(8)
        self.sync.sys += [
            If(ce_down_sys,
                ce_stretch.eq(0xff)
            ).Elif(ce_stretch != 0,
                ce_stretch.eq(ce_stretch - 1)
            )
        ]
        self.comb += leds[2].eq(ce_stretch != 0)


# =============================================================================
# Build script entry point
# =============================================================================
def main():
    from litex.build.parser import LiteXArgumentParser

    parser = LiteXArgumentParser(
        platform=alinx_ax7203.Platform,
        description="AX7203: CPU/CSR@100MHz, UberClock@65MHz, UberDDR3 with S2MM via wbxbar"
    )

    parser.add_target_argument("--cable",        default="ft232")
    parser.add_target_argument("--sys-clk-freq", default=100e6, type=float)
    ethopts = parser.target_group.add_mutually_exclusive_group()
    ethopts.add_argument("--with-ethernet",  action="store_true")
    ethopts.add_argument("--with-etherbone", action="store_true")
    sdopts = parser.target_group.add_mutually_exclusive_group()
    sdopts.add_argument("--with-spi-sdcard", action="store_true")
    sdopts.add_argument("--with-sdcard",     action="store_true")
    parser.add_argument("--with-pcie",       action="store_true")
    parser.add_argument("--with-hdmi",       action="store_true")
    parser.add_argument("--with-led-chaser", action="store_true")
    viopts = parser.target_group.add_mutually_exclusive_group()
    viopts.add_argument("--with-video-terminal",    action="store_true")
    viopts.add_argument("--with-video-framebuffer", action="store_true")
    viopts.add_argument("--with-video-colorbars",   action="store_true")
    parser.add_target_argument("--with-spi-flash",  action="store_true")
    parser.add_argument("--with-uberclock", action="store_true")
    parser.add_argument("--with-uberddr3",  action="store_true")

    args = parser.parse_args()

    soc = BaseSoC(
        toolchain                = args.toolchain,
        with_ethernet            = args.with_ethernet,
        with_etherbone           = args.with_etherbone,
        with_spi_flash           = args.with_spi_flash,
        with_sdcard              = args.with_sdcard,
        with_spi_sdcard          = args.with_spi_sdcard,
        with_pcie                = args.with_pcie,
        with_hdmi                = args.with_hdmi,
        with_led_chaser          = args.with_led_chaser,
        with_video_terminal      = args.with_video_terminal,
        with_video_framebuffer   = args.with_video_framebuffer,
        with_video_colorbars     = args.with_video_colorbars,
        with_uberclock           = args.with_uberclock,
        with_uberddr3            = args.with_uberddr3,
        **parser.soc_argdict
    )

    builder = Builder(soc, **parser.builder_argdict)

    if args.build:
        builder.build(**parser.toolchain_argdict)

    if args.load:
        prog = soc.platform.create_programmer(args.cable)
        prog.load_bitstream(builder.get_bitstream_filename(mode="sram"))


if __name__ == "__main__":
    main()
