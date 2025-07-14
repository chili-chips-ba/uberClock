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
from litex.soc.interconnect.csr_eventmanager import EventManager, EventSourcePulse

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
repository_dir = "/home/ahmed/ws/uberClock/" # Absolute path to the root of your cloned repo
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

        # use a 2% tolerance on generated clocks
        margin = 2e-2
        self.pll = pll = S7PLL(speedgrade=-2)
        self.comb += pll.reset.eq(self.rst)
        pll.register_clkin(platform.request("clk200"), 200e6)
        pll.create_clkout(self.cd_sys, sys_clk_freq, margin=margin)
        platform.add_false_path_constraints(self.cd_sys.clk, pll.clkin)

        if with_dram:
            pll.create_clkout(self.cd_sys4x,     4*sys_clk_freq, margin=margin)
            pll.create_clkout(self.cd_sys4x_dqs, 4*sys_clk_freq, phase=90, margin=margin)
            pll.create_clkout(self.cd_idelay,    200e6, margin=margin)
            self.idelayctrl = S7IDELAYCTRL(self.cd_idelay)


# -------------------------------------------------------------------------
#  BaseSoC with optional CORDIC, DAC & ICD wiring
# -------------------------------------------------------------------------
class BaseSoC(SoCCore):
    def __init__(self, toolchain="vivado", sys_clk_freq=200e6,
                 with_hdmi                = False,
                 with_ethernet            = False,
                 with_etherbone           = False,
                 with_spi_flash           = False,
                 with_led_chaser          = False,
                 with_sdcard              = False,
                 with_spi_sdcard          = False,
                 with_pcie                = False,
                 with_video_terminal      = False,
                 with_video_framebuffer   = False,
                 with_video_colorbars     = False,
                 with_ledmem              = False,
                 with_uberclock           = False,
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
                    ip_address = "192.168.1.123",
                    mac_address=0x0200000000AB
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

        # ---------------------------------------------------------------------
        #  Uberclock
        # ---------------------------------------------------------------------
        if with_uberclock:
            self._add_uberclock(verilog_dir, sys_clk_freq)

    def _add_uberclock(self, verilog_dir, sys_clk_freq):
        files = [
            "adc/adc.v",     "dac/dac.v",
            "filters/cic.v", "filters/cic_comp_down_mac.v",
            "filters/comp_down_coeffs.mem",
            "filters/hb_down_mac.v","filters/hb_down_coeffs.mem",
            "filters/downsamplerFilter.v",
            "filters/upsamplerFilter.v","filters/hb_up_mac.v",
            "filters/coeffs.mem","filters/cic_comp_up_mac.v",
            "filters/coeffs_comp.mem","filters/cic_int.v",
            "uberclock/uberclock.v",
            "cordic/cordic_pre_rotate.v","cordic/cordic_pipeline_stage.v",
            "cordic/cordic_round.v","cordic/cordic.v",
            "cordic/cordic_logic.v","cordic/gain_and_saturate.v",
            "cordic16/cordic16.v","cordic16/cordic_pre_rotate_16.v",
        ]
        for fn in files:
            self.platform.add_source(f"{verilog_dir}/{fn}")

        self._input_select     = CSRStorage(1,  description="0=ADC,1=NCO")
        self._output_select_ch1    = CSRStorage(2,  description="DAC CH1 output selector")
        self._output_select_ch2    = CSRStorage(2,  description="DAC CH1 output selector")
        self._phase_inc_nco    = CSRStorage(19, description="NCO phase increment")
        self._phase_inc_down   = CSRStorage(19, description="Downconversion phase inc")
        self._gain1            = CSRStorage(32, description="Gain1 (Q format)")
        self._gain2            = CSRStorage(32, description="Gain2 (Q format)")
        self._upsampler_input_x  = CSRStorage(16, description="Upsampler input x")
        self._upsampler_input_y  = CSRStorage(16, description="Upsampler input y")

        self._downsampled_data_x = CSRStatus(16, description="Downsampled data x")
        self._downsampled_data_y = CSRStatus(16, description="Downsampled data y")

        input_select    = self._input_select.storage
        output_select_ch1   = self._output_select_ch1.storage
        output_select_ch2   = self._output_select_ch2.storage
        phase_inc_nco   = self._phase_inc_nco.storage
        phase_inc_down  = self._phase_inc_down.storage
        gain1, gain2    = self._gain1.storage, self._gain2.storage
        upsampler_input_x = self._upsampler_input_x.storage
        upsampler_input_y = self._upsampler_input_y.storage

        ce_down = Signal(name="ce_down")
        self.submodules.evm     = EventManager()
        self.evm.ce_down = EventSourcePulse(description="Downsample ready")
        self.evm.finalize()

        #self.add_csr("evm")
        self.irq.add("evm")

        dbg = {
            "nco_cos":        Signal(12),
            "nco_sin":        Signal(12),
            "phase_acc_down": Signal(19),
            "x_downconverted":Signal(12),
            "y_downconverted":Signal(12),
            "downsampled_x":  Signal(16),
            "downsampled_y":  Signal(16),
            "upsampled_x":    Signal(16),
            "upsampled_y":    Signal(16),
            "phase_inv":      Signal(23),
            "x_upconverted":  Signal(16),
            "y_upconverted":  Signal(16),
            "ce_down_x":      Signal(),
            "ce_down_y":      Signal(),
            "ce_up_x":        Signal(),
            "cic_ce_x":       Signal(),
            "comp_ce_x":      Signal(),
            "hb_ce_x":        Signal(),
            "cic_out_x":      Signal(12),
            "comp_out_x":     Signal(16),
        }

        self.specials += Instance(
            "uberclock",
            i_sys_clk  = ClockSignal("sys"),
            i_rst      = ResetSignal("sys"),

            # ADC
            o_adc_clk_ch0  = self.platform.request("adc_clk_ch0"),
            o_adc_clk_ch1  = self.platform.request("adc_clk_ch1"),
            i_adc_data_ch0 = self.platform.request("adc_data_ch0"),
            i_adc_data_ch1 = self.platform.request("adc_data_ch1"),

            # DAC
            o_da1_clk  = self.platform.request("da1_clk", 0),
            o_da1_wrt  = self.platform.request("da1_wrt", 0),
            o_da1_data = self.platform.request("da1_data",0),
            o_da2_clk  = self.platform.request("da2_clk", 0),
            o_da2_wrt  = self.platform.request("da2_wrt", 0),
            o_da2_data = self.platform.request("da2_data",0),

            # CSR inputs
            i_input_select    = input_select,
            i_output_select_ch1   = output_select_ch1,
            i_output_select_ch2   = output_select_ch2,
            i_phase_inc_nco   = phase_inc_nco,
            i_phase_inc_down  = phase_inc_down,
            i_gain1           = gain1,
            i_gain2           = gain2,
            i_upsampler_input_x = upsampler_input_x,
            i_upsampler_input_y = upsampler_input_y,

            # CSR outputs + event
            o_downsampled_data_x = self._downsampled_data_x.status,
            o_downsampled_data_y = self._downsampled_data_y.status,

            o_ce_down          = ce_down,

            # debug outputs (unpack the dict)
            **{f"o_dbg_{name}": sig for name, sig in dbg.items()}
        )

        self.sync += If(ce_down, self.evm.ce_down.trigger.eq(1))
        self.comb += self._downsampled_data_x.status.eq(dbg["downsampled_x"])
        self.comb += self._downsampled_data_y.status.eq(dbg["downsampled_y"])



        probes = (
            list(dbg.values()) +                              # all the internal debug nets
            [phase_inc_nco, phase_inc_down,                   # CSR knobs
             input_select, output_select_ch1, output_select_ch1,
             gain1, gain2,
             ce_down,
             upsampler_input_x,
             upsampler_input_y,
             self._downsampled_data_x.status,
             self._downsampled_data_y.status,
            ]
        )

        self.submodules.analyzer = LiteScopeAnalyzer(
            probes,
            depth        = 16384,
            clock_domain = "sys",
            samplerate   = sys_clk_freq
        )
        self.add_csr("analyzer")

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
    parser.add_argument("--with-uberclock", action="store_true",
        help="Instantiate Uberclock")

    args = parser.parse_args()

    soc = BaseSoC(
        toolchain                = args.toolchain,
        sys_clk_freq             = args.sys_clk_freq,
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
