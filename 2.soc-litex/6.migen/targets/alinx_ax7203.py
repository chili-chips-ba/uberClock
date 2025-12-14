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
    def __init__(self, platform, sys_clk_freq=65e6, with_dram=True):
        self.rst    = Signal()
        self.cd_sys = ClockDomain()

        if with_dram:
            self.cd_sys4x     = ClockDomain()
            self.cd_sys4x_dqs = ClockDomain()
            self.cd_idelay    = ClockDomain()

        clk200 = platform.request("clk200")
        clk200_se = Signal()

        self.specials += Instance("IBUFDS",
            i_I = clk200.p,
            i_IB= clk200.n,
            o_O = clk200_se
        )

        margin = 1e-2

        # ---------------------------------------------------------------------
        # Main MMCM: Generate sysclk and DDR clocks
        # ---------------------------------------------------------------------
        self.pll = pll = S7MMCM(speedgrade=-2)
        self.comb += pll.reset.eq(self.rst)
        pll.register_clkin(clk200_se, 200e6)

        pll.create_clkout(self.cd_sys, sys_clk_freq, margin=margin)

        if with_dram:
            pll.create_clkout(self.cd_sys4x,     4 * sys_clk_freq, margin=margin)
            pll.create_clkout(self.cd_sys4x_dqs, 4 * sys_clk_freq, phase=90, margin=margin)

        # ---------------------------------------------------------------------
        # IDELAYCTRL: Use 200 MHz after BUFG
        # ---------------------------------------------------------------------
        if with_dram:
            clk200_bufg = Signal()
            self.specials += Instance("BUFG", i_I=clk200_se, o_O=clk200_bufg)
            self.comb += self.cd_idelay.clk.eq(clk200_bufg)
            self.idelayctrl = S7IDELAYCTRL(self.cd_idelay)

        platform.add_false_path_constraints(self.cd_sys.clk, pll.clkin)

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
            "uberclock/rx_channel.v",
            "uberclock/tx_channel.v",
            "to_polar/to_polar.v",
            "cordic/cordic_pre_rotate.v","cordic/cordic_pipeline_stage.v",
            "cordic/cordic_round.v","cordic/cordic.v",
            "cordic/cordic_logic.v","cordic/gain_and_saturate.v",
            "cordic16/cordic16.v","cordic16/cordic_pre_rotate_16.v",
        ]
        for fn in files:
            self.platform.add_source(f"{verilog_dir}/{fn}")

        self._input_select        = CSRStorage(2,  description="0=ADC, 1=NCO, 2=CPU")
        self._output_select_ch1   = CSRStorage(4,  description="DAC CH1 output selector")
        self._output_select_ch2   = CSRStorage(4,  description="DAC CH1 output selector")
        self._lowspeed_dbg_select = CSRStorage(3,  description="Lowspeed dbg selector")
        self._highspeed_dbg_select = CSRStorage(3,  description="Lowspeed dbg selector")
        self._upsampler_input_mux = CSRStorage(2,  description="0=Gain, 1=CPU, 2=CPU NCO")
        self._phase_inc_nco       = CSRStorage(32, description="NCO phase increment")
        self._nco_mag             = CSRStorage(12, description="NCO magnitude")
        self._phase_inc_down_1      = CSRStorage(24, description="Downconversion phase inc ch1")
        self._phase_inc_down_2      = CSRStorage(24, description="Downconversion phase inc ch2")
        self._phase_inc_down_3      = CSRStorage(24, description="Downconversion phase inc ch3")
        self._phase_inc_down_4      = CSRStorage(24, description="Downconversion phase inc ch4")
        self._phase_inc_down_5      = CSRStorage(24, description="Downconversion phase inc ch5")
        self._phase_inc_down_ref      = CSRStorage(24, description="Downconversion phase inc ref")

        self._phase_inc_cpu1       = CSRStorage(24, description="CPU phase increment CH1")
        self._phase_inc_cpu2       = CSRStorage(24, description="CPU phase increment CH2")
        self._phase_inc_cpu3       = CSRStorage(24, description="CPU phase increment CH3")
        self._phase_inc_cpu4       = CSRStorage(24, description="CPU phase increment CH4")
        self._phase_inc_cpu5       = CSRStorage(24, description="CPU phase increment CH5")
        self._mag_cpu1             = CSRStorage(24, description="CPU magnitude CH1")
        self._mag_cpu2             = CSRStorage(24, description="CPU magnitude CH2")
        self._mag_cpu3             = CSRStorage(24, description="CPU magnitude CH3")
        self._mag_cpu4             = CSRStorage(24, description="CPU magnitude CH4")
        self._mag_cpu5             = CSRStorage(24, description="CPU magnitude CH5")
        
        self._gain1               = CSRStorage(32, description="Gain1 (Q format)")
        self._gain2               = CSRStorage(32, description="Gain2 (Q format)")
        self._gain3               = CSRStorage(32, description="Gain3 (Q format)")
        self._gain4               = CSRStorage(32, description="Gain4 (Q format)")
        self._gain5               = CSRStorage(32, description="Gain5 (Q format)")
        self._upsampler_input_x   = CSRStorage(16, description="Upsampler input x")
        self._upsampler_input_y   = CSRStorage(16, description="Upsampler input y")

        self._downsampled_data_x  = CSRStatus(16, description="Downsampled data x")
        self._downsampled_data_y  = CSRStatus(16, description="Downsampled data y")
        self._magnitude           = CSRStatus(16, description="Downsampled magnitude")
        self._phase               = CSRStatus(25, description="Downsampled phase")
        self._final_shift         = CSRStorage(3, description="Final output shift S (divide by 2^S)")
                # --- Capture CSRs (512-sample single-shot) ---
        self._cap_arm  = CSRStorage(1,  description="Write 1 to arm/start 512-sample capture")
        self._cap_idx  = CSRStorage(16, description="Read index (0..511)")
        self._cap_done = CSRStatus(1,   description="Capture done (1 when 512 samples stored)")
        self._cap_data = CSRStatus(16,  description="Captured sample at cap_idx (sign-extended)")

        # --- High-speed capture CSRs (8192 @ 65MHz) ---
        self._hs_cap_arm  = CSRStorage(1,  description="HS capture arm (pulse 0->1)")
        self._hs_cap_idx  = CSRStorage(16, description="HS capture read index (0..8191)")
        self._hs_cap_done = CSRStatus(1,   description="HS capture done flag")
        self._hs_cap_data = CSRStatus(16,  description="HS captured data (sign-extended 16-bit)")
        
        input_select              = self._input_select.storage
        output_select_ch1         = self._output_select_ch1.storage
        output_select_ch2         = self._output_select_ch2.storage
        lowspeed_dbg_select       = self._lowspeed_dbg_select.storage
        highspeed_dbg_select       = self._highspeed_dbg_select.storage
        upsampler_input_mux       = self._upsampler_input_mux.storage
        phase_inc_nco             = self._phase_inc_nco.storage
        nco_mag                   = self._nco_mag.storage
        phase_inc_down_1            = self._phase_inc_down_1.storage
        phase_inc_down_2            = self._phase_inc_down_2.storage
        phase_inc_down_3            = self._phase_inc_down_3.storage
        phase_inc_down_4            = self._phase_inc_down_4.storage
        phase_inc_down_5            = self._phase_inc_down_5.storage
        phase_inc_down_ref            = self._phase_inc_down_ref.storage
        phase_inc_cpu1             = self._phase_inc_cpu1.storage
        phase_inc_cpu2             = self._phase_inc_cpu2.storage
        phase_inc_cpu3             = self._phase_inc_cpu3.storage
        phase_inc_cpu4             = self._phase_inc_cpu4.storage
        phase_inc_cpu5             = self._phase_inc_cpu5.storage
        mag_cpu1                   = self._mag_cpu1.storage
        mag_cpu2                   = self._mag_cpu2.storage
        mag_cpu3                   = self._mag_cpu3.storage
        mag_cpu4                   = self._mag_cpu4.storage
        mag_cpu5                   = self._mag_cpu5.storage
        gain1, gain2              = self._gain1.storage, self._gain2.storage
        gain3, gain4              = self._gain3.storage, self._gain4.storage
        gain5                     = self._gain5.storage
        upsampler_input_x         = self._upsampler_input_x.storage
        upsampler_input_y         = self._upsampler_input_y.storage
        final_shift = self._final_shift.storage
        ce_down = Signal(name="ce_down")
        self.submodules.evm     = EventManager()
        self.evm.ce_down = EventSourcePulse(description="Downsample ready")
        self.evm.finalize()

        #self.add_csr("evm")
        self.irq.add("evm")

        # dbg = {
        #     "downsampled_x":  Signal(16),
        #     "downsampled_y":  Signal(16),
        # }

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
            i_input_select        = input_select,
            i_output_select_ch1   = output_select_ch1,
            i_output_select_ch2   = output_select_ch2,
            i_lowspeed_dbg_select = lowspeed_dbg_select,
            i_highspeed_dbg_select = highspeed_dbg_select,
            i_upsampler_input_mux = upsampler_input_mux,
            i_phase_inc_nco       = phase_inc_nco,
            i_nco_mag             = nco_mag,
            i_phase_inc_down_1    = phase_inc_down_1,
            i_phase_inc_down_2    = phase_inc_down_2,
            i_phase_inc_down_3    = phase_inc_down_3,
            i_phase_inc_down_4    = phase_inc_down_4,
            i_phase_inc_down_5    = phase_inc_down_5,
            i_phase_inc_down_ref    = phase_inc_down_ref,
            i_phase_inc_cpu1       = phase_inc_cpu1,
            i_phase_inc_cpu2       = phase_inc_cpu2,
            i_phase_inc_cpu3       = phase_inc_cpu3,
            i_phase_inc_cpu4       = phase_inc_cpu4,
            i_phase_inc_cpu5       = phase_inc_cpu5,
            i_mag_cpu1             = mag_cpu1,
            i_mag_cpu2             = mag_cpu2,
            i_mag_cpu3             = mag_cpu3,
            i_mag_cpu4             = mag_cpu4,
            i_mag_cpu5             = mag_cpu5,
            i_gain1               = gain1,
            i_gain2               = gain2,
            i_gain3               = gain3,
            i_gain4               = gain4,
            i_gain5               = gain5,
            i_upsampler_input_x   = upsampler_input_x,
            i_upsampler_input_y   = upsampler_input_y,

            i_final_shift         = final_shift,
            o_magnitude           = self._magnitude.status,
            o_phase               = self._phase.status,

            # CSR outputs + event
            o_downsampled_data_x = self._downsampled_data_x.status,
            o_downsampled_data_y = self._downsampled_data_y.status,
            o_ce_down          = ce_down,
            # Capture ports
            i_cap_arm  = self._cap_arm.storage,
            i_cap_idx  = self._cap_idx.storage,
            o_cap_done = self._cap_done.status,
            o_cap_data = self._cap_data.status,

            # ---- High-speed capture ports ----
            i_hs_cap_arm  = self._hs_cap_arm.storage,
            i_hs_cap_idx  = self._hs_cap_idx.storage,
            o_hs_cap_done = self._hs_cap_done.status,
            o_hs_cap_data = self._hs_cap_data.status,
            # debug outputs (unpack the dict)
            # **{f"o_dbg_{name}": sig for name, sig in dbg.items()}
        )

        self.sync += If(ce_down, self.evm.ce_down.trigger.eq(1))
        # self.comb += self._downsampled_data_x.status.eq(dbg["downsampled_x"])
        # self.comb += self._downsampled_data_y.status.eq(dbg["downsampled_y"])


        # probes = (
        #     list(dbg.values()) +
        #     [phase_inc_nco, phase_inc_down_1, phase_inc_cpu,
        #      input_select, output_select_ch1, output_select_ch1, upsampler_input_mux,
        #      gain1, gain2,
        #      ce_down,
        #      upsampler_input_x,
        #      upsampler_input_y,
        #      self._downsampled_data_x.status,
        #      self._downsampled_data_y.status,
        #     ]
        # )

        # self.submodules.analyzer = LiteScopeAnalyzer(
        #     probes,
        #     depth        = 2048, #32768
        #     clock_domain = "sys",
        #     samplerate   = sys_clk_freq
        # )
        # self.add_csr("analyzer")

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
