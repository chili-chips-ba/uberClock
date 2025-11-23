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

from litex.soc.interconnect import stream
from litex.soc.interconnect import wishbone as wb

from litex.soc.interconnect.csr import CSRStorage, CSRStatus
from litex.soc.interconnect.csr_eventmanager import EventManager, EventSourcePulse
from litex.soc.integration.soc_core import *
from litex.soc.integration.builder import *
from litex.soc.integration.soc import SoCRegion

from litex.soc.cores.clock import *
from litex.soc.cores.led import LedChaser
from litex.soc.cores.timer import Timer
from litex.soc.cores.video import VideoS7HDMIPHY
from litex.soc.cores.dma import WishboneDMAWriter

from litedram.modules import MT41J256M16
from litedram.phy import s7ddrphy
from litedram.core.controller import ControllerSettings

from liteeth.phy.s7rgmii import LiteEthPHYRGMII
from litescope import LiteScopeAnalyzer

from migen.genlib.cdc import PulseSynchronizer, ClockDomainsRenamer, MultiReg
from migen.genlib.fifo import AsyncFIFO

import math

# --- set your repo paths ---
#   repository_dir:  root of your uberClock repo
#   verilog_dir:     directory that contains the Verilog sources used below
repository_dir = "/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/"
verilog_dir    = repository_dir + "/2.soc-litex/1.hw"


