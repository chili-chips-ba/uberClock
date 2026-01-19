#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
uberclock_soc/soc.py

Top-level LiteX SoC assembly for the Alinx AX7203 platform.

This module is the *integration layer* that ties together:
  - Clock/reset generation (100 MHz sys + exact 65 MHz uc + optional DDR clocks)
  - Optional standard LiteDRAM main RAM (LiteX SDRAM controller path)
  - Optional UberDDR3 side-memory (custom DDR3 controller + DMA/S2MM path)
  - Optional LiteEth Ethernet / Etherbone
  - Optional HDMI output (LiteX Video pipeline)
  - Optional UberClock DSP core (Verilog block in the uc clock domain)

Design intent:
  - Keep the target wrapper script tiny (it should just call build_main()).
  - Keep most “feature wiring” here, while block internals live in their own files
    (clocking.py, ubddr3.py, uberclock_core.py, etc.).
"""

from __future__ import annotations

from migen import *
from litex.gen import *

from litex_boards.platforms import alinx_ax7203

from litex.soc.integration.soc_core import SoCCore
from litex.soc.integration.soc import SoCRegion
from litex.soc.integration.builder import Builder

from litex.soc.cores.timer import Timer
from litex.soc.cores.led import LedChaser
from litex.soc.cores.video import VideoS7HDMIPHY

from liteeth.phy.s7rgmii import LiteEthPHYRGMII

from litedram.modules import MT41J256M16
from litedram.phy import s7ddrphy
from litedram.core.controller import ControllerSettings

from .clocking import UberClockCRG
from .ubddr3 import UberDDR3
from .uberclock_core import add_uberclock_fullrate

# =============================================================================
#                               BaseSoC
# =============================================================================
class BaseSoC(SoCCore):
    """
    AX7203 SoC with optional peripherals and optional UberClock/UberDDR3 blocks.

    Clock domains (via CRG):
      - sys      : CPU/CSR domain, default 100 MHz
      - uc       : UberClock domain, exact 65 MHz
      - ub_4x    : DDR PHY clock (optional), 400 MHz
      - ub_4x_dqs: DDR DQS 90° clock (optional), 400 MHz @ +90°
      - idelay   : IDELAYCTRL reference, 200 MHz

    Memory options:
      - Integrated LiteX main RAM (default 64 KiB)
      - Optional LiteDRAM (if integrated_main_ram_size == 0 AND not using UberDDR3)
      - Optional UberDDR3 as *side memory* mapped at 0xA0000000
    """

    # ---- Useful constants for readability / maintenance ----
    SYS_CLK_HZ_DEFAULT        = 100e6
    INTEGRATED_MAIN_RAM_BYTES = 64 * 1024

    UBDDR3_BASE               = 0xA000_0000
    UBDDR3_SIZE               = 0x1000_0000

    def __init__(
        self,
        toolchain: str = "vivado",
        *,
        # Platform features
        with_hdmi: bool = False,
        with_ethernet: bool = False,
        with_etherbone: bool = False,
        with_spi_flash: bool = False,
        with_led_chaser: bool = False,

        # Unused here but kept for compatibility with common target signatures
        with_sdcard: bool = False,
        with_spi_sdcard: bool = False,
        with_pcie: bool = False,
        with_video_terminal: bool = False,
        with_video_framebuffer: bool = False,
        with_video_colorbars: bool = False,

        # Custom blocks
        with_uberclock: bool = False,
        with_uberddr3: bool = False,
        **kwargs,
    ):
        # ---------------------------------------------------------------------
        # SoCCore defaults
        # ---------------------------------------------------------------------
        sys_clk_hz = float(kwargs.get("clk_freq", self.SYS_CLK_HZ_DEFAULT))

        kwargs.setdefault("integrated_main_ram_size", self.INTEGRATED_MAIN_RAM_BYTES)
        kwargs.setdefault("uart_name", "serial")

        platform = alinx_ax7203.Platform(toolchain=toolchain)

        # ---------------------------------------------------------------------
        # CRG (clocks/resets)
        # ---------------------------------------------------------------------
        # DDR clock outputs are needed if:
        #   - UberDDR3 is enabled, OR
        #   - LiteX is using external SDRAM (integrated RAM is disabled).
        need_ddr_clks = bool(with_uberddr3) or (int(kwargs.get("integrated_main_ram_size", 0)) == 0)
        self.submodules.crg = UberClockCRG(platform, need_ddr_clks=need_ddr_clks)

        # ---------------------------------------------------------------------
        # SoCCore init (CPU/CSR @ sys clock)
        # ---------------------------------------------------------------------
        SoCCore.__init__(
            self,
            platform,
            sys_clk_hz,
            ident="AX7203: sys@100MHz uc@65MHz optional UberDDR3 + UberClock",
            **kwargs,
        )

        # ---------------------------------------------------------------------
        # LEDs: define a local “leds bus” and add a heartbeat (LED0)
        # ---------------------------------------------------------------------
        leds = Cat(*platform.request_all("user_led"))
        hb = Signal(24)
        self.sync.sys += hb.eq(hb + 1)
        self.comb += leds[0].eq(hb[-1])  # heartbeat

        # ---------------------------------------------------------------------
        # Timer CSR (handy for firmware delays / profiling)
        # ---------------------------------------------------------------------
        self.submodules.timer1 = Timer()
        self.add_csr("timer1")

        # ---------------------------------------------------------------------
        # Standard LiteDRAM path (only when main RAM is external AND not UberDDR3)
        # ---------------------------------------------------------------------
        if (not self.integrated_main_ram_size) and (not with_uberddr3):
            self._add_litedram_main_ram(platform, sys_clk_hz)

        # ---------------------------------------------------------------------
        # UberDDR3 side-memory (custom controller)
        # ---------------------------------------------------------------------
        if with_uberddr3:
            self._add_ubddr3(platform, leds, sys_clk_hz)

        # ---------------------------------------------------------------------
        # Ethernet / Etherbone
        # ---------------------------------------------------------------------
        if with_ethernet or with_etherbone:
            self._add_ethernet(platform, with_ethernet=with_ethernet, with_etherbone=with_etherbone)

        # ---------------------------------------------------------------------
        # SPI Flash
        # ---------------------------------------------------------------------
        if with_spi_flash:
            self._add_spi_flash()

        # ---------------------------------------------------------------------
        # HDMI output
        # ---------------------------------------------------------------------
        if with_hdmi and (with_video_colorbars or with_video_framebuffer or with_video_terminal):
            self._add_hdmi(platform,
                           with_video_colorbars=with_video_colorbars,
                           with_video_terminal=with_video_terminal,
                           with_video_framebuffer=with_video_framebuffer)

        # ---------------------------------------------------------------------
        # LED chaser (optional)
        # ---------------------------------------------------------------------
        if with_led_chaser:
            self.leds = LedChaser(pads=platform.request_all("user_led"), sys_clk_freq=sys_clk_hz)

        # ---------------------------------------------------------------------
        # UberClock DSP integration (uc clock domain)
        # ---------------------------------------------------------------------
        if with_uberclock:
            add_uberclock_fullrate(self, leds=leds)

    # =========================================================================
    #                           Feature helpers
    # =========================================================================
    def _add_litedram_main_ram(self, platform, sys_clk_hz: float) -> None:
        """
        Add LiteX+LiteDRAM external DDR3 as main_ram.

        Used only when integrated main RAM is disabled and UberDDR3 is NOT enabled.
        """
        self.ddrphy = s7ddrphy.A7DDRPHY(
            platform.request("ddram"),
            memtype="DDR3",
            nphases=4,
            sys_clk_freq=sys_clk_hz,
        )

        cs = ControllerSettings()
        cs.auto_precharge = False

        self.add_sdram(
            name="sdram",
            phy=self.ddrphy,
            module=MT41J256M16(sys_clk_hz, "1:4"),
            size=0x4000_0000,
            controller_settings=cs,
            origin=self.mem_map["main_ram"],
            l2_cache_size=int(self.constants.get("L2_SIZE", 8192)) if hasattr(self, "constants") else 8192,
        )

    def _add_ubddr3(self, platform, leds, sys_clk_hz: float) -> None:
        """
        Add UberDDR3 as *side memory* mapped at UBDDR3_BASE.

        - Provides a Wishbone slave (ubddr3.wb) for CPU access to DDR.
        - Exposes DMA/S2MM CSRs for high-speed capture into DDR.
        """
        pads = platform.request("ddram")
        self.submodules.ubddr3 = UberDDR3(
            platform=platform,
            pads=pads,
            locked=self.crg.pll0.locked,
            SYS_CLK_HZ=sys_clk_hz,
            DDR_CK_HZ=400e6,
            ROW_BITS=15,
            COL_BITS=10,
            BA_BITS=3,
            BYTE_LANES=4,
            DUAL_RANK=0,
            SPEED_BIN=3,
            SDRAM_CAPACITY=5,
            DLL_OFF=0,
            ODELAY_SUPPORTED=0,
            BIST_MODE=0,
        )

        region = SoCRegion(origin=self.UBDDR3_BASE, size=self.UBDDR3_SIZE, cached=False, linker=False)
        self.bus.add_slave("ub_ram", self.ubddr3.wb, region)

        self.add_constant("UBDDR3_MEM_BASE", self.UBDDR3_BASE)
        self.add_csr("ubddr3")

        # LED1 shows DDR calibration done (useful sanity indicator)
        self.comb += leds[1].eq(self.ubddr3.calib_done.status)

    def _add_ethernet(self, platform, *, with_ethernet: bool, with_etherbone: bool) -> None:
        """
        Add LiteEth RGMII PHY and either Ethernet or Etherbone stack.
        """
        self.ethphy = LiteEthPHYRGMII(
            clock_pads=platform.request("eth_clocks"),
            pads=platform.request("eth"),
        )
        if with_ethernet:
            self.add_ethernet(phy=self.ethphy)
        if with_etherbone:
            self.add_etherbone(
                phy=self.ethphy,
                ip_address="192.168.0.123",
                mac_address=0x0200_0000_00AB,
            )

    def _add_spi_flash(self) -> None:
        """
        Add external SPI-NOR flash support (Quad mode).
        """
        from litespi.modules import N25Q128
        from litespi.opcodes import SpiNorFlashOpCodes as Codes

        self.add_spi_flash(
            mode="4x",
            module=N25Q128(Codes.READ_1_1_1),
            rate="1:2",
            with_master=True,
        )

    def _add_hdmi(
        self,
        platform,
        *,
        with_video_terminal: bool,
        with_video_framebuffer: bool,
        with_video_colorbars: bool,
    ) -> None:
        """
        Add HDMI PHY and optionally video sources.

        Notes:
          - 'hdmi' clock domain is created by LiteX video core integration.
        """
        self.videophy = VideoS7HDMIPHY(platform.request("hdmi_out"), clock_domain="hdmi")

        timings = "640x480@60Hz"
        if with_video_colorbars:
            self.add_video_colorbars(self.videophy, timings=timings, clock_domain="hdmi")
        if with_video_terminal:
            self.add_video_terminal(self.videophy, timings=timings, clock_domain="hdmi")
        if with_video_framebuffer:
            self.add_video_framebuffer(self.videophy, timings=timings, clock_domain="hdmi")


# =============================================================================
#                               CLI / build entry
# =============================================================================
def build_main() -> None:
    """
    Entry point used by your thin target wrapper script.

    Keeps argument parsing + Builder invocation in one place.
    """
    from litex.build.parser import LiteXArgumentParser

    parser = LiteXArgumentParser(
        platform=alinx_ax7203.Platform,
        description="AX7203: sys@100MHz, uc@65MHz, optional UberDDR3 + UberClock",
    )

    # Standard LiteX args
    parser.add_target_argument("--cable", default="ft232")
    parser.add_target_argument("--sys-clk-freq", default=BaseSoC.SYS_CLK_HZ_DEFAULT, type=float)

    eth = parser.target_group.add_mutually_exclusive_group()
    eth.add_argument("--with-ethernet", action="store_true")
    eth.add_argument("--with-etherbone", action="store_true")

    parser.add_argument("--with-hdmi", action="store_true")
    parser.add_argument("--with-led-chaser", action="store_true")

    parser.add_target_argument("--with-spi-flash", action="store_true")

    parser.add_argument("--with-uberclock", action="store_true")
    parser.add_argument("--with-uberddr3", action="store_true")

    args = parser.parse_args()

    soc = BaseSoC(
        toolchain=args.toolchain,
        with_ethernet=args.with_ethernet,
        with_etherbone=args.with_etherbone,
        with_spi_flash=args.with_spi_flash,
        with_hdmi=args.with_hdmi,
        with_led_chaser=args.with_led_chaser,
        with_uberclock=args.with_uberclock,
        with_uberddr3=args.with_uberddr3,
        **parser.soc_argdict,
    )

    builder = Builder(soc, **parser.builder_argdict)

    if args.build:
        builder.build(**parser.toolchain_argdict)

    if args.load:
        prog = soc.platform.create_programmer(args.cable)
        prog.load_bitstream(builder.get_bitstream_filename(mode="sram"))
