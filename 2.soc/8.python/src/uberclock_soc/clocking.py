# uberclock_soc/clocking.py
#
# Clock / Reset Generation (CRG) for AX7203 UberClock SoC
# ======================================================
# Clock plan:
#   - Input clock: 200 MHz differential (clk200_p/n)
#   - pll0 (S7MMCM): generates:
#       * cd_sys        : SoC system domain (100 MHz)
#       * cd_ub_4x      : DDR / UberDDR3 high-speed domain (400 MHz)
#       * cd_ub_4x_dqs  : DDR phase-shifted clock (400 MHz, +90°)
#   - pll1 (S7MMCM): generates:
#       * cd_uc         : UberClock DSP domain, forced to exact 65.000 MHz
#   - cd_idelay: 200 MHz domain used for IDELAYCTRL reference clock
#

from migen import Signal, ClockDomain, Instance
from litex.gen import LiteXModule
from litex.soc.cores.clock import S7MMCM, S7IDELAYCTRL


class UberClockCRG(LiteXModule):
    """Clock/reset generation for the UberClock SoC on a 7-series FPGA (Artix-7)."""

    CLKIN_HZ     = 200e6
    SYS_CLK_HZ   = 100e6
    UC_CLK_HZ    = 65e6
    DDR4X_CLK_HZ = 400e6
    IDELAY_HZ    = 200e6

    MMCM_MARGIN  = 1e-6

    def __init__(self, platform, need_ddr_clks: bool = True):
        self.rst = Signal()

        self.locked0 = Signal()
        self.locked1 = Signal()

        # ----------------------------
        # Clock domains (MUST be named)
        # ----------------------------
        self.cd_sys       = ClockDomain("sys")
        self.cd_uc        = ClockDomain("uc")
        self.cd_ub_4x     = ClockDomain("ub_4x")
        self.cd_ub_4x_dqs = ClockDomain("ub_4x_dqs")
        self.cd_idelay    = ClockDomain("idelay")

        # ---------------------------------------------------------------------
        # Differential 200 MHz input buffer
        # ---------------------------------------------------------------------
        clk200_pads = platform.request("clk200")
        clk200_se   = Signal()  # single-ended 200 MHz after IBUFDS

        self.specials += Instance(
            "IBUFDS",
            i_I  = clk200_pads.p,
            i_IB = clk200_pads.n,
            o_O  = clk200_se,
        )

        # ---------------------------------------------------------------------
        # pll0: sys + DDR clocks
        # ---------------------------------------------------------------------
        self.pll0 = S7MMCM(speedgrade=-2)
        self.comb += self.pll0.reset.eq(self.rst)
        self.pll0.register_clkin(clk200_se, self.CLKIN_HZ)

        self.pll0.create_clkout(self.cd_sys, self.SYS_CLK_HZ, margin=self.MMCM_MARGIN)
        if need_ddr_clks:
            self.pll0.create_clkout(self.cd_ub_4x,     self.DDR4X_CLK_HZ,           margin=self.MMCM_MARGIN)
            self.pll0.create_clkout(self.cd_ub_4x_dqs, self.DDR4X_CLK_HZ, phase=90, margin=self.MMCM_MARGIN)

        self.comb += self.locked0.eq(self.pll0.locked)

        # ---------------------------------------------------------------------
        # pll1: uc clock
        # ---------------------------------------------------------------------
        self.pll1 = S7MMCM(speedgrade=-2)
        self.comb += self.pll1.reset.eq(self.rst)
        self.pll1.register_clkin(clk200_se, self.CLKIN_HZ)

        self.pll1.create_clkout(self.cd_uc, self.UC_CLK_HZ, margin=self.MMCM_MARGIN, with_reset=False)
        self.comb += self.locked1.eq(self.pll1.locked)

        # ---------------------------------------------------------------------
        # IDELAYCTRL reference clock (BUFG @ 200 MHz)
        # ---------------------------------------------------------------------
        clk200_bufg = Signal()
        self.specials += Instance("BUFG", i_I=clk200_se, o_O=clk200_bufg)
        self.comb += self.cd_idelay.clk.eq(clk200_bufg)

        self.idelayctrl = S7IDELAYCTRL(self.cd_idelay)

        # Constraints
        platform.add_false_path_constraints(self.cd_sys.clk, self.pll0.clkin)
        platform.add_false_path_constraints(self.cd_uc.clk,  self.pll1.clkin)
        if need_ddr_clks:
            platform.add_false_path_constraints(self.cd_ub_4x.clk,     self.pll0.clkin)
            platform.add_false_path_constraints(self.cd_ub_4x_dqs.clk, self.pll0.clkin)

    def do_finalize(self):
        """Force pll1 ratios to guarantee 65.000 MHz for the UberClock domain."""
        super().do_finalize()

        # Force pll1 exact 65.000 MHz:
        #   VCO = 200 * 13 / 2 = 1300 MHz (valid for -2: 600..1440 MHz)
        #   UC  = 1300 / 20    = 65.000 MHz
        p = self.pll1.params

        # VCO ratio
        p["p_DIVCLK_DIVIDE"]   = 2
        p["p_CLKFBOUT_MULT_F"] = 13.0
        p["p_CLKFBOUT_PHASE"]  = 0.0

        # Output divider (CLKOUT0 usually maps to first create_clkout)
        p["p_CLKOUT0_DIVIDE_F"]   = 20.0
        p["p_CLKOUT0_PHASE"]      = 0.0
        p["p_CLKOUT0_DUTY_CYCLE"] = 0.5

        # Safety if mapped to CLKOUT1
        p["p_CLKOUT1_DIVIDE"]     = 20
        p["p_CLKOUT1_PHASE"]      = 0.0
        p["p_CLKOUT1_DUTY_CYCLE"] = 0.5
