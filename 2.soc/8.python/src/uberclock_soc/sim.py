#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# SPDX-FileCopyrightText: 2026 Tarik Hamedovic
# SPDX-License-Identifier: BSD-2-Clause
"""
LiteX simulation target for the uberClock SoC integration.

This target is intentionally separate from the AX7203 board target in soc.py.
It uses a SimPlatform, simulation clocks, and simulation-only primitive models
while reusing the production uberClock CSR/core integration and canonical RTL.
"""

from __future__ import annotations

from pathlib import Path
import shutil

from migen import *
from litex.gen import *

from litex.build.generic_platform import Pins, Subsignal
from litex.build.sim import SimPlatform
from litex.build.sim.config import SimConfig
from litex.soc.integration.builder import Builder
from litex.soc.integration.common import get_boot_address, get_mem_data
from litex.soc.integration.soc_core import SoCCore
from litex.soc.cores.timer import Timer

from .rtl_filelist import UBERCLOCK_RTL_FILES
from .rtl_sources import rtl_dir
from .uberclock_core import add_uberclock_fullrate


_IO = [
    ("sys_clk", 0, Pins(1)),
    ("uc_clk",  0, Pins(1)),
    ("sys_rst", 0, Pins(1)),

    ("serial", 0,
        Subsignal("source_valid", Pins(1)),
        Subsignal("source_ready", Pins(1)),
        Subsignal("source_data",  Pins(8)),
        Subsignal("sink_valid",   Pins(1)),
        Subsignal("sink_ready",   Pins(1)),
        Subsignal("sink_data",    Pins(8)),
    ),

    ("user_led", 0, Pins(1)),
    ("user_led", 1, Pins(1)),
    ("user_led", 2, Pins(1)),
    ("user_led", 3, Pins(1)),

    ("adc_clk_ch0",  0, Pins(1)),
    ("adc_clk_ch1",  0, Pins(1)),
    ("adc_data_ch0", 0, Pins(12)),
    ("adc_data_ch1", 0, Pins(12)),

    ("da1_clk",  0, Pins(1)),
    ("da1_wrt",  0, Pins(1)),
    ("da1_data", 0, Pins(14)),
    ("da2_clk",  0, Pins(1)),
    ("da2_wrt",  0, Pins(1)),
    ("da2_data", 0, Pins(14)),
]


class Platform(SimPlatform):
    """Simulation platform with the board IOs needed by the SoC wrapper."""

    def __init__(self):
        SimPlatform.__init__(self, "SIM", _IO)


class SimCRG(LiteXModule):
    """Clock/reset generator for the SYS and uberClock simulation domains."""

    def __init__(self, platform):
        self.cd_sys = ClockDomain()
        self.cd_uc = ClockDomain()

        sys_clk = platform.request("sys_clk")
        uc_clk = platform.request("uc_clk")
        sys_rst = platform.request("sys_rst")

        self.comb += [
            ClockSignal("sys").eq(sys_clk),
            ResetSignal("sys").eq(sys_rst),
            ClockSignal("uc").eq(uc_clk),
            ResetSignal("uc").eq(sys_rst),
        ]


