#!/usr/bin/env python3

#
# This file is part of LiteX-Boards.
#
# Copyright (c) 2022 Yonggang Liu <ggang.liu@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause
#

from migen import *
from litex.gen import *

from litex_boards.platforms import alinx_ax7203

from litex.soc.interconnect import axi, wishbone
from litex.soc.interconnect.axi import AXILiteInterface
from litex.soc.interconnect.csr import CSRStorage, CSRStatus

from litex.soc.integration.soc_core import *
from litex.soc.integration.builder import *

from litex.soc.cores.clock import *
from litex.soc.cores.led import LedChaser
from litex.soc.cores.video import VideoS7HDMIPHY

from litedram.modules import MT41J256M16
from litedram.phy import s7ddrphy
from litedram.core.controller import ControllerSettings

from liteeth.phy.s7rgmii import LiteEthPHYRGMII

from litescope import LiteScopeAnalyzer

# NOTE: Change this accordingly!
repository_dir = "/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock" # Absolute path to the root of your cloned repo
verilog_dir = repository_dir + "/2.soc-litex/1.hw"

# -------------------------------------------------------------------------
#  Clock Reset Generator
# -------------------------------------------------------------------------
class _CRG(LiteXModule):
    def __init__(self, platform, sys_clk_freq, with_dram=True):
        self.rst    = Signal()
        self.cd_sys = ClockDomain()
        if with_dram:
            self.cd_sys4x     = ClockDomain()
            self.cd_sys4x_dqs = ClockDomain()
            self.cd_idelay    = ClockDomain()

        self.pll = pll = S7PLL(speedgrade=-2)
        self.comb += pll.reset.eq(self.rst)
        pll.register_clkin(platform.request("clk200"), 200e6)
        pll.create_clkout(self.cd_sys, sys_clk_freq)
        platform.add_false_path_constraints(self.cd_sys.clk, pll.clkin)
        if with_dram:
            pll.create_clkout(self.cd_sys4x,     4*sys_clk_freq)
            pll.create_clkout(self.cd_sys4x_dqs, 4*sys_clk_freq, phase=90)
            pll.create_clkout(self.cd_idelay,    200e6)
            self.idelayctrl = S7IDELAYCTRL(self.cd_idelay)


