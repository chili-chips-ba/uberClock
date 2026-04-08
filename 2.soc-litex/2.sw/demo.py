#!/usr/bin/env python3

#
# This file is part of LiteX.
#
# Copyright (c) 2020-2022 Florent Kermarrec <florent@enjoy-digital.fr>
# SPDX-License-Identifier: BSD-2-Clause

import os
import sys
import shlex
import glob
import argparse
import subprocess

from litex.build.tools import replace_in_file


def log_step(step, detail):
    print(f"[demo.py] {step}: {detail}", flush=True)


def run_cmd(cmd, *, cwd=None, env=None, step):
    location = cwd or os.getcwd()
    log_step(step, f"cwd={location}")
    print(f"[demo.py] $ {shlex.join(cmd)}", flush=True)
    subprocess.run(cmd, cwd=cwd, env=env, check=True)


def remove_patterns(root_dir, patterns, *, step):
    log_step(step, f"removing generated files from {root_dir}")
    for pattern in patterns:
        for path in glob.glob(os.path.join(root_dir, pattern)):
            if os.path.isfile(path):
                os.remove(path)

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
        "--data-mem",
        default="sram",
        help="Memory Region where .data,.bss,  heap, stack will be placed."
    )
    parser.add_argument(
        "--app-dir",
        default="uberClock",
        help="Name of the application directory to create (default: uberClock)."
    )
    args = parser.parse_args()

    app_dir = args.app_dir

    # 1) Create demo directory
    log_step("prepare", f"creating app directory {app_dir}")
    os.makedirs(app_dir, exist_ok=True)

    # 2) Copy contents (recursive!) to demo directory
    src_dir = os.path.abspath(os.path.dirname(__file__))
    rsync_cmd = [
        "rsync",
        "-a",
        "--delete",
        f"--exclude={app_dir}/",
        "--exclude=*.o",
        "--exclude=*.d",
        "--exclude=*.elf",
        "--exclude=*.elf.map",
        "--exclude=*.bin",
        "--exclude=*.fbi",
        "--exclude=__pycache__/",
        f"{src_dir}/",
        f"{app_dir}/",
    ]
    run_cmd(
        rsync_cmd,
        step="sync",
    )
    run_cmd(["chmod", "-R", "u+w", app_dir], step="permissions")
    remove_patterns(
        app_dir,
        [
            "*.o",
            "*.d",
            "*.elf",
            "*.elf.map",
            "*.bin",
            "*.fbi",
            ".*.swp",
        ],
        step="cleanup",
    )

    # 3) Update memory region in linker script
    log_step("patch", f"updating linker memory regions in {app_dir}/linker.ld")
    replace_in_file(f"{app_dir}/linker.ld", "main_ram", args.mem)
    replace_in_file(f"{app_dir}/linker.ld", "data_ram", args.data_mem)

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
    build_env = os.environ.copy()
    build_env["BUILD_DIR"] = build_path
    if args.with_cxx:
        build_env["WITH_CXX"] = "1"
    run_cmd(["make", "--no-print-directory"], cwd=app_dir, env=build_env, step="build")

    # 6) Copy demo.bin back to top‐level
    run_cmd(["cp", f"{app_dir}/demo.bin", "./"], step="copy")

    # 7) Generate flash boot image
    python3_exe = sys.executable or "python3"
    run_cmd(
        [
            python3_exe,
            "-m",
            "litex.soc.software.crcfbigen",
            "demo.bin",
            "-o",
            "demo.fbi",
            "--fbi",
            "--little",
        ],
        step="pack",
    )

if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as exc:
        print(f"[demo.py] failed: command exited with status {exc.returncode}", file=sys.stderr, flush=True)
        sys.exit(exc.returncode)
