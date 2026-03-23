# wishbone.py
#
# Wishbone Adapter Blocks for UberDDR3 Integration
# ===============================================
# Glue modules that adapt LiteX/Migen Wishbone busses to the
# pipelined wishbone used by the external UberDDR3 controller RTL.
#

from __future__ import annotations

from migen import *
from litex.gen import *
from litex.soc.interconnect import wishbone as wb


# =============================================================================
#           Classic Wishbone -> Pipelined Wishbone Bridge
# =============================================================================
class ClassicToPipelinedWishboneBridge(LiteXModule):
    """
    Wraps the Verilog module `wbc2pipeline.v` to convert a classic Wishbone bus
    (LiteX-facing) into a pipelined Wishbone bus (UberDDR3-facing).

    Interfaces:
      - `self.s`     : classic Wishbone *slave* interface (connects to SoC bus).
      - `m_*` signals: pipelined Wishbone *master* raw signals (to downstream).
    """

    def __init__(self, data_width: int = 128, adr_width: int = 27, clock_domain: str = "sys"):
        DATA_WIDTH   = int(data_width)
        ADDR_WIDTH   = int(adr_width)
        SEL_WIDTH    = DATA_WIDTH // 8
        CLOCK_DOMAIN = str(clock_domain)

        assert DATA_WIDTH % 8 == 0, "Wishbone data_width must be a multiple of 8."

        # ---------------------------------------------------------------------
        # Upstream classic Wishbone interface (LiteX side)
        # ---------------------------------------------------------------------
        self.s = wb.Interface(data_width=DATA_WIDTH, adr_width=ADDR_WIDTH)

        # ---------------------------------------------------------------------
        # Downstream pipelined Wishbone raw signals (UberDDR3 side)
        # ---------------------------------------------------------------------
        self.m_cyc   = Signal()              # pipelined WB cycle valid
        self.m_stb   = Signal()              # pipelined WB strobe/request
        self.m_we    = Signal()              # write enable
        self.m_adr   = Signal(ADDR_WIDTH)    # word address
        self.m_dat_w = Signal(DATA_WIDTH)    # write data
        self.m_sel   = Signal(SEL_WIDTH)     # byte enables
        self.m_stall = Signal()              # backpressure from downstream
        self.m_ack   = Signal()              # acknowledge from downstream
        self.m_dat_r = Signal(DATA_WIDTH)    # read data
        self.m_err   = Signal()              # error

        try:
            s_cti, s_bte = self.s.cti, self.s.bte
        except AttributeError:
            s_cti = Signal(3, reset=0)
            s_bte = Signal(2, reset=0)

        # ---------------------------------------------------------------------
        # Verilog instance
        # ---------------------------------------------------------------------
        self.specials += Instance(
            "wbc2pipeline",
            p_AW=len(self.s.adr),
            p_DW=len(self.s.dat_w),

            i_i_clk   = ClockSignal(CLOCK_DOMAIN),
            i_i_reset = ResetSignal(CLOCK_DOMAIN),

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
            i_i_merr   = self.m_err,
        )