# =============================================================================
# CRG  (Clock/Reset Generation)
# =============================================================================
class _CRG(LiteXModule):
    """
    Clock/Reset generator for AX7203.

    - Takes board 200 MHz differential clock.
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

        # Tell the tools that paths through the MMCMs are false for timing
        platform.add_false_path_constraints(self.cd_sys.clk,       self.pll0.clkin)
        platform.add_false_path_constraints(self.cd_uc.clk,        self.pll1.clkin)
        platform.add_false_path_constraints(self.cd_ub_4x.clk,     self.pll0.clkin)
        platform.add_false_path_constraints(self.cd_ub_4x_dqs.clk, self.pll0.clkin)


# =============================================================================
# Classic -> Pipelined Wishbone bridge
# =============================================================================
class WBC2PipelineBridge(LiteXModule):
    """
    Bridge from classic Wishbone (LiteX side) to pipelined Wishbone
    (as used by UberDDR3) via the wbc2pipeline.v module.

    - 'self.s' is a classic WB slave interface (32/64/128-bit configurable).
    - 'm_*' is the pipelined WB master side
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
        self.m_sel   = Signal(data_width//8) # byte select
        self.m_stall = Signal()
        self.m_ack   = Signal()
        self.m_dat_r = Signal(data_width)
        self.m_err   = Signal()

        # Handle optional cti/bte on the classic side
        try:
            s_cti, s_bte = self.s.cti, self.s.bte
        except AttributeError:
            s_cti, s_bte = Signal(3, reset=0), Signal(2, reset=0)

        # Instantiate Verilog bridge
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
# CSR snapshot/commit (sys->uc)
# =============================================================================
class CSRConfigAFIFO(LiteXModule):
    """
    Capture a snapshot of multiple CSR fields in the sys domain and forward
    them atomically to the uc domain via an AsyncFIFO.

    - fields: dict(name -> Signal) in sys domain
    - commit CSR: rising edge queues a whole config frame into FIFO
    - out_<name>_uc: registered copies in uc domain updated when frame pops
    - cfg_update_uc: pulse in uc domain when a new frame is applied
    """
    def __init__(self, fields, cd_from="sys", cd_to="uc", fifo_depth=4):
        # Software writes 1 to commit a snapshot of all fields
        self.commit   = CSRStorage(1, description="Write 1 to snapshot & enqueue config frame.")
        # Sticky overflow flag if FIFO is full on commit
        self.overflow = CSRStatus(1, description="Sticky: commit attempted while FIFO full.")
        # FIFO status (bit0=readable, bit1=writable)
        self.level    = CSRStatus(8, description="FIFO flags: bit0=readable, bit1=writable.")

        # Total width of packed config frame
        total_w = sum(len(sig) for sig in fields.values())
        flat_sys = Signal(total_w)  # packed data in sys domain
        flat_uc  = Signal(total_w)  # packed data in uc domain

        # Concatenate all fields in a fixed order (sys side)
        offs = 0
        for _, sig in fields.items():
            w = len(sig)
            self.comb += flat_sys[offs:offs+w].eq(sig)
            offs += w

        # Async FIFO bridging cd_from -> cd_to
        fifo = AsyncFIFO(width=total_w, depth=fifo_depth)
        self.submodules.fifo = ClockDomainsRenamer({"write": cd_from, "read": cd_to})(fifo)

        # Edge detect commit (sys domain)
        commit_q = Signal()
        commit_rise = Signal()
        self.sync += commit_q.eq(self.commit.storage[0])
        self.comb += commit_rise.eq(self.commit.storage[0] & ~commit_q)

        # Write into FIFO when commit rising edge and FIFO writable
        self.comb += [
            fifo.din.eq(flat_sys),
            fifo.we.eq(commit_rise & fifo.writable)
        ]

        # Overflow flag if commit is requested when FIFO not writable
        self.sync += If(commit_rise & ~fifo.writable,
            self.overflow.status.eq(1)
        )

        # Export simple flags to CSR
        self.comb += self.level.status.eq(Cat(fifo.readable, fifo.writable, C(0, 6)))

        # Read side (uc domain)
        self.cfg_update_uc = Signal()  # pulse when new config frame is applied

        # Always read when something is available; you get "latest wins" behavior
        self.comb += fifo.re.eq(fifo.readable)

        # Register new flat frame when read
        self.sync += If(fifo.readable, flat_uc.eq(fifo.dout))
        # Pulse cfg_update_uc at same time
        self.sync += self.cfg_update_uc.eq(fifo.readable)

        # Unpack into out_<name>_uc signals
        offs = 0
        for name, sig in fields.items():
            w = len(sig)
            out = Signal(w, name=f"out_{name}_uc")
            setattr(self, f"out_{name}_uc", out)
            self.sync += If(fifo.readable,
                out.eq(flat_uc[offs:offs+w])
            )
            offs += w


# =============================================================================
# Minimal 2×1 Wishbone crossbar wrapper (wbxbar)
# =============================================================================
class WbXbar2x1(LiteXModule):
    """
    Small wrapper around the Verilog wbxbar for the common case:
      - NM = 2 masters (M0 = CPU path, M1 = DMA)
      - NS = 1 slave  (S0 = DDR3)
      - Pipelined Wishbone

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

        # Instance of the Verilog crossbar (NM=2, NS=1)
        self.specials += Instance("wbxbar",
            p_NM=2,
            p_NS=1,
            p_AW=AW,
            p_DW=DW,

            p_SLAVE_MASK=0,
            p_LGMAXBURST=6,
            p_OPT_TIMEOUT=0,
            p_OPT_STARVATION_TIMEOUT=0,
            p_OPT_DBLBUFFER=0,
            p_OPT_LOWPOWER=1,

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

            # Single slave unpacked
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


class RampSource(LiteXModule):
    def __init__(self, dw=256, length=256):
        assert dw % 16 == 0

        self.start = Signal()
        self.valid = Signal()
        self.ready = Signal()
        self.data  = Signal(dw)
        self.bytes = Signal(max=dw//8 + 1)
        self.last  = Signal()

        running   = Signal()
        base_step = Signal(16)           # base ramp value for this 256-bit beat
        idx       = Signal(max=length)   # how many beats emitted so far

        self.sync += [
            If(self.start,
                running.eq(1),
                idx.eq(0),
                base_step.eq(0),
            ).Elif(running & self.ready,
                # next beat: advance by 16 values
                base_step.eq(base_step + (dw // 16)),
                If(idx == (length - 1),
                    running.eq(0)
                ).Else(
                    idx.eq(idx + 1)
                )
            )
        ]

        self.comb += [
            self.valid.eq(running),
            self.bytes.eq(dw // 8),              # full beat
            self.last.eq(running & (idx == (length - 1))),
        ]

        # pack 16 consecutive values into the 256-bit bus
        lanes = dw // 16
        for i in range(lanes):
            self.comb += self.data[16*i:16*(i+1)].eq(base_step + i)


# =============================================================================
# UberDDR3 wrapper
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
      - It writes a streaming source (s_valid/s_data/etc.) into DDR3.

    DDR side:
      - ddr3_top Verilog module is the single slave on the crossbar.
      - Exposes DDR3 pins and calibration done CSR.
    """
    def __init__(self, platform, pads, locked,
                 sys_clk_hz=100e6, ddr_ck_hz=400e6,
                 row_bits=15, col_bits=10, ba_bits=3,
                 byte_lanes=4, dual_rank=0, speed_bin=3,
                 sdram_capacity=5, dll_off=0, odelay_supported=0, bist_mode=0):

        # (ctrl_ps, ddr_ps) kept for reference/debug; not used directly below
        ctrl_ps = int(round(1e12/float(sys_clk_hz)))
        ddr_ps  = int(round(1e12/float(ddr_ck_hz)))

        # DDR data width: 64 bits per byte lane
        ub_dw = 64 * byte_lanes
        serdes_ratio = 4

        # Address bits used inside DDR3 controller
        wb_addr_bits = (
            row_bits + col_bits + ba_bits
            - int(math.log2(serdes_ratio * 2))
            + dual_rank
        )

        # ------------------------------------------------------------------
        # Classic CPU WB -> wide WB (ub_dw) via Converter
        # ------------------------------------------------------------------
        self.wb = wb.Interface(data_width=32)           # external WB port (32-bit)
        wb_wide = wb.Interface(data_width=ub_dw)        # internal wide port (e.g. 256-bit)

        self.submodules.wb_up = wb.Converter(self.wb, wb_wide)

        # ------------------------------------------------------------------
        # Classic -> pipelined bridge
        # ------------------------------------------------------------------
        self.submodules.c2p = WBC2PipelineBridge(
            data_width   = wb_wide.data_width,
            adr_width    = wb_wide.adr_width,
            clock_domain ="sys"
        )
        # Connect wide classic WB to bridge slave side
        self.comb += wb_wide.connect(self.c2p.s)

        # Address/data widths on pipelined side
        AW = len(self.c2p.m_adr)
        DW = ub_dw

        # Crossbar
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
        # WBLSB: # of address bits that correspond to byte lane selection
        WBLSB = int(math.log2(DW//8))

        # zipdma_s2mm's ADDRESS_WIDTH counts bytes, not bus words
        ADDR_WIDTH_FOR_DMA = AW + WBLSB

        # Exposed source stream into DMA (sys domain side of CDC bridge)
        self.s_valid = Signal()
        self.s_ready = Signal()
        self.s_data  = Signal(DW)
        self.s_bytes = Signal(max=DW//8+1)  # number of valid bytes-1
        self.s_last  = Signal()

        # DMA control / status CSRs
        self.dma_req   = CSRStorage(1, description="Write 1 (edge) to start S2MM (uses addr/size/inc).")
        self.dma_busy  = CSRStatus(1)
        self.dma_err   = CSRStatus(1)
        self.dma_inc   = CSRStorage(1, reset=1)
        self.dma_size  = CSRStorage(2, reset=0, description="00=bus,01=32b,10=16b,11=byte")
        self.dma_addr0 = CSRStorage(32)  # lower part of DMA address
        self.dma_addr1 = CSRStorage(32)  # upper part of DMA address

        # Edge-detect dma_req (treat as "start" pulse) in sys domain
        dma_req_pulse = Signal()
        dma_req_q     = Signal()
        self.sync += dma_req_q.eq(self.dma_req.storage[0])
        self.comb += dma_req_pulse.eq(self.dma_req.storage[0] & ~dma_req_q)

        # Compose full DMA address (truncate to ADDR_WIDTH_FOR_DMA)
        dma_addr = Signal(ADDR_WIDTH_FOR_DMA)
        self.comb += dma_addr.eq(
            Cat(self.dma_addr0.storage, self.dma_addr1.storage)[:ADDR_WIDTH_FOR_DMA]
        )

        # S2MM DMA instance
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
        # Internal ramp source (uc domain) + CDC into S2MM (sys domain)
        # ------------------------------------------------------------------

        # Ramp lives in uc clock domain
        self.submodules.ramp = ClockDomainsRenamer("uc")(RampSource(dw=DW, length=256))

        # sys -> uc pulse: start ramp when DMA request occurs
        self.submodules.ps_ramp_start = PulseSynchronizer("sys", "uc")
        self.comb += self.ps_ramp_start.i.eq(dma_req_pulse)
        self.comb += self.ramp.start.eq(self.ps_ramp_start.o)

        # Async FIFO: write@uc, read@sys, carries {data, bytes, last}
        bytes_width = len(self.s_bytes)
        fifo_width  = DW + bytes_width + 1  # data + bytes + last

        fifo = AsyncFIFO(width=fifo_width, depth=4)
        self.submodules.s2mm_fifo = ClockDomainsRenamer(
            {"write": "uc", "read": "sys"}
        )(fifo)

        # uc side (producer): ramp -> FIFO
        self.comb += [
            fifo.din.eq(Cat(self.ramp.data, self.ramp.bytes, self.ramp.last)),
            fifo.we.eq(self.ramp.valid & fifo.writable),
            # backpressure towards ramp
            self.ramp.ready.eq(fifo.writable),
        ]

        # sys side (consumer): FIFO -> S2MM stream
        data_sys   = Signal(DW)
        bytes_sys  = Signal(bytes_width)
        last_sys   = Signal()

        self.comb += [
            Cat(data_sys, bytes_sys, last_sys).eq(fifo.dout),

            # Drive DMA stream from FIFO
            self.s_valid.eq(fifo.readable),
            self.s_data.eq(data_sys),
            self.s_bytes.eq(bytes_sys),
            self.s_last.eq(last_sys),

            # Pop FIFO when S2MM can accept data
            fifo.re.eq(self.s_ready & fifo.readable),
        ]

        # ------------------------------------------------------------------
        # DDR3 top as slave on the xbar
        # ------------------------------------------------------------------
        self.calib_done = CSRStatus()  # exported calibration-complete flag

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

            # Clocking / reset
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

            # Auxiliary signals (e.g., write strobes, etc.)
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

            # Status / misc
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

        # DDR Vref property for the IO bank used by DDR signals
        platform.add_platform_command(
            "set_property INTERNAL_VREF 0.75 "
            "[get_iobanks -of_objects [get_ports {{ddram_dq[*] ddram_dqs_p[*] ddram_dqs_n[*]}}]]"
        )
        # Recommended bitstream startup setting
        platform.add_platform_command(
            "set_property BITSTREAM.STARTUP.MATCH_CYCLE 6 [current_design]"
        )


# =============================================================================
# Main CSRs (UberClock configuration registers)
# =============================================================================
class MainCSRs(LiteXModule):
    """
    Set of CSRs that control the UberClock DSP block:

      - phase_inc_*:  phase increments for NCO and various downsamplers/CPU
      - input/output_select: routing of signals to/from UberClock
      - gains: 32-bit fixed-point gains for 5 stages
      - upsampler_input_x/y: direct complex input into upsampler
      - final_shift: output scaling (shift)
      - cap_enable: full-rate (65 MS/s) capture enable flag
    """
    def __init__(self):
        self.phase_inc_nco       = CSRStorage(19)
        self.phase_inc_down_1    = CSRStorage(19)
        self.phase_inc_down_2    = CSRStorage(19)
        self.phase_inc_down_3    = CSRStorage(19)
        self.phase_inc_down_4    = CSRStorage(19)
        self.phase_inc_down_5    = CSRStorage(19)
        self.phase_inc_cpu       = CSRStorage(19)
        self.input_select        = CSRStorage(2)
        self.output_select_ch1   = CSRStorage(2)
        self.output_select_ch2   = CSRStorage(2)
        self.upsampler_input_mux = CSRStorage(2)
        self.gain1               = CSRStorage(32)
        self.gain2               = CSRStorage(32)
        self.gain3               = CSRStorage(32)
        self.gain4               = CSRStorage(32)
        self.gain5               = CSRStorage(32)
        self.upsampler_input_x   = CSRStorage(16)
        self.upsampler_input_y   = CSRStorage(16)
        self.final_shift         = CSRStorage(3)
        self.cap_enable          = CSRStorage(1, description="1=enable full-rate capture (65 MS/s).")


# =============================================================================
# SoC
# =============================================================================
class BaseSoC(SoCCore):
    """
    Top-level LiteX SoC for AX7203:

      - System clock 100 MHz via _CRG
      - Optional: LiteDRAM as main RAM
      - Optional: UberDDR3 as side memory (mapped at UB base)
      - Optional: Ethernet / Etherbone
      - Optional: SPI Flash, SDCard
      - Optional: HDMI video
      - UberClock DSP block in 65 MHz domain
    """
    def __init__(self, toolchain="vivado",
                 with_hdmi=False, with_ethernet=False, with_etherbone=False,
                 with_spi_flash=False, with_led_chaser=False,
                 with_sdcard=False, with_spi_sdcard=False, with_pcie=False,
                 with_video_terminal=False, with_video_framebuffer=False, with_video_colorbars=False,
                 with_ledmem=False, with_uberclock=False, with_uberddr3=False,
                 **kwargs):

        # Default main RAM size if no external DRAM used
        kwargs.setdefault("integrated_main_ram_size", 64*1024)

        # Use serial UART (JTAG-UART also possible)
        kwargs["uart_name"] = "serial"

        # Create platform instance
        platform = alinx_ax7203.Platform(toolchain=toolchain)

        # If uberddr3 is used or no integrated RAM is requested, we need DDR clocks
        need_ddr_clks = with_uberddr3 or (kwargs.get("integrated_main_ram_size", 0) == 0)
        self.crg = _CRG(platform, need_ddr_clks=need_ddr_clks)

        # Base SoC at 100 MHz using our CRG
        SoCCore.__init__(self, platform, 100e6,
            ident="AX7203 UberClock65 UberDDR3 with S2MM via wbxbar",
            **kwargs)

        # ------------------------------------------------------------------
        # Heartbeat LED (LED0)
        # ------------------------------------------------------------------
        hb = Signal(24)
        self.sync += hb.eq(hb + 1)
        leds = Cat(*platform.request_all("user_led"))
        self.comb += leds[0].eq(hb[23])

        # Timer1 peripheral (standard LiteX timer)
        self.submodules.timer1 = Timer()
        self.add_csr("timer1")

        # ------------------------------------------------------------------
        # LiteDRAM path (standard SoC DRAM)
        # ------------------------------------------------------------------
        # Use standard LiteDRAM only if:
        #   - integrated_main_ram_size == 0 (no internal SRAM), and
        #   - with_uberddr3 is False (we're not using UberDDR3 instead)
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
        # UberDDR3 side-memory (separate from LiteDRAM main RAM)
        # ------------------------------------------------------------------
        if with_uberddr3:
            pads = platform.request("ddram")
            self.submodules.ubddr3 = UberDDR3(
                platform = platform,
                pads     = pads,
                locked   = self.crg.pll0.locked,
                sys_clk_hz=100e6,
                ddr_ck_hz=400e6,
                row_bits=15,
                col_bits=10,
                ba_bits=3,
                byte_lanes=4,
                dual_rank=0,
                speed_bin=3,
                sdram_capacity=5,
                dll_off=0,
                odelay_supported=0,
                bist_mode=0
            )

            # Map UberDDR3 memory into address space (e.g. 0xA000_0000)
            ub_base = 0xA000_0000
            ub_size = 0x1000_0000  # 256 MiB
            region  = SoCRegion(origin=ub_base, size=ub_size, cached=False, linker=False)

            # Add as a LiteX bus slave
            self.bus.add_slave("ub_ram", self.ubddr3.wb, region)

            # Export base address as a constant to firmware
            self.add_constant("UBDDR3_MEM_BASE", ub_base)
            self.add_csr("ubddr3")

            # Light up LED1 when DDR3 calib done
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
                    ip_address="192.168.1.123",
                    mac_address=0x0200000000AB
                )

        # ------------------------------------------------------------------
        # SPI Flash (N25Q128)
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
        # LED chaser (test pattern on user LEDs)
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

    # === UberClock integration ===
    def _add_uberclock_fullrate(self, verilog_dir, leds):
        """
        Add the UberClock DSP block to the SoC:

          - Loads Verilog sources for ADC/DAC, filters, CORDIC, uberclock top.
          - Instantiates MainCSRs for configuration.
          - Uses CSRConfigAFIFO to push configs from sys -> uc domain.
          - Connects ADC/DAC pins from platform.
          - Exposes downsampled data via CSR event (ce_down).
        """

        # Add all required Verilog sources for UberClock pipeline
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

        # EventManager: single event 'ce_down' (downsample clock enable)
        self.submodules.evm = EventManager()
        self.evm.ce_down = EventSourcePulse(description="Downsample ready")
        self.evm.finalize()
        # Wire event manager to CPU interrupt controller
        self.irq.add("evm")
        self.add_csr("evm")

        # Map CSR storages into a dict for CSRConfigAFIFO
        m = self.main
        cfg_sys = {
            "input_select":        m.input_select.storage,
            "output_sel_ch1":      m.output_select_ch1.storage,
            "output_sel_ch2":      m.output_select_ch2.storage,
            "upsampler_input_mux": m.upsampler_input_mux.storage,
            "phase_inc_nco":       m.phase_inc_nco.storage,
            "phase_inc_down_1":    m.phase_inc_down_1.storage,
            "phase_inc_down_2":    m.phase_inc_down_2.storage,
            "phase_inc_down_3":    m.phase_inc_down_3.storage,
            "phase_inc_down_4":    m.phase_inc_down_4.storage,
            "phase_inc_down_5":    m.phase_inc_down_5.storage,
            "phase_inc_cpu":       m.phase_inc_cpu.storage,
            "gain1":               m.gain1.storage,
            "gain2":               m.gain2.storage,
            "gain3":               m.gain3.storage,
            "gain4":               m.gain4.storage,
            "gain5":               m.gain5.storage,
            "ups_in_x":            m.upsampler_input_x.storage,
            "ups_in_y":            m.upsampler_input_y.storage,
            "final_shift":         m.final_shift.storage,
            "cap_enable":          m.cap_enable.storage,
        }

        # Create async FIFO link from sys -> uc
        self.submodules.cfg_link = CSRConfigAFIFO(
            cfg_sys, cd_from="sys", cd_to="uc", fifo_depth=4
        )
        self.add_csr("cfg_link")

        # ------------------------------------------------------------------
        # ce_down event crossing from uc -> sys
        # ------------------------------------------------------------------
        ce_down_uc  = Signal()   # pulse in uc domain from uberclock
        ce_down_sys = Signal()   # same pulse synchronized to sys

        # PulseSynchronizer takes a pulse from uc and recreates it in sys
        self.submodules.ps_down = PulseSynchronizer("uc", "sys")
        self.comb += [
            self.ps_down.i.eq(ce_down_uc),
            ce_down_sys.eq(self.ps_down.o),
            self.evm.ce_down.trigger.eq(ce_down_sys),
        ]

        # Downsampled data outputs from uberclock (uc domain)
        ds_x_uc   = Signal(16)
        ds_y_uc   = Signal(16)

        # Short alias for cfg_link module
        uc = self.cfg_link

        # ------------------------------------------------------------------
        # UberClock Verilog instance
        # ------------------------------------------------------------------
        self.specials += Instance(
            "uberclock",
            # Clock + reset in uc domain
            i_sys_clk  = ClockSignal("uc"),
            i_rst      = ResetSignal("uc"),

            # ADC interface
            o_adc_clk_ch0  = self.platform.request("adc_clk_ch0"),
            o_adc_clk_ch1  = self.platform.request("adc_clk_ch1"),
            i_adc_data_ch0 = self.platform.request("adc_data_ch0"),
            i_adc_data_ch1 = self.platform.request("adc_data_ch1"),

            # DAC interface
            o_da1_clk  = self.platform.request("da1_clk", 0),
            o_da1_wrt  = self.platform.request("da1_wrt", 0),
            o_da1_data = self.platform.request("da1_data",0),
            o_da2_clk  = self.platform.request("da2_clk", 0),
            o_da2_wrt  = self.platform.request("da2_wrt", 0),
            o_da2_data = self.platform.request("da2_data",0),

            # Configuration inputs from cfg_link (uc domain copies)
            i_input_select        = getattr(uc, "out_input_select_uc"),
            i_output_select_ch1   = getattr(uc, "out_output_sel_ch1_uc"),
            i_output_select_ch2   = getattr(uc, "out_output_sel_ch2_uc"),
            i_upsampler_input_mux = getattr(uc, "out_upsampler_input_mux_uc"),
            i_phase_inc_nco       = getattr(uc, "out_phase_inc_nco_uc"),
            i_phase_inc_down_1    = getattr(uc, "out_phase_inc_down_1_uc"),
            i_phase_inc_down_2    = getattr(uc, "out_phase_inc_down_2_uc"),
            i_phase_inc_down_3    = getattr(uc, "out_phase_inc_down_3_uc"),
            i_phase_inc_down_4    = getattr(uc, "out_phase_inc_down_4_uc"),
            i_phase_inc_down_5    = getattr(uc, "out_phase_inc_down_5_uc"),
            i_phase_inc_cpu       = getattr(uc, "out_phase_inc_cpu_uc"),
            i_gain1               = getattr(uc, "out_gain1_uc"),
            i_gain2               = getattr(uc, "out_gain2_uc"),
            i_gain3               = getattr(uc, "out_gain3_uc"),
            i_gain4               = getattr(uc, "out_gain4_uc"),
            i_gain5               = getattr(uc, "out_gain5_uc"),
            i_upsampler_input_x   = getattr(uc, "out_ups_in_x_uc"),
            i_upsampler_input_y   = getattr(uc, "out_ups_in_y_uc"),
            i_final_shift         = getattr(uc, "out_final_shift_uc"),

            # Downsampled outputs + event
            o_ce_down             = ce_down_uc,
            o_downsampled_data_x  = ds_x_uc,
            o_downsampled_data_y  = ds_y_uc,

            # Magnitude/phase outputs (currently unused here)
            o_magnitude           = Open(16),
            o_phase               = Open(25),
        )


# =============================================================================
# Build script entry point
# =============================================================================
def main():
    from litex.build.parser import LiteXArgumentParser

    # Argument parser with standard LiteX SoC options plus target-specific ones
    parser = LiteXArgumentParser(
        platform=alinx_ax7203.Platform,
        description="AX7203: CPU/CSR@100MHz, UberClock@65MHz, UberDDR3 with S2MM via wbxbar"
    )

    # Programming cable selection
    parser.add_target_argument("--cable",        default="ft232")
    # System clock frequency (should match CRG/SoCCore freq above)
    parser.add_target_argument("--sys-clk-freq", default=100e6, type=float)

    # Ethernet vs Etherbone (mutually exclusive)
    ethopts = parser.target_group.add_mutually_exclusive_group()
    ethopts.add_argument("--with-ethernet",  action="store_true")
    ethopts.add_argument("--with-etherbone", action="store_true")

    # SDCard vs SPI-SDCard (mutually exclusive)
    sdopts = parser.target_group.add_mutually_exclusive_group()
    sdopts.add_argument("--with-spi-sdcard", action="store_true")
    sdopts.add_argument("--with-sdcard",     action="store_true")

    # Optional features
    parser.add_argument("--with-pcie",       action="store_true")
    parser.add_argument("--with-hdmi",       action="store_true")
    parser.add_argument("--with-led-chaser", action="store_true")

    # Video options (mutually exclusive)
    viopts = parser.target_group.add_mutually_exclusive_group()
    viopts.add_argument("--with-video-terminal",    action="store_true")
    viopts.add_argument("--with-video-framebuffer", action="store_true")
    viopts.add_argument("--with-video-colorbars",   action="store_true")

    parser.add_target_argument("--with-spi-flash",  action="store_true")
    parser.add_argument("--with-uberclock", action="store_true")
    parser.add_argument("--with-uberddr3",  action="store_true")

    args = parser.parse_args()

    # Create SoC with parsed options (+ standard LiteX SoC args via soc_argdict)
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

    # Builder: handles gateware build + firmware image, etc.
    builder = Builder(soc, **parser.builder_argdict)

    # If --build is passed, run synthesis/P&R/bitstream
    if args.build:
        builder.build(**parser.toolchain_argdict)

    # If --load is passed, program bitstream to FPGA via chosen cable
    if args.load:
        prog = soc.platform.create_programmer(args.cable)
        prog.load_bitstream(builder.get_bitstream_filename(mode="sram"))


if __name__ == "__main__":
    main()
