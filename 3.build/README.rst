<!--
SPDX-FileCopyrightText: 2026 Ahmed Imamović
SPDX-FileCopyrightText: 2026 Tarik Hamedović
SPDX-License-Identifier: CC-BY-SA-4.0
-->

Build and Host Tools
====================

The ``3.build`` directory contains the repo-local build entry point for the
LiteX SoC flow, plus the host-side utilities used to capture, inspect, and plot
data produced by the hardware.

Layout
------

``Makefile``
   Main build interface for gateware generation, FPGA loading, software build,
   UART terminal access, and editable installation of the SoC Python package.

``scripts/``
   Host-side Python utilities used after a build to capture DDR data, receive
   UDP dumps, convert binary captures, and inspect results visually.

``build/``
   Generated LiteX output. This is created by ``make build-board`` and contains
   products such as ``csr.csv``, bitstreams, generated software support files,
   and board-specific build directories.

``*.csv``, ``*.txt``, ``*.bin``
   Local capture outputs or scratch analysis files. These are not the canonical
   source of the build flow; they are user-generated artifacts from experiments.

Common Make Targets
-------------------

.. code-block:: bash

   make print-python
   make install-python
   make build-board
   make build-sw
   make load
   make term

The Makefile auto-selects a Python interpreter in this order:

1. the active ``$VIRTUAL_ENV``,
2. ``./.venv``,
3. ``~/litex-venv``,
4. plain ``python3``.

Typical Workflow
----------------

1. Install the SoC Python package into the selected environment:

   .. code-block:: bash

      make install-python

2. Build the FPGA image:

   .. code-block:: bash

      make build-board FREQ=100e6 OPTIONS="--with-uberddr3 --with-uberclock --with-ethernet"

3. Build the bare-metal software against the generated LiteX build tree:

   .. code-block:: bash

      make build-sw

4. Program the board and open the UART console:

   .. code-block:: bash

      make load
      make term PORT=/dev/ttyUSB0

openXC7 Toolchain (Vivado-free build)
--------------------------------------

``make build-board`` also supports `openXC7 1.0.0
<https://github.com/openXC7/toolchain-installer>`_ (Yosys 0.69 +
nextpnr-himbaechel + prjxray) as a fully open-source alternative to Vivado,
via ``TOOLCHAIN=openxc7``:

.. code-block:: bash

   make build-board TOOLCHAIN=openxc7 \
     OPTIONS='--integrated-main-ram-size=0x10000 --nextpnr-seed=1'

One-time setup first: ``2.soc/setup_dev.sh --with-openxc7``, copy
``2.soc/6.migen/platforms/alinx_ax7203.py`` into litex-boards, build the
openXC7 1.0.0 toolchain, and generate ``~/openxc7-chipdb/xc7a200tfbg484.bin``.
Use ``OPENXC7_DIR=...`` / ``CHIPDB=...`` if they are not in the default
locations (``/opt/openxc7``, ``~/openxc7-chipdb``). The full step-by-step
instructions are in ``0.doc/docs/source/build/build_soc.rst`` and the
repository ``README.md``.

This builds RISC-V CPU + UART + 64 KiB integrated RAM end to end, and with
seed 1 meets 100 MHz timing. Other seeds may not: check the last
``Max frequency`` line for ``s7mmcm0_clkout_buf``, because a bitstream is
written even when timing fails.

``--with-uberclock`` does **not** build under openXC7 1.0.0: synthesis
completes, but nextpnr cannot legally place a CARRY4 from the CORDIC
pipeline's adders, the same upstream bug as in the older nextpnr-xilinx.
``--with-uberddr3``, ``--with-ethernet`` and ``--with-hdmi`` are expected to
need Vivado regardless. Use Vivado for any of those.

Full debugging log, root causes, and fixes:
``docs/openxc7_port/main.tex`` at the repository root.

Scripts
-------

``scripts/serial_capture_to_csv.py``
   Serial-console helper that starts a low-speed or high-speed capture on the
   board, waits for completion, requests a text dump, and writes the result as a
   CSV file.

``scripts/receive_udp_capture.py``
   UDP receiver for the repository's ``UBD3`` framed DDR dump stream. It
   reconstructs ``capture.bin``, reports missing blocks, validates ramp data,
   and can show an interactive plot.

``scripts/capture_bin_to_txt.py``
   Converts ``capture.bin`` into a plain text sample listing for quick manual
   inspection.

``scripts/plot_capture_bin.py``
   Quick time-domain and optional FFT viewer for an existing ``capture.bin``.

``scripts/plot_capture_lanes.py``
   Lane-aware waveform viewer for 16-lane capture files. This is the better
   choice when the DDR dump contains interleaved lane data.

``scripts/plot_csv_fft.py``
   FFT inspection helper for CSV captures, using a linear-amplitude spectrum.

``scripts/plot_csv_fft_coherent.py``
   Variant FFT viewer for coherent-tone experiments with fixed window and
   labeling assumptions.

Example Host-Side Capture Flow
------------------------------

Receive a DDR dump over UDP and inspect one lane:

.. code-block:: bash

   python 3.build/scripts/receive_udp_capture.py
   python 3.build/scripts/plot_capture_lanes.py capture.bin --lane 0 --decim 16

Capture through the UART command interface and inspect the resulting CSV:

.. code-block:: bash

   python 3.build/scripts/serial_capture_to_csv.py low capture.csv
   python 3.build/scripts/plot_csv_fft.py capture.csv

Guidance
--------

- Put reusable automation or analysis helpers in ``scripts/``.
- Keep generated build output inside ``build/``.
- Treat ad hoc CSV, TXT, BIN, and PNG files in this directory as experiment
  artifacts, not as part of the canonical source tree.
