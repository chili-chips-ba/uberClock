from __future__ import annotations

import math
from typing import Optional

from migen import *
from litex.gen import *
from litex.soc.interconnect import wishbone as wb
from litex.soc.interconnect.csr import CSRStorage, CSRStatus
from migen.genlib.cdc import PulseSynchronizer, ClockDomainsRenamer
from migen.genlib.fifo import AsyncFIFO

from .wishbone import ClassicToPipelinedWishboneBridge, PipelinedWishboneXbar2M1S
from .streams import UCStreamMux, SamplePackerStream
from .rtl_sources import add_sources


# =============================================================================
# UberDDR3: DDR3 controller wrapper + DMA writer (UC->DDR) + CPU WB access
# =============================================================================
class UberDDR3(LiteXModule):
    """
    UberDDR3 top-level integration block.

    Use-case
    --------
    This module wires a Verilog DDR3 controller (`ddr3_top`) into a LiteX SoC and
    provides TWO ways to write/read DDR:

      1) CPU / software access (SYS domain)
         - Classic 32-bit Wishbone (LiteX standard) exposed as `self.wb`.
         - Width-converted up to the DDR bus width (UB_BUS_WIDTH_BITS).
         - Bridged into pipelined WB signaling used by the controller.

      2) High-throughput capture / DMA (UC domain -> SYS domain -> DDR)
         - A UC-domain stream source generates DW-bit beats:
             - Either an internal ramp (debug/validation)
             - Or packed samples from the design (e.g. selected 12-bit ADC path)
         - Stream crosses UC->SYS via AsyncFIFO
         - zipdma_s2mm writes the stream into DDR through a shared crossbar

    Clock domains
    -------------
      - "sys"    : CPU, CSR bus, zipdma_s2mm, Wishbone fabric
      - "uc"     : UberClock DSP / sample domain
      - "ub_4x"  : DDR3 PHY clock
      - "ub_4x_dqs" : DDR3 PHY clock 90° shifted (DQS)
      - "idelay" : IDELAYCTRL reference clock

    External inputs (from SoC)
    --------------------------
      - cap_enable_uc : 1 = capture external design samples, 0 = use ramp
      - cap_beats_uc  : number of DW-bit beats to write
      - cap_sample    : the selected 12-bit sample (UC domain)

    CSRs (SYS domain)
    -----------------
      - dma_req   : strobe to start DMA write
      - dma_addr0/1 : 64-bit base address (packed from two 32-bit regs)
      - dma_inc   : increment mode for DMA
      - dma_size  : transfer size encoding (zipdma convention)
      - dma_busy/dma_err : status from zipdma
      - calib_done : DDR controller calibration complete
    """

    # ----------------------------
    # Defaults / constants
    # ----------------------------
    DEFAULT_SERDES_RATIO = 4           # matches your Verilog controller assumptions
    DEFAULT_CAP_BEATS    = 256
    DEFAULT_S2MM_FIFO_DEPTH = 256       # UC->SYS stream FIFO depth (beats)

    def __init__(
        self,
        platform,
        pads,
        locked: Signal,
        *,
        SYS_CLK_HZ: float = 100e6,
        DDR_CK_HZ: float = 400e6,
        ROW_BITS: int = 15,
        COL_BITS: int = 10,
        BA_BITS: int = 3,
        BYTE_LANES: int = 4,
        DUAL_RANK: int = 0,
        SPEED_BIN: int = 3,
        SDRAM_CAPACITY: int = 5,
        DLL_OFF: int = 0,
        ODELAY_SUPPORTED: int = 0,
        BIST_MODE: int = 0,
    ):
        # ------------------------------------------------------------------
        # Derived bus parameters
        # ------------------------------------------------------------------
        UB_BUS_WIDTH_BITS = 64 * BYTE_LANES
        SERDES_RATIO      = self.DEFAULT_SERDES_RATIO

        # Address width used by ddr3_top WB port (word addressing inside controller)
        WB_ADDR_BITS = (
            ROW_BITS + COL_BITS + BA_BITS
            - int(math.log2(SERDES_RATIO * 2))
            + DUAL_RANK
        )

        # ------------------------------------------------------------------
        # (1) CPU path: classic 32-bit WB -> wide WB -> pipelined WB -> xbar M0
        # ------------------------------------------------------------------
        self.wb = wb.Interface(data_width=32)
        wb_wide = wb.Interface(data_width=UB_BUS_WIDTH_BITS)
        self.submodules.wb_up = wb.Converter(self.wb, wb_wide)

        self.submodules.c2p = ClassicToPipelinedWishboneBridge(
            data_width=wb_wide.data_width,
            adr_width=wb_wide.adr_width,
            clock_domain="sys",
        )
        self.comb += wb_wide.connect(self.c2p.s)

        AW = len(self.c2p.m_adr)         # pipelined WB address width (words on wide bus)
        DW = UB_BUS_WIDTH_BITS           # wide bus width

        self.submodules.xbar = PipelinedWishboneXbar2M1S(AW=AW, DW=DW, clock_domain="sys")

        # Connect CPU pipelined master -> xbar master0
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
        # (2) DMA writer: zipdma_s2mm is xbar master1
        # ------------------------------------------------------------------
        WB_LSB_BITS          = int(math.log2(DW // 8))
        DMA_ADDRESS_WIDTH    = AW + WB_LSB_BITS

        # UC-domain capture interface (wired from SoC)
        self.cap_sample    = Signal(12, name="cap_sample_uc")
        self.cap_enable_uc = Signal(name="cap_enable_uc")
        self.cap_beats_uc  = Signal(32, reset=self.DEFAULT_CAP_BEATS, name="cap_beats_uc")

        # Stream into DMA (SYS domain)
        self.s_valid = Signal(name="s2mm_s_valid")
        self.s_ready = Signal(name="s2mm_s_ready")
        self.s_data  = Signal(DW, name="s2mm_s_data")
        self.s_bytes = Signal(max=DW // 8 + 1, name="s2mm_s_bytes")
        self.s_last  = Signal(name="s2mm_s_last")

        # DMA CSRs
        self.dma_req = CSRStorage(
            1,
            description="Write (strobe) to start S2MM DMA write. Uses dma_addr[63:0], dma_inc, dma_size.",
        )
        self.dma_busy = CSRStatus(1, description="DMA busy flag from zipdma_s2mm.")
        self.dma_err  = CSRStatus(1, description="DMA error flag from zipdma_s2mm (sticky in the Verilog core).")
        self.dma_inc  = CSRStorage(1, reset=1, description="DMA increment enable (1=increment address, 0=fixed).")
        self.dma_size = CSRStorage(
            2,
            reset=0,
            description="DMA transfer size encoding: 00=bus-width, 01=32-bit, 10=16-bit, 11=8-bit.",
        )
        self.dma_addr0 = CSRStorage(32, description="DMA base address [31:0].")
        self.dma_addr1 = CSRStorage(32, description="DMA base address [63:32].")

        dma_req_pulse = Signal(name="dma_req_pulse_sys")
        self.comb += dma_req_pulse.eq(self.dma_req.re)

        dma_addr = Signal(DMA_ADDRESS_WIDTH, name="dma_addr_sys")
        self.comb += dma_addr.eq(
            Cat(self.dma_addr0.storage, self.dma_addr1.storage)[:DMA_ADDRESS_WIDTH]
        )

        self.specials += Instance(
            "zipdma_s2mm",
            p_ADDRESS_WIDTH     = DMA_ADDRESS_WIDTH,
            p_BUS_WIDTH         = DW,
            p_OPT_LITTLE_ENDIAN = 1,
            p_LGPIPE            = 10,

            i_i_clk   = ClockSignal("sys"),
            i_i_reset = ResetSignal("sys"),

            i_i_request = dma_req_pulse,
            o_o_busy    = self.dma_busy.status,
            o_o_err     = self.dma_err.status,
            i_i_inc     = self.dma_inc.storage[0],
            i_i_size    = self.dma_size.storage,
            i_i_addr    = dma_addr,

            # Input stream (SYS domain side)
            i_S_VALID = self.s_valid,
            o_S_READY = self.s_ready,
            i_S_DATA  = self.s_data,
            i_S_BYTES = self.s_bytes,
            i_S_LAST  = self.s_last,

            # WB write port to DDR through xbar master1
            o_o_wr_cyc   = self.xbar.m1_cyc,
            o_o_wr_stb   = self.xbar.m1_stb,
            o_o_wr_we    = self.xbar.m1_we,
            o_o_wr_addr  = self.xbar.m1_adr,
            o_o_wr_data  = self.xbar.m1_dat_w,
            o_o_wr_sel   = self.xbar.m1_sel,
            i_i_wr_stall = self.xbar.m1_stall,
            i_i_wr_ack   = self.xbar.m1_ack,
            i_i_wr_data  = self.xbar.m1_dat_r,
            i_i_wr_err   = self.xbar.m1_err,
        )

        # ------------------------------------------------------------------
        # UC stream source: ramp OR packed external samples (UC domain)
        # ------------------------------------------------------------------
        self.submodules.uc_src = ClockDomainsRenamer("uc")(
            UCStreamMux(bus_data_width=DW, max_beats=1 << 23)
        )
        self.submodules.cap_stream = ClockDomainsRenamer("uc")(
            SamplePackerStream(sample_width=12, bus_data_width=DW, beat_fifo_depth=2048)
        )

        # SYS->UC: start pulse derived from DMA request
        self.submodules.ps_start = PulseSynchronizer("sys", "uc")
        self.comb += self.ps_start.i.eq(dma_req_pulse)

        # Control / length wiring in UC domain
        self.comb += [
            self.uc_src.use_external.eq(self.cap_enable_uc),
            self.uc_src.ramp_length_beats.eq(self.cap_beats_uc),
            self.uc_src.start.eq(self.ps_start.o & ~self.cap_enable_uc),
            self.cap_stream.start.eq(self.ps_start.o & self.cap_enable_uc),
        ]
        self.comb += [
            self.cap_stream.sample_in.eq(self.cap_sample),
            self.cap_stream.frames.eq(self.cap_beats_uc),
        ]
        self.comb += [
            self.uc_src.ext_valid.eq(self.cap_stream.valid),
            self.uc_src.ext_data.eq(self.cap_stream.data),
            self.uc_src.ext_bytes.eq(self.cap_stream.bytes),
            self.uc_src.ext_last.eq(self.cap_stream.last),
            self.cap_stream.ready.eq(self.uc_src.ext_ready),
        ]

        # ------------------------------------------------------------------
        # UC->SYS CDC: async FIFO carries {data, bytes, last}
        # ------------------------------------------------------------------
        bytes_width = len(self.s_bytes)
        fifo_width  = DW + bytes_width + 1

        fifo = AsyncFIFO(width=fifo_width, depth=self.DEFAULT_S2MM_FIFO_DEPTH)
        self.submodules.s2mm_fifo = ClockDomainsRenamer({"write": "uc", "read": "sys"})(fifo)

        self.comb += [
            fifo.din.eq(Cat(self.uc_src.data, self.uc_src.bytes, self.uc_src.last)),
            fifo.we.eq(self.uc_src.valid & fifo.writable),
            self.uc_src.ready.eq(fifo.writable),
        ]

        data_sys  = Signal(DW, name="fifo_data_sys")
        bytes_sys = Signal(bytes_width, name="fifo_bytes_sys")
        last_sys  = Signal(name="fifo_last_sys")

        self.comb += [
            Cat(data_sys, bytes_sys, last_sys).eq(fifo.dout),

            self.s_valid.eq(fifo.readable),
            self.s_data.eq(data_sys),
            self.s_bytes.eq(bytes_sys),
            self.s_last.eq(last_sys),

            fifo.re.eq(self.s_ready & fifo.readable),
        ]

        # ------------------------------------------------------------------
        # DDR3 controller (xbar slave0)
        # ------------------------------------------------------------------
        self.calib_done = CSRStatus(1, description="DDR3 controller calibration complete.")

        self.specials += Instance(
            "ddr3_top",
            p_CONTROLLER_CLK_PERIOD = int(round(1e12 / float(SYS_CLK_HZ))),
            p_DDR3_CLK_PERIOD       = int(round(1e12 / float(DDR_CK_HZ))),
            p_ROW_BITS              = ROW_BITS,
            p_COL_BITS              = COL_BITS,
            p_BA_BITS               = BA_BITS,
            p_BYTE_LANES            = BYTE_LANES,
            p_DUAL_RANK_DIMM        = DUAL_RANK,
            p_SPEED_BIN             = SPEED_BIN,
            p_SDRAM_CAPACITY        = SDRAM_CAPACITY,
            p_DLL_OFF               = DLL_OFF,
            p_ODELAY_SUPPORTED      = ODELAY_SUPPORTED,
            p_BIST_MODE             = BIST_MODE,
            p_DIC                   = 0b01,
            p_RTT_NOM               = 0b001,
            p_AUX_WIDTH             = 4,

            i_i_controller_clk = ClockSignal("sys"),
            i_i_ddr3_clk       = ClockSignal("ub_4x"),
            i_i_ddr3_clk_90    = ClockSignal("ub_4x_dqs"),
            i_i_ref_clk        = ClockSignal("idelay"),
            i_i_rst_n          = locked & ~ResetSignal("sys"),

            # Pipelined Wishbone slave interface from xbar
            i_i_wb_cyc   = self.xbar.s_cyc,
            i_i_wb_stb   = self.xbar.s_stb,
            i_i_wb_we    = self.xbar.s_we,
            i_i_wb_addr  = self.xbar.s_adr[:WB_ADDR_BITS],
            i_i_wb_data  = self.xbar.s_dat_w,
            i_i_wb_sel   = self.xbar.s_sel,
            o_o_wb_stall = self.xbar.s_stall,
            o_o_wb_ack   = self.xbar.s_ack,
            o_o_wb_err   = self.xbar.s_err,
            o_o_wb_data  = self.xbar.s_dat_r,

            i_i_aux = Cat(self.xbar.s_we, C(0, 3)),
            o_o_aux = Open(4),

            # Second WB port disabled
            i_i_wb2_cyc   = 0,
            i_i_wb2_stb   = 0,
            i_i_wb2_we    = 0,
            i_i_wb2_addr  = 0,
            i_i_wb2_data  = 0,
            i_i_wb2_sel   = 0,
            o_o_wb2_stall = Open(),
            o_o_wb2_ack   = Open(),
            o_o_wb2_data  = Open(),

            # DDR3 pads
            o_o_ddr3_clk_p   = pads.clk_p,
            o_o_ddr3_clk_n   = pads.clk_n,
            o_o_ddr3_reset_n = pads.reset_n,
            o_o_ddr3_cke     = pads.cke,
            o_o_ddr3_cs_n    = pads.cs_n,
            o_o_ddr3_ras_n   = pads.ras_n,
            o_o_ddr3_cas_n   = pads.cas_n,
            o_o_ddr3_we_n    = pads.we_n,
            o_o_ddr3_addr    = pads.a,
            o_o_ddr3_ba_addr = pads.ba,
            io_io_ddr3_dq    = pads.dq,
            io_io_ddr3_dqs   = pads.dqs_p,
            io_io_ddr3_dqs_n = pads.dqs_n,
            o_o_ddr3_dm      = pads.dm,
            o_o_ddr3_odt     = pads.odt,

            o_o_calib_complete    = self.calib_done.status,
            o_o_debug1            = Open(),
            i_i_user_self_refresh = 0,
            o_uart_tx             = Open(),
        )

        # ------------------------------------------------------------------
        # RTL sources + Vivado properties
        # ------------------------------------------------------------------
        add_sources(platform, [
            "memory/ddr3_top.v",
            "memory/ddr3_controller.v",
            "memory/ddr3_phy.v",
            "memory/wbc2pipeline.v",
            "memory/wbxbar.v",
            "memory/skidbuffer.v",
            "memory/addrdecode.v",
            "memory/zipdma_s2mm.v",
        ])

        platform.add_platform_command(
            "set_property INTERNAL_VREF 0.75 "
            "[get_iobanks -of_objects [get_ports {{ddram_dq[*] ddram_dqs_p[*] ddram_dqs_n[*]}}]]"
        )
        platform.add_platform_command(
            "set_property BITSTREAM.STARTUP.MATCH_CYCLE 6 [current_design]"
        )
