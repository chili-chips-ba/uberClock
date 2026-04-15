#!/usr/bin/env python3

import argparse
import os
import shlex
import subprocess
import sys


def log_step(step, detail):
    print(f"[demo.py] {step}: {detail}", flush=True)


def run_cmd(cmd, *, cwd=None, env=None, step):
    location = cwd or os.getcwd()
    log_step(step, f"cwd={location}")
    print(f"[demo.py] $ {shlex.join(cmd)}", flush=True)
    subprocess.run(cmd, cwd=cwd, env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description="LiteX Bare Metal UberClock App.")
    parser.add_argument(
        "--build-path",
        help="Target's build path (ex build/board_name).",
        required=True,
    )
    parser.add_argument(
        "--with-cxx",
        action="store_true",
        help="Enable CXX support.",
    )
    parser.add_argument(
        "--mem",
        default="main_ram",
        help="Memory region where code will be loaded/executed.",
    )
    parser.add_argument(
        "--data-mem",
        default="sram",
        help="Memory region where .data/.bss/heap/stack will be placed.",
    )
    args = parser.parse_args()

    app_dir = os.path.abspath(os.path.dirname(__file__))
    app_build_dir = os.path.join(app_dir, "build")
    build_path = args.build_path if os.path.isabs(args.build_path) else os.path.join("..", args.build_path)

    build_env = os.environ.copy()
    build_env["BUILD_DIR"] = build_path
    build_env["APP_BUILD_DIR"] = app_build_dir
    build_env["CODE_MEM"] = args.mem
    build_env["DATA_MEM"] = args.data_mem
    build_env["PYTHON"] = sys.executable or "python3"
    if args.with_cxx:
        build_env["WITH_CXX"] = "1"

    run_cmd(["make", "--no-print-directory"], cwd=app_dir, env=build_env, step="build")

    run_cmd(["cp", os.path.join(app_build_dir, "demo.bin"), os.path.join(app_dir, "demo.bin")], step="copy-bin")

    python3_exe = sys.executable or "python3"
    run_cmd(
        [
            python3_exe,
            "-m",
            "litex.soc.software.crcfbigen",
            os.path.join(app_build_dir, "demo.bin"),
            "-o",
            os.path.join(app_dir, "demo.fbi"),
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