# -------------------------------------------------------------------------
#  BaseSoC with optional CORDIC, DAC & ICD wiring
# -------------------------------------------------------------------------
class BaseSoC(SoCCore):
    def __init__(self, toolchain="vivado", sys_clk_freq=200e6,
                 with_hdmi              = False,
                 with_ethernet          = False,
                 with_etherbone         = False,
                 with_spi_flash         = False,
                 with_led_chaser        = False,
                 with_sdcard            = False,
                 with_spi_sdcard        = False,
                 with_pcie              = False,
                 with_video_terminal    = False,
                 with_video_framebuffer = False,
                 with_video_colorbars   = False,
                 with_ledmem            = False,
                 with_cordic            = False,
                 with_dac               = False,
                 with_cordic_dac        = False,
                 with_adc_dac           = False,
                 with_icd               = False,
                 **kwargs):
        platform = alinx_ax7203.Platform(toolchain=toolchain)

        # CRG
        with_dram = (kwargs.get("integrated_main_ram_size", 0) == 0)
        self.crg = _CRG(platform, sys_clk_freq, with_dram)

        # SoCCore init
        kwargs["uart_name"] = "serial"
        SoCCore.__init__(self, platform, sys_clk_freq,
                         ident="LiteX SoC on Alinx AX7203",
                         **kwargs)

        self._heartbeat = Signal(24)
        self.sync += self._heartbeat.eq(self._heartbeat + 1)
        leds = Cat(*platform.request_all("user_led"))
        self.comb += leds[0].eq(self._heartbeat[23])

        # DDR3 SDRAM
        if not self.integrated_main_ram_size:
            self.ddrphy = s7ddrphy.A7DDRPHY(
                platform.request("ddram"),
                memtype="DDR3", nphases=4,
                sys_clk_freq=sys_clk_freq
            )
            cs = ControllerSettings()
            cs.auto_precharge = False
            self.add_sdram("sdram",
                phy=self.ddrphy,
                module=MT41J256M16(sys_clk_freq, "1:4"),
                size=0x40000000,
                controller_settings=cs,
                origin=self.mem_map["main_ram"],
                l2_cache_size=kwargs.get("l2_size", 8192)
            )

        # Ethernet / Etherbone
        if with_ethernet or with_etherbone:
            self.ethphy = LiteEthPHYRGMII(
                clock_pads=platform.request("eth_clocks"),
                pads      =platform.request("eth")
            )
            if with_ethernet:
                self.add_ethernet(phy=self.ethphy)
            if with_etherbone:
                self.add_etherbone(phy=self.ethphy,
                    ip_address = "192.168.1.50",
                    mac_address=0x10e2d5_000001
                )

        # SPI Flash
        if with_spi_flash:
            from litespi.modules import N25Q128
            from litespi.opcodes import SpiNorFlashOpCodes as Codes
            self.add_spi_flash(
                mode="4x",
                module=N25Q128(Codes.READ_1_1_1),
                rate="1:2",
                with_master=True
            )

        # HDMI (video terminal, framebuffer, colorbars)
        if with_hdmi and (with_video_colorbars
                          or with_video_framebuffer
                          or with_video_terminal):
            self.videophy = VideoS7HDMIPHY(
                platform.request("hdmi_out"),
                clock_domain="hdmi"
            )
            if with_video_colorbars:
                self.add_video_colorbars(
                    phy=self.videophy,
                    timings="640x480@60Hz",
                    clock_domain="hdmi"
                )
            if with_video_terminal:
                self.add_video_terminal(
                    phy=self.videophy,
                    timings="640x480@60Hz",
                    clock_domain="hdmi"
                )
            if with_video_framebuffer:
                self.add_video_framebuffer(
                    phy=self.videophy,
                    timings="640x480@60Hz",
                    clock_domain="hdmi"
                )

        # PCIe
        if with_pcie:
            self.pcie_phy = S7PCIEPHY(
                platform,
                platform.request("pcie_x4"),
                data_width=128,
                bar0_size=0x20000
            )
            self.add_pcie(phy=self.pcie_phy, ndmas=1)

        # SDCard
        if with_sdcard:
            self.add_sdcard()
        if with_spi_sdcard:
            self.add_spi_sdcard()

        # LED Chaser
        if with_led_chaser:
            self.leds = LedChaser(
                pads         = platform.request_all("user_led"),
                sys_clk_freq = sys_clk_freq
            )

        # CORDIC
        if with_cordic:

            for filename in [
                "cordic/cordic_pre_rotate.v",
                "cordic/cordic_pipeline_stage.v",
                "cordic/cordic_round.v",
                "cordic/cordic.v",
                "cordic/cordic_top.v",
            ]:
                full_path = f"{verilog_dir}/{filename}"
                self.platform.add_source(full_path)

            self._cordic_cos = CSRStatus(12, description="CORDIC NCO cosine out")
            self._cordic_sin = CSRStatus(12, description="CORDIC NCO sine out")
            self._cordic_aux = CSRStatus(1,   description="CORDIC NCO valid (tied high)")

            i_ce   = Signal(reset=1)
            cos_sig = Signal(12)
            sin_sig = Signal(12)

            cordic_inst = Instance("cordic_top",
                # clock/reset/enable
                i_clk    = ClockSignal("sys"),
                i_reset  = ResetSignal("sys"),
                i_ce     = i_ce,
                # outputs: wire signed [11:0] o_cos, o_sin
                o_o_cos    = cos_sig,
                o_o_sin    = sin_sig
            )
            self.specials += cordic_inst

            self.comb += [
                self._cordic_cos .status.eq(cos_sig),
                self._cordic_sin .status.eq(sin_sig),
                self._cordic_aux .status.eq(1),
            ]

            analyzer_signals = [
                i_ce,
                cos_sig,
                sin_sig,
            ]
            self.submodules.analyzer = LiteScopeAnalyzer(
                analyzer_signals,
                depth        = 512,
                clock_domain = "sys",
                samplerate   = sys_clk_freq,
                csr_csv      = "analyzer.csv"
            )
            self.add_csr("analyzer")

        # Stand-alone DAC
        if with_dac:

            filename = "dac/dac_control.v"
            self.platform.add_source(f"{verilog_dir}/{filename}")

            self._dac1_data   = CSRStorage(14, description="DAC1 data")
            self._dac1_wrt_en = CSRStorage(1,  description="DAC1 write enable")
            self._dac2_data   = CSRStorage(14, description="DAC2 data")
            self._dac2_wrt_en = CSRStorage(1,  description="DAC2 write enable")

            data1   = self._dac1_data.storage
            wrt1_en = self._dac1_wrt_en.storage
            data2   = self._dac2_data.storage
            wrt2_en = self._dac2_wrt_en.storage

            self.specials += Instance("dac_control",
                i_sys_clk  = ClockSignal("sys"),
                i_rst_n    = ResetSignal("sys"),
                i_data1    = data1,
                i_wrt1_en  = wrt1_en,
                i_data2    = data2,
                i_wrt2_en  = wrt2_en,
                o_da1_clk  = platform.request("da1_clk"),
                o_da1_wrt  = platform.request("da1_wrt"),
                o_da1_data = platform.request("da1_data"),
                o_da2_clk  = platform.request("da2_clk"),
                o_da2_wrt  = platform.request("da2_wrt"),
                o_da2_data = platform.request("da2_data"),
            )

        # ---------------------------------------------------------------------
        # ICD-defined register banks (8–15)
        # ---------------------------------------------------------------------
        if with_icd:
            # GLOBAL_CTRL
            self._bypass_en   = CSRStorage(1, description="0 = CPU processing chain; 1 = raw bypass")
            self._mux_sel     = CSRStorage(3, description="Select frequency path (0…4)")
            self._method_sel  = CSRStorage(3, description="Select algorithm variant (1…5)")
            self.add_csr("bypass_en")
            self.add_csr("mux_sel")
            self.add_csr("method_sel")

            # TX_PATH
            self._cordic_tx_phase = CSRStorage(19, description="TX-CORDIC phase word")
            self.add_csr("cordic_tx_phase")

            # RX_PATH
            self._cordic_rx_phase   = CSRStorage(19, description="RX-CORDIC phase word")
            self.add_csr("cordic_rx_phase")

            # GAIN
            self._gain0 = CSRStorage(12, description="Gain for path 0")
            self._gain1 = CSRStorage(12, description="Gain for path 1")
            self._gain2 = CSRStorage(12, description="Gain for path 2")
            self._gain3 = CSRStorage(12, description="Gain for path 3")
            self._gain4 = CSRStorage(12, description="Gain for path 4")
            self.add_csr("gain0")
            self.add_csr("gain1")
            self.add_csr("gain2")
            self.add_csr("gain3")
            self.add_csr("gain4")

            # DEBUG_HS
            self._hs_dbg_addr  = CSRStorage(16, description="High-speed debug RAM address")
            self._hs_dbg_wdata = CSRStorage(32, description="High-speed debug RAM write data")
            self._hs_dbg_rdata = CSRStatus(32,  description="High-speed debug RAM read data")
            self.add_csr("hs_dbg_addr")
            self.add_csr("hs_dbg_wdata")
            self.add_csr("hs_dbg_rdata")

            # DEBUG_LS
            self._ls_dbg_addr  = CSRStorage(16, description="Low-speed debug RAM address")
            self._ls_dbg_wdata = CSRStorage(32, description="Low-speed debug RAM write data")
            self._ls_dbg_rdata = CSRStatus(32,  description="Low-speed debug RAM read data")
            self.add_csr("ls_dbg_addr")
            self.add_csr("ls_dbg_wdata")
            self.add_csr("ls_dbg_rdata")

        # CORDIC + DAC (fused)
        if with_cordic_dac:

            for filename in [
                "cordic/cordic_pre_rotate.v",
                "cordic/cordic_pipeline_stage.v",
                "cordic/cordic_round.v",
                "cordic/cordic.v",
                "dac/dac.v",
                "cordic-dac/cordic_dac.v",
            ]:
                self.platform.add_source(f"{verilog_dir}/{filename}")

            self._phase_inc = CSRStorage(19, description="CORDIC_DAC phase increment")
            phase_inc = self._phase_inc.storage

            self.specials += Instance(
                "cordic_dac",
                i_sys_clk    = ClockSignal("sys"),
                i_rst_n      = ~ResetSignal("sys"),
                i_phase_inc  = phase_inc,
                o_da1_clk    = platform.request("da1_clk"),
                o_da1_wrt    = platform.request("da1_wrt"),
                o_da1_data   = platform.request("da1_data"),
                o_da2_clk    = platform.request("da2_clk"),
                o_da2_wrt    = platform.request("da2_wrt"),
                o_da2_data   = platform.request("da2_data"),
            )

        # ---------------------------------------------------------------------
        #  ADC + DAC (fused)
        # ---------------------------------------------------------------------
        if with_adc_dac:
            for fname in ["adc/adc.v", "dac/dac.v", "adc-dac/adc_dac.v"]:
                self.platform.add_source(f"{verilog_dir}/{fname}")

            adc_debug_ch0 = Signal(12, name="adc_debug_ch0")
            adc_debug_ch1 = Signal(12, name="adc_debug_ch1")
            dac_debug_1   = Signal(14, name="dac_debug_1")
            dac_debug_2   = Signal(14, name="dac_debug_2")
            self.specials += Instance(
                "adc_dac",
                # Clocks / reset
                i_sys_clk      = ClockSignal("sys"),
                i_rst_n        = ResetSignal("sys"),

                # ADC ports
                o_adc_clk_ch0   = platform.request("adc_clk_ch0"),
                o_adc_clk_ch1   = platform.request("adc_clk_ch1"),
                i_adc_data_ch0  = platform.request("adc_data_ch0"),
                i_adc_data_ch1  = platform.request("adc_data_ch1"),
                o_debug_adc_data_ch0 = adc_debug_ch0,
                o_debug_adc_data_ch1 = adc_debug_ch1,


                # DAC ports
                o_da1_clk       = platform.request("da1_clk",  0),
                o_da1_wrt       = platform.request("da1_wrt",  0),
                o_da1_data      = platform.request("da1_data", 0),
                o_da2_clk       = platform.request("da2_clk",  0),
                o_da2_wrt       = platform.request("da2_wrt",  0),
                o_da2_data      = platform.request("da2_data", 0),
                o_debug_dac_data1 = dac_debug_1,
                o_debug_dac_data2 = dac_debug_2,
            )
            analyzer_signals = [
                 adc_debug_ch0, adc_debug_ch1,
                 dac_debug_1, dac_debug_2
            ]

            self.submodules.analyzer = LiteScopeAnalyzer(
                analyzer_signals,
                depth        = 1024,
                clock_domain = "sys"
            )
            self.add_csr("analyzer")


    # optionally export analyzer CSV
    # def do_exit(self, vns, **kwargs):
    #     self.analyzer.export_csv(vns, "analyzer.csv")


