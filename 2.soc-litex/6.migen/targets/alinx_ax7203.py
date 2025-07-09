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
repository_dir = "/home/hamed/FPGA/chili-chips/uberclock-hub/uberClock/" # Absolute path to the root of your cloned repo
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

        margin = 2e-2 # 2% tolerance
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
                 with_cordic              = False,
                 with_dac                 = False,
                 with_cordic_dac          = False,
                 with_adc_dac             = False,
                 with_input_mux           = False,
                 with_adc_dsp_dac         = False,
                 with_adc_dsp_dac_nocpu   = False,
                 with_icd                 = False,
                 with_adc_cordic_dsp_dac  = False,
                 with_uberclock           = False,
                 with_cordic_dsp_dac      = False,
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
        #  CORDIC
        # ---------------------------------------------------------------------
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


        # ---------------------------------------------------------------------
        #  Standalone DAC
        # ---------------------------------------------------------------------
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

        # ---------------------------------------------------------------------
        #  CORDIC + DAC
        # ---------------------------------------------------------------------
        if with_cordic_dac:

            for filename in [
                "cordic/cordic_pre_rotate.v",
                "cordic/cordic_pipeline_stage.v",
                "cordic/cordic_round.v",
                "cordic/cordic.v",
                "cordic/cordic_logic.v",
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
        #  ADC + DAC
        # ---------------------------------------------------------------------
        if with_adc_dac:

            for filename in ["adc/adc.v", "dac/dac.v", "adc-dac/adc_dac.v"]:
                self.platform.add_source(f"{verilog_dir}/{filename}")


            debug_adc_ch0     = Signal(12)
            debug_adc_ch1     = Signal(12)
            debug_dac1_input  = Signal(14)
            debug_dac2_input  = Signal(14)
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

                # DAC ports
                o_da1_clk       = platform.request("da1_clk",  0),
                o_da1_wrt       = platform.request("da1_wrt",  0),
                o_da1_data      = platform.request("da1_data", 0),
                o_da2_clk       = platform.request("da2_clk",  0),
                o_da2_wrt       = platform.request("da2_wrt",  0),
                o_da2_data      = platform.request("da2_data", 0),

                o_debug_adc_ch0     = debug_adc_ch0,
                o_debug_adc_ch1     = debug_adc_ch1,
                o_debug_dac1_input  = debug_dac1_input,
                o_debug_dac2_input  = debug_dac2_input,
            )
            analyzer_signals = [
                debug_adc_ch0,
                debug_adc_ch1,
                debug_dac1_input,
                debug_dac2_input,
            ]

            self.submodules.analyzer = LiteScopeAnalyzer(
                analyzer_signals,
                depth        = 1024,
                clock_domain = "sys",
                samplerate   = sys_clk_freq,
                csr_csv      = "analyzer.csv"
            )
            self.add_csr("analyzer")

        # ---------------------------------------------------------------------
        #  Input Mux
        # ---------------------------------------------------------------------
        if with_input_mux:
            for filename in [
                "adc/adc.v",
                "dac/dac.v",
                "cordic/cordic_pre_rotate.v",
                "cordic/cordic_pipeline_stage.v",
                "cordic/cordic_round.v",
                "cordic/cordic.v",
                "cordic/cordic_logic.v",
                "input-mux/input_mux.v"
            ]:
                self.platform.add_source(f"{verilog_dir}/{filename}")

            self._phase_inc = CSRStorage(19, description="CORDIC_DAC phase increment")
            phase_inc = self._phase_inc.storage

            self._input_sw_reg = CSRStorage(1, description="Input Switch Register")
            input_sw_reg = self._input_sw_reg.storage


            debug_sin = Signal(14)
            debug_cos = Signal(14)
            phase_acc_out = Signal(19)
            cordic_aux_out = Signal()
            self.specials += Instance(
                "input_mux",
                # Clocks / reset
                i_sys_clk      = ClockSignal("sys"),
                i_rst_n        = ResetSignal("sys"),

                # CPU Inputs
                i_phase_inc    = phase_inc,
                i_input_sw_reg = input_sw_reg,

                # ADC ports
                o_adc_clk_ch0   = platform.request("adc_clk_ch0"),
                o_adc_clk_ch1   = platform.request("adc_clk_ch1"),
                i_adc_data_ch0  = platform.request("adc_data_ch0"),
                i_adc_data_ch1  = platform.request("adc_data_ch1"),

                # DAC ports
                o_da1_clk       = platform.request("da1_clk",  0),
                o_da1_wrt       = platform.request("da1_wrt",  0),
                o_da1_data      = platform.request("da1_data", 0),
                o_da2_clk       = platform.request("da2_clk",  0),
                o_da2_wrt       = platform.request("da2_wrt",  0),
                o_da2_data      = platform.request("da2_data", 0),

                o_debug_sin    = debug_sin,
                o_debug_cos    = debug_cos,
                o_phase_acc_out = phase_acc_out,
                o_cordic_aux_out = cordic_aux_out,
            )

            analyzer_signals =  [phase_inc, input_sw_reg, debug_sin, debug_cos, phase_acc_out, cordic_aux_out]

            self.submodules.analyzer = LiteScopeAnalyzer(
                analyzer_signals,
                depth        = 1024,
                clock_domain = "sys",
                samplerate   = sys_clk_freq,
                csr_csv      = "analyzer.csv"
            )
            self.add_csr("analyzer")

        # ---------------------------------------------------------------------
        #  ADC - DSP - DAC
        # ---------------------------------------------------------------------
        if with_adc_dsp_dac:
            for filename in [
                "adc/adc.v",
                "dac/dac.v",

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

                "adc-dsp-dac/adc_dsp_dac.v"
            ]:
                self.platform.add_source(f"{verilog_dir}/{filename}")

            self._downsampled = CSRStatus(16, description="Downsampled data from filter")
            self._upsampler_in = CSRStorage(16, description="Upsampler input to filter")
            self._upsampler_gain = CSRStorage(8, description="Gain for upsampler (signed, 8-bit)")

            downsampled_sig = Signal(16)
            upsampler_in_sig = Signal(16)

            # Debug signals
            debug_downsampledY = Signal(16)
            debug_upsampledY   = Signal(16)
            debug_ce_out_down  = Signal()
            debug_ce_out_up    = Signal()
            debug_adc_input    = Signal(16)

            self.specials += Instance(
                "adc_dsp_dac",
                # Clocks / reset
                i_sys_clk      = ClockSignal("sys"),
                i_rst_n        = ResetSignal("sys"),

                # ADC ports
                o_adc_clk_ch0   = platform.request("adc_clk_ch0"),
                o_adc_clk_ch1   = platform.request("adc_clk_ch1"),
                i_adc_data_ch0  = platform.request("adc_data_ch0"),
                i_adc_data_ch1  = platform.request("adc_data_ch1"),

                # DAC ports
                o_da1_clk       = platform.request("da1_clk",  0),
                o_da1_wrt       = platform.request("da1_wrt",  0),
                o_da1_data      = platform.request("da1_data", 0),
                o_da2_clk       = platform.request("da2_clk",  0),
                o_da2_wrt       = platform.request("da2_wrt",  0),
                o_da2_data      = platform.request("da2_data", 0),

                o_downsampledData = downsampled_sig,
                i_upsamplerInput  = upsampler_in_sig,
                i_gain = self._upsampler_gain.storage,

                o_debug_downsampledY = debug_downsampledY,
                o_debug_upsampledY   = debug_upsampledY,
                o_debug_ce_out_down  = debug_ce_out_down,
                o_debug_ce_out_up    = debug_ce_out_up,
                o_debug_adc_input    = debug_adc_input,
            )

            self.comb += [
                self._downsampled.status.eq(downsampled_sig),
                upsampler_in_sig.eq(self._upsampler_in.storage),
            ]


            analyzer_signals = [
                debug_downsampledY,
                debug_upsampledY,
                debug_ce_out_down,
                debug_ce_out_up,
                debug_adc_input
            ]
            self.submodules.analyzer = LiteScopeAnalyzer(
                analyzer_signals,
                depth        = 1024,
                clock_domain = "sys",
                samplerate   = sys_clk_freq,
                csr_csv      = "analyzer.csv"
            )
            self.add_csr("analyzer")


        # ---------------------------------------------------------------------
        #  ADC - CORDIC - DSP - DAC
        # ---------------------------------------------------------------------
        if with_adc_cordic_dsp_dac:
            for filename in [
                "adc/adc.v",
                "dac/dac.v",

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


                "adc_cordic_dsp_dac/adc_cordic_dsp_dac.v",


                "cordic/cordic_pre_rotate.v",
                "cordic/cordic_pipeline_stage.v",
                "cordic/cordic_round.v",
                "cordic/cordic.v",
                "cordic/cordic_logic.v",
                "cordic/gain_and_saturate.v",

                "cordic16/cordic16.v",
                # "cordic16/gain_and_saturate.v",
                # "cordic16/cordic_round.v",
                "cordic16/cordic_pre_rotate_16.v",
                # "cordic16/cordic_pipeline_stage.v",
            ]:
                self.platform.add_source(f"{verilog_dir}/{filename}")

            self._phase_inc = CSRStorage(19, description="CORDIC_DAC phase increment")
            phase_inc = self._phase_inc.storage

            self._output_select= CSRStorage(2, description="Output select for DACs")
            output_select = self._output_select.storage

            debug_filter_in         = Signal(12)
            debug_phase2            = Signal(19)
            debug_xval_downconverted = Signal(12)
            debug_yval_downconverted = Signal(12)
            debug_downsampledX      = Signal(16)
            debug_downsampledY      = Signal(16)
            debug_upsampledX        = Signal(16)
            debug_upsampledY        = Signal(16)
            debug_phase2_inv_alt    = Signal(23)
            debug_xval_upconverted  = Signal(16)
            debug_yval_upconverted  = Signal(16)
            debug_ce_out_down_x     = Signal()
            debug_ce_out_up_x       = Signal()
            debug_cic_ce_x   = Signal()
            debug_comp_ce_x  = Signal()
            debug_hb_ce_x    = Signal()
            ds_cic_out_x  = Signal(12)
            ds_comp_out_x = Signal(16)
            output_select = Signal(2)

            self.specials += Instance(
                "adc_cordic_dsp_dac",
                # Clocks / reset
                i_sys_clk      = ClockSignal("sys"),
                i_rst_n        = ResetSignal("sys"),

                # ADC ports
                o_adc_clk_ch0   = platform.request("adc_clk_ch0"),
                o_adc_clk_ch1   = platform.request("adc_clk_ch1"),
                i_adc_data_ch0  = platform.request("adc_data_ch0"),
                i_adc_data_ch1  = platform.request("adc_data_ch1"),

                # DAC ports
                o_da1_clk       = platform.request("da1_clk",  0),
                o_da1_wrt       = platform.request("da1_wrt",  0),
                o_da1_data      = platform.request("da1_data", 0),
                o_da2_clk       = platform.request("da2_clk",  0),
                o_da2_wrt       = platform.request("da2_wrt",  0),
                o_da2_data      = platform.request("da2_data", 0),
                # CPU Inputs
                i_phase_inc    = phase_inc,
                i_output_select = output_select,

                o_debug_filter_in         = debug_filter_in,
                o_debug_phase2            = debug_phase2,
                o_debug_xval_downconverted = debug_xval_downconverted,
                o_debug_yval_downconverted = debug_yval_downconverted,
                o_debug_downsampledX      = debug_downsampledX,
                o_debug_downsampledY      = debug_downsampledY,
                o_debug_upsampledX        = debug_upsampledX,
                o_debug_upsampledY        = debug_upsampledY,
                o_debug_phase2_inv_alt    = debug_phase2_inv_alt,
                o_debug_xval_upconverted  = debug_xval_upconverted,
                o_debug_yval_upconverted  = debug_yval_upconverted,
                o_debug_ce_out_down_x     = debug_ce_out_down_x,
                o_debug_ce_out_up_x       = debug_ce_out_up_x,
                o_debug_cic_ce_x   = debug_cic_ce_x,
                o_debug_comp_ce_x  = debug_comp_ce_x,
                o_debug_hb_ce_x    = debug_hb_ce_x,
                o_debug_cic_out_x   = ds_cic_out_x,
                o_debug_comp_out_x  = ds_comp_out_x,
            )

            self.submodules.analyzer = LiteScopeAnalyzer(
            [
                debug_filter_in,
                debug_phase2,
                debug_xval_downconverted,
                debug_yval_downconverted,
                debug_downsampledX,
                debug_downsampledY,
                debug_upsampledX,
                debug_upsampledY,
                debug_phase2_inv_alt,
                debug_xval_upconverted,
                debug_yval_upconverted,
                debug_ce_out_down_x,
                debug_ce_out_up_x,
                debug_cic_ce_x,
                debug_comp_ce_x,
                debug_hb_ce_x,
                ds_cic_out_x,
                ds_comp_out_x,
                phase_inc,
                output_select
            ],
            depth        = 16384,
            clock_domain = "sys",
            samplerate   = sys_clk_freq,
            csr_csv      = "analyzer.csv"
            )
            self.add_csr("analyzer")


        # ---------------------------------------------------------------------
        #  ADC - DSP - DAC (no CPU)
        # ---------------------------------------------------------------------
        if with_adc_dsp_dac_nocpu:
            for filename in [
                "adc/adc.v",
                "dac/dac.v",

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

                "adc-dsp-dac/adc_dsp_dac_nocpu.v"

            ]:
                self.platform.add_source(f"{verilog_dir}/{filename}")


            debug_downsampledY = Signal(16)
            debug_upsampledY   = Signal(16)
            debug_ce_out_down  = Signal()
            debug_ce_out_up    = Signal()
            debug_adc_input    = Signal(12)

            self.specials += Instance(
                "adc_dsp_dac_nocpu",
                # Clocks / reset
                i_sys_clk      = ClockSignal("sys"),
                i_rst_n        = ResetSignal("sys"),

                # ADC ports
                o_adc_clk_ch0   = platform.request("adc_clk_ch0"),
                o_adc_clk_ch1   = platform.request("adc_clk_ch1"),
                i_adc_data_ch0  = platform.request("adc_data_ch0"),
                i_adc_data_ch1  = platform.request("adc_data_ch1"),

                # DAC ports
                o_da1_clk       = platform.request("da1_clk",  0),
                o_da1_wrt       = platform.request("da1_wrt",  0),
                o_da1_data      = platform.request("da1_data", 0),
                o_da2_clk       = platform.request("da2_clk",  0),
                o_da2_wrt       = platform.request("da2_wrt",  0),
                o_da2_data      = platform.request("da2_data", 0),

                o_debug_downsampledY = debug_downsampledY,
                o_debug_upsampledY   = debug_upsampledY,
                o_debug_ce_out_down  = debug_ce_out_down,
                o_debug_ce_out_up    = debug_ce_out_up,
                o_debug_adc_input    = debug_adc_input,
            )

            analyzer_signals = [
                debug_downsampledY,
                debug_upsampledY,
                debug_ce_out_down,
                debug_ce_out_up,
                debug_adc_input
            ]

            self.submodules.analyzer = LiteScopeAnalyzer(
                analyzer_signals,
                depth        = 16384,
                clock_domain = "sys",
                samplerate   = sys_clk_freq,
                csr_csv      = "analyzer.csv"
            )
            self.add_csr("analyzer")




        if with_uberclock:

            for filename in [
                "adc/adc.v",
                "dac/dac.v",

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

                "uberclock/uberclock.v",

                "cordic/cordic_pre_rotate.v",
                "cordic/cordic_pipeline_stage.v",
                "cordic/cordic_round.v",
                "cordic/cordic.v",
                "cordic/cordic_logic.v",
                "cordic/gain_and_saturate.v",

                "cordic16/cordic16.v",
                # "cordic16/gain_and_saturate.v",
                # "cordic16/cordic_round.v",
                "cordic16/cordic_pre_rotate_16.v",
                # "cordic16/cordic_pipeline_stage.v",
            ]:
                self.platform.add_source(f"{verilog_dir}/{filename}")

            # expose a 3-bit CSR to the CPU for mode selection
            self._mode_sel = CSRStorage(3, description="uberclock signal-path mode (0–4)")
            self.add_csr("mode_sel")
            mode_sel = self._mode_sel.storage

            self._phase_inc = CSRStorage(19, description="uberclock CORDIC phase increment")
            self.add_csr("phase_inc")
            phase_inc = self._phase_inc.storage


            # instantiate your top-level
            self.specials += Instance(
                "uberclock",
                # clocks & reset
                i_sys_clk   = ClockSignal("sys"),
                i_rst_n     = ResetSignal("sys"),

                # the CPU-driven selector
                i_mode_sel  = mode_sel,

                # ADC ports
                o_adc_clk_ch0  = platform.request("adc_clk_ch0"),
                o_adc_clk_ch1  = platform.request("adc_clk_ch1"),
                i_adc_data_ch0 = platform.request("adc_data_ch0"),
                i_adc_data_ch1 = platform.request("adc_data_ch1"),

                # phase increment
                i_phase_inc = phase_inc,

                # DAC ports
                o_da1_clk   = platform.request("da1_clk"),
                o_da1_wrt   = platform.request("da1_wrt"),
                o_da1_data  = platform.request("da1_data"),
                o_da2_clk   = platform.request("da2_clk"),
                o_da2_wrt   = platform.request("da2_wrt"),
                o_da2_data  = platform.request("da2_data"),
            )

        # ---------------------------------------------------------------------
        #  CORDIC - DSP - DAC
        # ---------------------------------------------------------------------
        if with_cordic_dsp_dac:
            for filename in [
                "adc/adc.v",
                "dac/dac.v",

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

                "cordic_dsp_dac/cordic_dsp_dac.v",

                "cordic/cordic_pre_rotate.v",
                "cordic/cordic_pipeline_stage.v",
                "cordic/cordic_round.v",
                "cordic/cordic.v",
                "cordic/cordic_logic.v",
                "cordic/gain_and_saturate.v",

                "cordic16/cordic16.v",
                # "cordic16/gain_and_saturate.v",
                # "cordic16/cordic_round.v",
                "cordic16/cordic_pre_rotate_16.v",
                # "cordic16/cordic_pipeline_stage.v",
            ]:
                self.platform.add_source(f"{verilog_dir}/{filename}")


            self._input_select = CSRStorage(1, description="Select ADC (0) or NCO tone (1) as pipeline input")
            input_select = self._input_select.storage

            self._output_select= CSRStorage(2, description="Output select for DACs")
            output_select = self._output_select.storage

            self._phase_inc_nco = CSRStorage(19, description="CORDIC_DAC NCO phase increment")
            phase_inc_nco = self._phase_inc_nco.storage

            self._phase_inc_down = CSRStorage(19, description="CORDIC_DAC DOWN phase increment")
            phase_inc_down = self._phase_inc_down.storage

            self._gain1 = CSRStorage(32, description="32--bit Gain1 value")
            gain1 = self._gain1.storage

            self._gain2 = CSRStorage(32, description="32--bit Gain2 value")
            gain2 = self._gain2.storage

            dbg_nco_cos         = Signal(12)
            dbg_nco_sin         = Signal(12)
            dbg_phase_acc_down  = Signal(19)
            dbg_x_downconverted = Signal(12)
            dbg_y_downconverted = Signal(12)
            dbg_downsampled_x   = Signal(16)
            dbg_downsampled_y   = Signal(16)
            dbg_upsampled_x     = Signal(16)
            dbg_upsampled_y     = Signal(16)
            dbg_phase_inv       = Signal(23)
            dbg_x_upconverted   = Signal(16)
            dbg_y_upconverted   = Signal(16)
            dbg_ce_down_x       = Signal()
            dbg_ce_up_x         = Signal()
            dbg_cic_ce_x        = Signal()
            dbg_comp_ce_x       = Signal()
            dbg_hb_ce_x         = Signal()
            dbg_cic_out_x       = Signal(12)
            dbg_comp_out_x      = Signal(16)

            self.specials += Instance(
                "cordic_dsp_dac",
                # Clocks / reset
                i_sys_clk      = ClockSignal("sys"),
                i_rst          = ResetSignal("sys"),

                # ADC ports
                o_adc_clk_ch0  = platform.request("adc_clk_ch0"),
                o_adc_clk_ch1  = platform.request("adc_clk_ch1"),
                i_adc_data_ch0 = platform.request("adc_data_ch0"),
                i_adc_data_ch1 = platform.request("adc_data_ch1"),

                # DAC ports
                o_da1_clk       = platform.request("da1_clk",  0),
                o_da1_wrt       = platform.request("da1_wrt",  0),
                o_da1_data      = platform.request("da1_data", 0),
                o_da2_clk       = platform.request("da2_clk",  0),
                o_da2_wrt       = platform.request("da2_wrt",  0),
                o_da2_data      = platform.request("da2_data", 0),

                # CPU Inputs
                i_phase_inc_nco  = phase_inc_nco,
                i_phase_inc_down = phase_inc_down,

                i_input_select      = input_select,
                i_output_select     = output_select,

                i_gain1             = gain1,
                i_gain2             = gain2,

                # Debug outputs
                o_dbg_nco_cos         = dbg_nco_cos,
                o_dbg_nco_sin         = dbg_nco_sin,
                o_dbg_phase_acc_down  = dbg_phase_acc_down,
                o_dbg_x_downconverted = dbg_x_downconverted,
                o_dbg_y_downconverted = dbg_y_downconverted,
                o_dbg_downsampled_x   = dbg_downsampled_x,
                o_dbg_downsampled_y   = dbg_downsampled_y,
                o_dbg_upsampled_x     = dbg_upsampled_x,
                o_dbg_upsampled_y     = dbg_upsampled_y,
                o_dbg_phase_inv       = dbg_phase_inv,
                o_dbg_x_upconverted   = dbg_x_upconverted,
                o_dbg_y_upconverted   = dbg_y_upconverted,
                o_dbg_ce_down_x       = dbg_ce_down_x,
                o_dbg_ce_up_x         = dbg_ce_up_x,
                o_dbg_cic_ce_x        = dbg_cic_ce_x,
                o_dbg_comp_ce_x       = dbg_comp_ce_x,
                o_dbg_hb_ce_x         = dbg_hb_ce_x,
                o_dbg_cic_out_x       = dbg_cic_out_x,
                o_dbg_comp_out_x      = dbg_comp_out_x
            )

            self.submodules.analyzer = LiteScopeAnalyzer(
                [
                    dbg_nco_cos,
                    dbg_nco_sin,
                    dbg_phase_acc_down,
                    dbg_x_downconverted,
                    dbg_y_downconverted,
                    dbg_downsampled_x,
                    dbg_downsampled_y,
                    dbg_upsampled_x,
                    dbg_upsampled_y,
                    dbg_phase_inv,
                    dbg_x_upconverted,
                    dbg_y_upconverted,
                    dbg_ce_down_x,
                    dbg_ce_up_x,
                    dbg_cic_ce_x,
                    dbg_comp_ce_x,
                    dbg_hb_ce_x,
                    dbg_cic_out_x,
                    dbg_comp_out_x,
                    phase_inc_nco,
                    phase_inc_down,
                    input_select,
                    output_select,
                    gain1,
                    gain2
                ],
                depth        = 16384,
                clock_domain = "sys",
                samplerate   = sys_clk_freq,
                csr_csv      = "analyzer.csv"
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
    parser.add_argument("--with-input-mux", action="store_true",
        help="Instantiate input_mux module")
    parser.add_argument("--with-adc-dsp-dac", action="store_true",
        help="Instantiate ADC - DSP - DAC Path")
    parser.add_argument("--with-adc-dsp-dac-nocpu", action="store_true",
        help="Instantiate ADC - DSP - DAC Path (no CPU)")
    parser.add_argument("--with-adc-cordic-dsp-dac", action="store_true",
        help="Instantiate ADC - CORDIC -DSP - DAC Path (no CPU)")
    parser.add_argument("--with-uberclock", action="store_true",
        help="Instantiate Uberclock")
    parser.add_argument("--with-cordic-dsp-dac", action="store_true",
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
        with_cordic              = args.with_cordic,
        with_dac                 = args.with_dac,
        with_cordic_dac          = args.with_cordic_dac,
        with_icd                 = args.with_icd,
        with_adc_dac             = args.with_adc_dac,
        with_input_mux           = args.with_input_mux,
        with_adc_dsp_dac         = args.with_adc_dsp_dac,
        with_adc_dsp_dac_nocpu   = args.with_adc_dsp_dac_nocpu,
        with_adc_cordic_dsp_dac  = args.with_adc_cordic_dsp_dac,
        with_uberclock           = args.with_uberclock,
        with_cordic_dsp_dac      = args.with_cordic_dsp_dac,
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
