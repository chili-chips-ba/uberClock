#
# This file is part of LiteX-Boards.
#
# Copyright (c) 2022 Yonggang Liu <ggang.liu@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

from litex.build.generic_platform import *
from litex.build.xilinx import Xilinx7SeriesPlatform, VivadoProgrammer
from litex.build.openfpgaloader import OpenFPGALoader

# IOs ----------------------------------------------------------------------------------------------

_io = [
    # Clk / Rst
    # 200Mhz Active Differential clock
    ("clk200", 0,
       Subsignal("p", Pins("R4"), IOStandard("DIFF_SSTL15")),
       Subsignal("n", Pins("T4"), IOStandard("DIFF_SSTL15")),
    ),

    # 148.5Mhz Active Differential Crystal
    ("clk125", 0,
       Subsignal("p", Pins("F6"), IOStandard("DIFF_SSTL12")),
       Subsignal("n", Pins("E6"), IOStandard("DIFF_SSTL12")),
    ),

    #("cpu_reset", 0,  Pins("T6"), IOStandard("LVCMOS15")),

    # Leds
    ("user_led", 0, Pins("B13"),  IOStandard("LVCMOS33")),
    ("user_led", 1, Pins("C13"),  IOStandard("LVCMOS33")),
    ("user_led", 2, Pins("D14"),  IOStandard("LVCMOS33")),
    ("user_led", 3, Pins("D15"),  IOStandard("LVCMOS33")),

    # Buttons
    ("user_btn", 0, Pins("T6"), IOStandard("LVCMOS15")),

    # Serial
    ("serial", 0,
        Subsignal("tx", Pins("N15"), IOStandard("LVCMOS33")),
        Subsignal("rx", Pins("P20"), IOStandard("LVCMOS33")),
    ),

    # DDR3 SDRAM
    ("ddram", 0,
        Subsignal("a",      Pins(
            "AA4 AB2 AA5 AB5 AB1 U3 W1 T1",
            "V2 U2 Y1 W2 Y2 U1 V3"),
            IOStandard("SSTL15")),
        Subsignal("ba",      Pins("AA3 Y3 Y4"),   IOStandard("SSTL15")),
        Subsignal("ras_n",   Pins("V4"),          IOStandard("SSTL15")),
        Subsignal("cas_n",   Pins("W4"),          IOStandard("SSTL15")),
        Subsignal("we_n",    Pins("AA1"),         IOStandard("SSTL15")),
        Subsignal("cs_n",    Pins("AB3"),         IOStandard("SSTL15")),
        Subsignal("dm",      Pins("D2 G2 M2 M5"), IOStandard("SSTL15")),
        Subsignal("dq",      Pins(
            "C2 G1 A1 F3 B2 F1 B1 E2",
            "H3 G3 H2 H5 J1 J5 K1 H4",
            "L4 M3 L3 J6 K3 K6 J4 L5",
            "P1 N4 R1 N2 M6 N5 P6 P2"),
             IOStandard("SSTL15"),
             Misc("IN_TERM=UNTUNED_SPLIT_50")),
        Subsignal("dqs_p",   Pins("E1 K2 M1 P5"), IOStandard("DIFF_SSTL15"), Misc("IN_TERM=UNTUNED_SPLIT_50")),
        Subsignal("dqs_n",   Pins("D1 J2 L1 P4"), IOStandard("DIFF_SSTL15"), Misc("IN_TERM=UNTUNED_SPLIT_50")),
        Subsignal("clk_p",   Pins("R3"),          IOStandard("DIFF_SSTL15")),
        Subsignal("clk_n",   Pins("R2"),          IOStandard("DIFF_SSTL15")),
        Subsignal("cke",     Pins("T5"),          IOStandard("SSTL15")),
        Subsignal("odt",     Pins("U5"),          IOStandard("SSTL15")),
        Subsignal("reset_n", Pins("W6"),          IOStandard("LVCMOS15")),
        Misc("SLEW=FAST")
    ),


    # SPIFlash4x
    ("spiflash4x", 0,
        Subsignal("cs_n", Pins("AA12")),
        Subsignal("clk",  Pins("Y11")),
        Subsignal("dq",   Pins("P22 R22 P21 R21")),
        IOStandard("LVCMOS33")
    ),

    # RGMII Ethernet
    ("eth_clocks", 0,
        Subsignal("tx", Pins("E18")),
        Subsignal("rx", Pins("B17")),
        IOStandard("LVCMOS33")
    ),
    ("eth_clocks", 1,
        Subsignal("tx", Pins("A14")),
        Subsignal("rx", Pins("E19")),
        IOStandard("LVCMOS33")
    ),

    ("eth", 0,
        Subsignal("rst_n",   Pins("D16")),
        Subsignal("mdio",    Pins("B15")),
        Subsignal("mdc",     Pins("B16")),
        Subsignal("rx_ctl",  Pins("A15")),
        Subsignal("rx_data", Pins("A16 B18 C18 C19")),
        Subsignal("tx_ctl",  Pins("F18")),
        Subsignal("tx_data", Pins("C20 D20 A19 A18")),
        IOStandard("LVCMOS33"),
        Misc("SLEW=FAST")
    ),
    ("eth", 1,
        Subsignal("rst_n",   Pins("B22")),
        Subsignal("mdio",    Pins("C22")),
        Subsignal("mdc",     Pins("F20")),
        Subsignal("rx_ctl",  Pins("F19")),
        Subsignal("rx_data", Pins("A20 B20 D19 C17")),
        Subsignal("tx_ctl",  Pins("D17")),
        Subsignal("tx_data", Pins("E17 C14 C15 A13")),
        IOStandard("LVCMOS33"),
        Misc("SLEW=FAST")
    ),

    # PCIe. TODO: Check if it is LVCMOS33?
    # TODO: Check rst_n signal?
    ("pcie_x4", 0,
       #Subsignal("rst_n", Pins("T6"), IOStandard("LVCMOS15")),
        Subsignal("clk_p", Pins("F10")),
        Subsignal("clk_n", Pins("E10")),
        Subsignal("rx_p",  Pins("D11 B8 B10 D9")),
        Subsignal("rx_n",  Pins("C11 A8 A10 C9")),
        Subsignal("tx_p",  Pins("D5 B4 B6 D7")),
        Subsignal("tx_n",  Pins("C5 A4 A6 C7"))
    ),

    # HDMI Out
    ("hdmi_out", 0,
        Subsignal("clk",     Pins("M13")),
        Subsignal("rst_n",   Pins("J19")),
        Subsignal("hsync_n", Pins("T15")),
        Subsignal("vsync_n", Pins("514")),
        Subsignal("de_n",    Pins("V13")),
        Subsignal("r",       Pins("L18 M18 N18 N19 M20 N20 L21 M21")), # D16-D23
        Subsignal("g",       Pins("K17 J17 L16 K16 L14 L15 M15 M16")), # D8-D15
        Subsignal("b",       Pins("V14 H14 J14 K13 K14 L13 L19 L20")), # D0-D7
        IOStandard("LVCMOS33"),
        Misc("SLEW=FAST")
    ),

    # HDMI In
    ("hdmi_in", 0,
        Subsignal("clk",     Pins("K21")),
        Subsignal("rst_n",   Pins("H19")),
        Subsignal("hsync_n", Pins("K19")),
        Subsignal("vsync_n", Pins("K18")),
        Subsignal("de_n",    Pins("H17")),
        Subsignal("r",       Pins("F21 M17 J16 F15 G17 G18 G15 G16")), # D16-D23
        Subsignal("g",       Pins("G22 G21 D22 E22 D21 E21 B21 A21")), # D8-D15
        Subsignal("b",       Pins("H18 N22 M22 K22 J22 H22 H20 G20")), # D0-D7
        IOStandard("LVCMOS33"),
        Misc("SLEW=FAST")
    ),

    # SDCard.
    ("sdcard", 0,
        Subsignal("data", Pins("AA13 AB13 Y13 AA14"), Misc("PULLUP True")),
        Subsignal("cmd",  Pins("AB11"),               Misc("PULLUP True")),
        Subsignal("clk",  Pins("AB12")),
        Subsignal("cd",   Pins("F14")),
        Misc("SLEW=FAST"),
        IOStandard("LVCMOS33"),
    ),

    # I2C EEPROM
    ("eeprom", 0,
        Subsignal("scl", Pins("F13")),
        Subsignal("sda", Pins("E14")),
        IOStandard("LVCMOS33")
    ),

    # ------------------ ADC on J13 (AD9238) ------------------
    # Channel 0 clock & rising-edge data (12 bits)
    ("adc_clk_ch0",  0, Pins("T18"), IOStandard("LVCMOS33")),
    ("adc_data_ch0", 0,
        Pins("W21 W22 U21 T21 Y22 Y21 W19 W20 Y18 Y19 U22 V22"),
        IOStandard("LVCMOS33")
    ),
    # Channel 1 clock & rising-edge data (12 bits)
    ("adc_clk_ch1",  0, Pins("W16"), IOStandard("LVCMOS33")),
    ("adc_data_ch1", 0,
        Pins("W15 V17 W17 U15 V15 AB21 AB22 AA21 AA20 AB20 AA19 AA18"),
        IOStandard("LVCMOS33")
    ),
    # ------------------ DAC on J11 ------------------
    # DAC1: 14-bit data + write strobe + clock
    ("da1_clk",  0, Pins("V20"), IOStandard("LVCMOS33")),
    ("da1_wrt",  0, Pins("U20"), IOStandard("LVCMOS33")),
    ("da1_data", 0,
        Pins("V19 V18 R19 P19 U18 U17 T16 U16 P17 N17 P15 R16 R17 P16"),
        IOStandard("LVCMOS33")),
    # DAC2: 14-bit data + write strobe + clock
    ("da2_clk",  0, Pins("AA9"), IOStandard("LVCMOS33")),
    ("da2_wrt",  0, Pins("AB10"), IOStandard("LVCMOS33")),
    ("da2_data", 0,
        Pins("AB17 AB16 AA16 Y16 AB15 AA15 W11 W12 Y11 Y12 V10 W10 AA11 AA10"),
        IOStandard("LVCMOS33")),

]