# Build --------------------------------------------------------------------------------------------
def main():
    from litex.build.parser import LiteXArgumentParser

    parser = LiteXArgumentParser(
        platform=alinx_ax7203.Platform,
        description="LiteX SoC on Alinx AX7203."
    )

    # standard targets
    parser.add_target_argument("--cable",        default="ft232",
        help="JTAG interface.")
    parser.add_target_argument("--sys-clk-freq", default=200e6, type=float,
        help="System clock frequency.")
    ethopts = parser.target_group.add_mutually_exclusive_group()
    ethopts.add_argument("--with-ethernet",  action="store_true",
        help="Enable Ethernet support.")
    ethopts.add_argument("--with-etherbone", action="store_true",
        help="Enable Etherbone support.")
    sdopts = parser.target_group.add_mutually_exclusive_group()
    sdopts.add_argument("--with-spi-sdcard", action="store_true",
        help="Enable SPI-mode SDCard support.")
    sdopts.add_argument("--with-sdcard",     action="store_true",
        help="Enable SDCard support.")
    parser.add_argument("--with-pcie",       action="store_true",
        help="Enable PCIe")
    parser.add_argument("--with-hdmi",       action="store_true",
        help="Enable HDMI")
    parser.add_argument("--with-led-chaser", action="store_true",
        help="Enable LED chaser")
    viopts = parser.target_group.add_mutually_exclusive_group()
    viopts.add_argument("--with-video-terminal",    action="store_true",
        help="Enable Video Terminal (HDMI).")
    viopts.add_argument("--with-video-framebuffer", action="store_true",
        help="Enable Video Framebuffer (HDMI).")
    viopts.add_argument("--with-video-colorbars",   action="store_true",
        help="Enable Video Colorbars (HDMI).")
    parser.add_target_argument("--with-spi-flash", action="store_true",
        help="Enable SPI Flash (MMAPed).")
    parser.add_argument("--with-cordic", action="store_true",
        help="Instantiate CORDIC accelerator on CSR")
    parser.add_argument("--with-dac", action="store_true",
        help="Instantiate standalone DAC-only module")
    parser.add_argument("--with-cordic-dac", action="store_true",
        help="Instantiate CORDIC_DAC module")
    parser.add_argument("--with-icd", action="store_true",
        help="Add ICD register banks (GLOBAL_CTRL, TX_PATH, ..., SD_CARD)")
    parser.add_argument("--with-adc-dac", action="store_true",
        help="Instantiate ADC-DAC interface")

    args = parser.parse_args()

    soc = BaseSoC(
        toolchain              = args.toolchain,
        sys_clk_freq           = args.sys_clk_freq,
        with_ethernet          = args.with_ethernet,
        with_etherbone         = args.with_etherbone,
        with_spi_flash         = args.with_spi_flash,
        with_sdcard            = args.with_sdcard,
        with_spi_sdcard        = args.with_spi_sdcard,
        with_pcie              = args.with_pcie,
        with_hdmi              = args.with_hdmi,
        with_led_chaser        = args.with_led_chaser,
        with_video_terminal    = args.with_video_terminal,
        with_video_framebuffer = args.with_video_framebuffer,
        with_video_colorbars   = args.with_video_colorbars,
        with_cordic            = args.with_cordic,
        with_dac               = args.with_dac,
        with_cordic_dac        = args.with_cordic_dac,
        with_icd               = args.with_icd,
        with_adc_dac           = args.with_adc_dac,
        **parser.soc_argdict
    )

    builder = Builder(soc, **parser.builder_argdict)
    if args.build:
        builder.build(**parser.toolchain_argdict)
    if args.load:
        prog = soc.platform.create_programmer(args.cable)
        prog.load_bitstream(
            builder.get_bitstream_filename(mode="sram")
        )


if __name__ == "__main__":
    main()