# =============================================================================
#              2-Master / 1-Slave Pipelined Wishbone Crossbar
# =============================================================================
class PipelinedWishboneXbar2M1S(LiteXModule):
    """
    Wraps the Verilog module `wbxbar.v` configured as:
      - NM = 2 masters
      - NS = 1 slave

    Intended topology for UberDDR3:
      - Master 0: CPU access path (classic -> converter -> c2p bridge)
      - Master 1: DMA engine (zipdma_s2mm)
      - Slave  0: DDR3 controller (ddr3_top)
    """

    def __init__(self, AW: int, DW: int, clock_domain: str = "sys"):

        ADDR_WIDTH   = int(AW)
        DATA_WIDTH   = int(DW)
        SEL_WIDTH    = DATA_WIDTH // 8
        CLOCK_DOMAIN = str(clock_domain)

        assert DATA_WIDTH % 8 == 0, "Wishbone data_width must be a multiple of 8."

        # ---------------------------------------------------------------------
        # Master[0] (CPU path)
        # ---------------------------------------------------------------------
        self.m0_cyc   = Signal()
        self.m0_stb   = Signal()
        self.m0_we    = Signal()
        self.m0_adr   = Signal(ADDR_WIDTH)
        self.m0_dat_w = Signal(DATA_WIDTH)
        self.m0_sel   = Signal(SEL_WIDTH)
        self.m0_stall = Signal()
        self.m0_ack   = Signal()
        self.m0_dat_r = Signal(DATA_WIDTH)
        self.m0_err   = Signal()

        # ---------------------------------------------------------------------
        # Master[1] (DMA path)
        # ---------------------------------------------------------------------
        self.m1_cyc   = Signal()
        self.m1_stb   = Signal()
        self.m1_we    = Signal()
        self.m1_adr   = Signal(ADDR_WIDTH)
        self.m1_dat_w = Signal(DATA_WIDTH)
        self.m1_sel   = Signal(SEL_WIDTH)
        self.m1_stall = Signal()
        self.m1_ack   = Signal()
        self.m1_dat_r = Signal(DATA_WIDTH)
        self.m1_err   = Signal()

        # ---------------------------------------------------------------------
        # Slave[0] (DDR controller)
        # ---------------------------------------------------------------------
        self.s_cyc   = Signal()
        self.s_stb   = Signal()
        self.s_we    = Signal()
        self.s_adr   = Signal(ADDR_WIDTH)
        self.s_dat_w = Signal(DATA_WIDTH)
        self.s_sel   = Signal(SEL_WIDTH)
        self.s_stall = Signal()
        self.s_ack   = Signal()
        self.s_dat_r = Signal(DATA_WIDTH)
        self.s_err   = Signal()

        # ---------------------------------------------------------------------
        # Verilog instance
        # ---------------------------------------------------------------------
        self.specials += Instance(
            "wbxbar",
            p_NM=2,
            p_NS=1,
            p_AW=ADDR_WIDTH,
            p_DW=DATA_WIDTH,

            # Controller options (keep defaults conservative)
            p_SLAVE_MASK=0,
            p_LGMAXBURST=6,
            p_OPT_TIMEOUT=0,
            p_OPT_STARVATION_TIMEOUT=0,
            p_OPT_DBLBUFFER=0,
            p_OPT_LOWPOWER=1,

            i_i_clk   = ClockSignal(CLOCK_DOMAIN),
            i_i_reset = ResetSignal(CLOCK_DOMAIN),

            # Masters packed as [M0, M1]
            i_i_mcyc  = Cat(self.m0_cyc,  self.m1_cyc),
            i_i_mstb  = Cat(self.m0_stb,  self.m1_stb),
            i_i_mwe   = Cat(self.m0_we,   self.m1_we),
            i_i_maddr = Cat(self.m0_adr,  self.m1_adr),
            i_i_mdata = Cat(self.m0_dat_w, self.m1_dat_w),
            i_i_msel  = Cat(self.m0_sel,  self.m1_sel),

            o_o_mstall = Cat(self.m0_stall, self.m1_stall),
            o_o_mack   = Cat(self.m0_ack,   self.m1_ack),
            o_o_mdata  = Cat(self.m0_dat_r, self.m1_dat_r),
            o_o_merr   = Cat(self.m0_err,   self.m1_err),

            # Single slave packed (NS=1)
            o_o_scyc  = self.s_cyc,
            o_o_sstb  = self.s_stb,
            o_o_swe   = self.s_we,
            o_o_saddr = self.s_adr,
            o_o_sdata = self.s_dat_w,
            o_o_ssel  = self.s_sel,

            i_i_sstall = self.s_stall,
            i_i_sack   = self.s_ack,
            i_i_sdata  = self.s_dat_r,
            i_i_serr   = self.s_err,
        )