class SimSoC(SoCCore):
    """LiteX simulation SoC for firmware/CSR-level uberClock testing."""

    SYS_CLK_HZ = int(100e6)
    UC_CLK_HZ = int(65e6)
    INTEGRATED_MAIN_RAM_BYTES = 256 * 1024

    def __init__(
        self,
        *,
        with_uberclock: bool = False,
        sim_debug: bool = False,
        finish_after_cycles: int = 0,
        **kwargs,
    ):
        platform = Platform()

        kwargs.setdefault("integrated_main_ram_size", self.INTEGRATED_MAIN_RAM_BYTES)
        if kwargs.get("uart_name", "serial") == "serial":
            kwargs["uart_name"] = "sim"

        self.submodules.crg = SimCRG(platform)

        SoCCore.__init__(
            self,
            platform,
            clk_freq=self.SYS_CLK_HZ,
            ident="uberClock LiteX Simulation",
            **kwargs,
        )

        self.submodules.timer1 = Timer()
        self.add_csr("timer1")

        leds = Cat(*platform.request_all("user_led"))
        heartbeat = Signal(24)
        self.sync.sys += heartbeat.eq(heartbeat + 1)
        self.comb += leds[0].eq(heartbeat[-1])

        if with_uberclock:
            add_uberclock_fullrate(self, leds=leds)
            sim_model = rtl_dir() / "2.soc" / "4.sim" / "litex" / "xilinx_primitive_models.v"
            platform.add_source(str(sim_model))

        if finish_after_cycles > 0:
            sim_cycles = Signal(max=finish_after_cycles + 1)
            self.sync.sys += [
                If(sim_cycles == finish_after_cycles,
                    Finish()
                ).Else(
                    sim_cycles.eq(sim_cycles + 1)
                )
            ]

        if sim_debug:
            platform.add_debug(self, reset=1)
        else:
            self.comb += platform.trace.eq(1)


def build_main() -> None:
    """Build and optionally run the uberClock LiteX simulation."""
    from litex.build.parser import LiteXArgumentParser

    parser = LiteXArgumentParser(platform=Platform, description="uberClock LiteX simulation")

    parser.add_argument("--with-uberclock", action="store_true", help="Instantiate the uberClock RTL/CSR block.")
    parser.add_argument("--sys-clk-freq", default=SimSoC.SYS_CLK_HZ, type=float, help="SYS clock frequency.")
    parser.add_argument("--uc-clk-freq", default=SimSoC.UC_CLK_HZ, type=float, help="uberClock clock frequency.")
    parser.add_argument("--ram-init", default=None, help="Optional main RAM init file (.bin or .json).")
    parser.add_argument(
        "--finish-after-cycles",
        default=0,
        type=int,
        help="Stop simulation after this many SYS clock cycles. 0 runs until another finish condition.",
    )
    parser.add_argument("--non-interactive", action="store_true", help="Run without an interactive console.")
    parser.add_argument("--no-run", action="store_true", help="Build the Verilator simulation but do not run it.")
    parser.add_argument("--sim-debug", action="store_true", help="Enable LiteX simulation debug tracing support.")

    args = parser.parse_args()

    soc_kwargs = dict(parser.soc_argdict)
    soc_kwargs.setdefault("integrated_main_ram_size", SimSoC.INTEGRATED_MAIN_RAM_BYTES)
    if soc_kwargs.get("uart_name", "serial") == "serial":
        soc_kwargs["uart_name"] = "sim"

    ram_boot_address = None
    if args.ram_init:
        soc_kwargs["integrated_main_ram_init"] = get_mem_data(
            args.ram_init,
            data_width=32,
            endianness="little",
            offset=0x40000000,
        )
        ram_boot_address = get_boot_address(args.ram_init)

    sim_config = SimConfig()
    sim_config.add_clocker("sys_clk", freq_hz=args.sys_clk_freq)
    sim_config.add_clocker("uc_clk", freq_hz=args.uc_clk_freq)
    sim_config.add_module("serial2console", "serial")

    soc = SimSoC(
        with_uberclock=args.with_uberclock,
        sim_debug=args.sim_debug,
        finish_after_cycles=args.finish_after_cycles,
        **soc_kwargs,
    )

    if ram_boot_address is not None:
        if ram_boot_address == 0:
            ram_boot_address = soc.mem_map["main_ram"]
        soc.add_constant("ROM_BOOT_ADDRESS", ram_boot_address)

    builder = Builder(soc, **parser.builder_argdict)
    gateware_dir = Path(builder.gateware_dir)
    gateware_dir.mkdir(parents=True, exist_ok=True)
    for rel_path in UBERCLOCK_RTL_FILES:
        src = rtl_dir() / rel_path
        if src.suffix == ".mem":
            shutil.copy2(src, gateware_dir / src.name)

    builder.build(
        sim_config=sim_config,
        interactive=not args.non_interactive,
        run=not args.no_run,
        **parser.toolchain_argdict,
    )


if __name__ == "__main__":
    build_main()
