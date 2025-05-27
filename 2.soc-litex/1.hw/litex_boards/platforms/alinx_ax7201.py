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

    ("cpu_reset", 0,  Pins("T6"), IOStandard("LVCMOS15")),

    # Leds
    ("user_led", 0, Pins("E17"),  IOStandard("LVCMOS33")),
    ("user_led", 1, Pins("F16"),  IOStandard("LVCMOS33")),

    # Buttons/Keys
    ("user_btn", 0, Pins("D16"), IOStandard("LVCMOS33")),
    ("user_btn", 1, Pins("E16"), IOStandard("LVCMOS33")),


    # Serial
    ("serial", 0,
        Subsignal("tx", Pins("AB15"), IOStandard("LVCMOS33")),
        Subsignal("rx", Pins("AA15"), IOStandard("LVCMOS33")),
    ),

    # DDR3 SDRAM (MT41K256M16HA-125)
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
        Subsignal("cs_n", Pins("T19")),
        Subsignal("clk",  Pins("L12")),
        Subsignal("dq",   Pins("P22 R22 P21 R21")),
        IOStandard("LVCMOS33")
    ),

    # GMII Ethernet
    ("eth_clocks", 0,
        Subsignal("tx",  Pins("K21")),
        Subsignal("gtx", Pins("G21")),
        Subsignal("rx",  Pins("K18")),
        IOStandard("LVCMOS33")
    ),
    ("eth_clocks", 1,
        Subsignal("tx",  Pins("T14")),
        Subsignal("gtx", Pins("M16")),
        Subsignal("rx",  Pins("J20")),
        IOStandard("LVCMOS33")
    ),
    ("eth_clocks", 2,
        Subsignal("tx",  Pins("V10")),
        Subsignal("gtx", Pins("AA21")),
        Subsignal("rx",  Pins("V13")),
        IOStandard("LVCMOS33")
    ),
    ("eth_clocks", 3,
        Subsignal("tx",  Pins("U16")),
        Subsignal("gtx", Pins("P20")),
        Subsignal("rx",  Pins("Y16")),
        IOStandard("LVCMOS33")
    ),

    ("eth", 0,
        Subsignal("rst_n",   Pins("G20")),
        #Subsignal("int_n",   Pins("")),
        Subsignal("mdio",    Pins("L16")),
        Subsignal("mdc",     Pins("J17")),
        Subsignal("rx_dv",   Pins("M22")),
        Subsignal("rx_er",   Pins("N18")),
        Subsignal("rx_data", Pins("N22 H18 H17 M21 L21 N20 M20 N19")),
        Subsignal("tx_en",   Pins("G22")),
        Subsignal("tx_er",   Pins("K17")),
        Subsignal("tx_data", Pins("D22 H20 H22 J22 K22 L19 K19 L20")),
        Subsignal("col",     Pins("M18")),
        Subsignal("crs",     Pins("L18")),
        IOStandard("LVCMOS33"),
        Misc("SLEW=FAST")
    ),
    ("eth", 1,
        Subsignal("rst_n",   Pins("L14")),
        #Subsignal("int_n",   Pins("")),
        Subsignal("mdio",    Pins("AB22")),
        Subsignal("mdc",     Pins("AB21")),
        Subsignal("rx_dv",   Pins("L13")),
        Subsignal("rx_er",   Pins("G14")),
        Subsignal("rx_data", Pins("M13 K14 K13 J14 H14 H15 J15 H13")),
        Subsignal("tx_en",   Pins("M15")),
        Subsignal("tx_er",   Pins("T15")),
        Subsignal("tx_data", Pins("L15 K16 W15 W16 V17 W17 U15 V15")),
        Subsignal("col",     Pins("J11")),
        Subsignal("crs",     Pins("E22")),
        IOStandard("LVCMOS33"),
        Misc("SLEW=FAST")
    ),
    ("eth", 2,
        Subsignal("rst_n",   Pins("T20")),
        #Subsignal("int_n",   Pins("")),
        Subsignal("mdio",    Pins("V19")),
        Subsignal("mdc",     Pins("V20")),
        Subsignal("rx_dv",   Pins("AA20")),
        Subsignal("rx_er",   Pins("U21")),
        Subsignal("rx_data", Pins("AB20 AA19 AA18 AB18 Y17 W22 W21 T21")),
        Subsignal("tx_en",   Pins("V14")),
        Subsignal("tx_er",   Pins("AA9")),
        Subsignal("tx_data", Pins("W11 W12 Y11 Y12 W10 AA11 AA10 AB10")),
        Subsignal("col",     Pins("Y21")),
        Subsignal("crs",     Pins("Y22")),
        IOStandard("LVCMOS33"),
        Misc("SLEW=FAST")
    ),
    ("eth", 3,
        Subsignal("rst_n",   Pins("V18")),
        #Subsignal("int_n",   Pins("")),
        Subsignal("mdio",    Pins("U20")),
        Subsignal("mdc",     Pins("V18")),
        Subsignal("rx_dv",   Pins("W20")),
        Subsignal("rx_er",   Pins("N13")),
        Subsignal("rx_data", Pins("W19 Y19 V22 U22 T18 R18 R14 P14")),
        Subsignal("tx_en",   Pins("P16")),
        Subsignal("tx_er",   Pins("R19")),
        Subsignal("tx_data", Pins("R17 P15 N17 P17 T16 U17 U18 P19")),
        Subsignal("col",     Pins("N14")),
        Subsignal("crs",     Pins("N15")),
        IOStandard("LVCMOS33"),
        Misc("SLEW=FAST")
    ),

    # SFP TODO:Check the IOStandard
    ("gtp_refclk", 0,
        Subsignal("p", Pins("F6")),
        Subsignal("n", Pins("E6"))
    ),
    # SFP1
    ("sfp_tx", 0,
        Subsignal("p", Pins("B4")),
        Subsignal("n", Pins("A4"))
    ),
    ("sfp_rx", 0,
        Subsignal("p", Pins("B8")),
        Subsignal("n", Pins("A8"))
    ),
    ("sfp_tx_disable_n", 0, Pins("A15"), IOStandard("SSTL15")),
    ("sfp_rx_los",       0, Pins("B15"), IOStandard("SSTL15")),

    # SFP2
    ("sfp_tx", 1,
        Subsignal("p", Pins("D5")),
        Subsignal("n", Pins("C5"))
    ),
    ("sfp_rx", 1,
        Subsignal("p", Pins("D11")),
        Subsignal("n", Pins("C11"))
    ),
    ("sfp_tx_disable_n", 1, Pins("A16"), IOStandard("SSTL15")),
    ("sfp_rx_los",       1, Pins("B16"), IOStandard("SSTL15")),

    # SFP3
    ("sfp_tx", 2,
        Subsignal("p", Pins("B6")),
        Subsignal("n", Pins("A6"))
    ),
    ("sfp_rx", 2,
        Subsignal("p", Pins("B10")),
        Subsignal("n", Pins("A10"))
    ),
    ("sfp_tx_disable_n", 2, Pins("A13"), IOStandard("SSTL15")),
    ("sfp_rx_los",       2, Pins("C14"), IOStandard("SSTL15")),

    # SFP4
    ("sfp_tx", 3,
        Subsignal("p", Pins("D7")),
        Subsignal("n", Pins("C7"))
    ),
    ("sfp_rx", 3,
        Subsignal("p", Pins("D9")),
        Subsignal("n", Pins("C9"))
    ),
    ("sfp_tx_disable_n", 3, Pins("A14"), IOStandard("SSTL15")),
    ("sfp_rx_los",       3, Pins("C15"), IOStandard("SSTL15")),

    # VGA TODO:Check the IOStandard
    ("vga", 0,
        Subsignal("r", Pins("AB16 Y16 AA16 Y13 AB17")),
        Subsignal("g", Pins("D15 AB13 W14 AA14 AA13 AB12")),
        Subsignal("b", Pins("D14 E14 E13 F13 F14")),
        Subsignal("hsync_n", Pins("C13")),
        Subsignal("vsync_n", Pins("B13")),
        IOStandard("LVCMOS33")
    ),

    # SDCard.
    ("sdcard", 0,
        Subsignal("data", Pins("AA13 AB13 Y13 AA14"), Misc("PULLUP True")),
        Subsignal("cmd",  Pins("AB11"),               Misc("PULLUP True")),
        Subsignal("clk",  Pins("AB12")),
        Subsignal("cd",   Pins("F14")),
        Misc("SLEW=FAST"),
        IOStandard("LVCMOS33"),
    )

]

# Connectors ---------------------------------------------------------------------------------------

_connectors = [
    ("J11", {
         3: "B22",  4: "C22",
         5: "A20",  6: "B20",
         7: "F20",  8: "F19",
         9: "J16",  10: "F15",
        11: "F21",  12: "M17",
        13: "A21",  14: "B21",
        15: "D21",  16: "E21",
        17: "G18",  18: "G17",
        19: "H19",  20: "J19",
        21: "G16",  22: "G15",
        23: "D19",  24: "E19",
        25: "C20",  26: "D20",
        27: "A19",  28: "A18",
        29: "E18",  30: "F18",
        31: "C19",  32: "C18",
        33: "B18",  34: "B17",
        35: "C17",  36: "D17",
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
        return OpenFPGALoader("alinx_ax7201", cable)

    def do_finalize(self, fragment):
        Xilinx7SeriesPlatform.do_finalize(self, fragment)
        self.add_period_constraint(self.lookup_request("clk200", loose=True), 1e9/200e6)
