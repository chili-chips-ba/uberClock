#!/usr/bin/env python3

#
# This file is part of LiteX.
#
# Copyright (c) 2020-2022 Florent Kermarrec <florent@enjoy-digital.fr>
# SPDX-License-Identifier: BSD-2-Clause

import os
import sys
import argparse

from litex.build.tools import replace_in_file

def main():
    parser = argparse.ArgumentParser(description="LiteX Bare Metal UberClock App.")
    parser.add_argument(
        "--build-path",
        help="Target's build path (ex build/board_name).",
        required=True
    )
    parser.add_argument(
        "--with-cxx",
        action="store_true",
        help="Enable CXX support."
    )
    parser.add_argument(
        "--mem",
        default="main_ram",
        help="Memory Region where code will be loaded/executed."
    )
    parser.add_argument(
        "--app-dir",
        default="uberClock",
        help="Name of the application directory to create (default: uberClock)."
    )
    args = parser.parse_args()

    app_dir = args.app_dir

    # 1) Create demo directory
    os.makedirs(app_dir, exist_ok=True)

    # 2) Copy contents (recursive!) to demo directory
    src_dir = os.path.abspath(os.path.dirname(__file__))
    os.system(f"cp -r {src_dir}/* {app_dir}/")
    os.system(f"chmod -R u+w {app_dir}")  # Allow linker script edits on Nix

    # 3) Update memory region in linker script
    replace_in_file(f"{app_dir}/linker.ld", "main_ram", args.mem)

    # 4) Patch Makefile so CRT0_SRC points at this venv’s litex
    import litex, os as _os
    vex_dir = _os.path.join(
        _os.path.dirname(litex.__file__),
        "soc", "cores", "cpu", "vexriscv"
    )
    replace_in_file(
        f"{app_dir}/Makefile",
        r"^CRT0_SRC\s*=.*",
        f"CRT0_SRC = {vex_dir}/crt0.S"
    )

    # 5) Compile demo
    build_path = (
        args.build_path
        if os.path.isabs(args.build_path)
        else os.path.join("..", args.build_path)
    )
    cxx_env = "export WITH_CXX=1 && " if args.with_cxx else ""
    os.system(
        f"export BUILD_DIR={build_path} && "
        f"{cxx_env}cd {app_dir} && make"
    )

    # 6) Copy demo.bin back to top‐level
    os.system(f"cp {app_dir}/demo.bin ./")

    # 7) Generate flash boot image
    python3_exe = sys.executable or "python3"
    os.system(
        f"{python3_exe} -m litex.soc.software.crcfbigen "
        "demo.bin -o demo.fbi --fbi --little"
    )

if __name__ == "__main__":
    main()