# Connectors ---------------------------------------------------------------------------------------

_connectors = [
    ("J11", {
         3: "P16",  4: "R17",
         5: "R16",  6: "P15",
         7: "N17",  8: "P17",
         9: "U16",  10: "T16",
        11: "U17",  12: "U18",
        13: "P19",  14: "R19",
        15: "V18",  16: "V19",
        17: "U20",  18: "V20",
        19: "AA9",  20: "AB10",
        21: "AA10", 22: "AA11",
        23: "W10",  24: "V10",
        25: "Y12",  26: "Y11",
        27: "W12",  28: "W11",
        29: "AA15", 30: "AB15",
        31: "Y16",  32: "AA16",
        33: "AB16", 34: "AB17",
        35: "W14",  36: "Y14",
    }),
    ("J13", {
         3: "W16",  4: "W15",
         5: "V17",  6: "W17",
         7: "U15",  8: "V15",
         9: "AB21", 10: "AA20",
        11: "AA21", 12: "AA19",
        13: "AB20", 14: "AB19",
        15: "AA18", 16: "AB18",
        17: "T20",  18: "Y17",
        19: "W22",  20: "WW1",
        21: "T21",  22: "U21",
        23: "Y21",  24: "Y22",
        25: "W20",  26: "W19",
        27: "Y19",  28: "Y18",
        29: "V22",  30: "U22",
        31: "T18",  32: "R18",
        33: "R14",  34: "P14",
        35: "N13",  36: "N14",
    })
]

# PMODS --------------------------------------------------------------------------------------------

# Platform -----------------------------------------------------------------------------------------

class Platform(Xilinx7SeriesPlatform):
    default_clk_name   = "clk200"
    default_clk_period = 1e9/200e6

    def __init__(self, toolchain="vivado"):
        Xilinx7SeriesPlatform.__init__(self, "xc7a200tfbg484-2", _io, _connectors, toolchain=toolchain)

    def create_programmer(self, cable):
        return OpenFPGALoader("alinx_ax7203", cable)

    def do_finalize(self, fragment):
        Xilinx7SeriesPlatform.do_finalize(self, fragment)
        self.add_period_constraint(self.lookup_request("clk200", loose=True), 1e9/200e6)
