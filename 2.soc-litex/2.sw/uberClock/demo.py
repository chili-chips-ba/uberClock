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
    parser.add_argument("--build-path",                      help="Target's build path (ex build/board_name).", required=True)
    parser.add_argument("--with-cxx",   action="store_true", help="Enable CXX support.")
    parser.add_argument("--mem",        default="main_ram",  help="Memory Region where code will be loaded/executed.")
    parser.add_argument("--app-dir",    default="uberClock", help="Name of the application directory to create (default: uberClock).")
    args = parser.parse_args()

    app_dir = args.app_dir

    # Create demo directory
    os.makedirs(app_dir, exist_ok=True)

    # Copy contents to demo directory
    src_dir = os.path.abspath(os.path.dirname(__file__))
    os.system(f"cp {src_dir}/* {app_dir}/")
    os.system(f"chmod -R u+w {app_dir}")  # Allow linker script modification on Nix

    # Update memory region.
    replace_in_file(f"{app_dir}/linker.ld", "main_ram", args.mem)

    # Compile demo
    build_path = args.build_path if os.path.isabs(args.build_path) else os.path.join("..", args.build_path)
    cxx_env = "export WITH_CXX=1 && " if args.with_cxx else ""
    os.system(f"export BUILD_DIR={build_path} && {cxx_env} cd {app_dir} && make")

    # Copy demo.bin
    os.system(f"cp {app_dir}/demo.bin ./")

    # Prepare flash boot image.
    python3_exe = sys.executable or "python3" # Nix specific: Reuse current Python executable if available.
    os.system(f"{python3_exe} -m litex.soc.software.crcfbigen demo.bin -o demo.fbi --fbi --little")

if __name__ == "__main__":
    main()

