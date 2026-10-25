<!--
SPDX-FileCopyrightText: 2026 Ahmed Imamović
SPDX-FileCopyrightText: 2026 Tarik Hamedović
SPDX-License-Identifier: CC-BY-SA-4.0
-->

SoC Build and Host Tools
========================

The main SoC build flow lives under ``3.build``. That directory is the working area for building the LiteX SoC, loading it onto the board, running
the firmware/demo flow, and using the host-side utilities for capture and inspection.

Directory Structure
-------------------

``Makefile``
   Primary entry point for hardware build, FPGA loading, software build, UART
   terminal access, and editable Python package installation.

``scripts/``
   Host-side tools for capture, UDP reception, conversion, and plotting.

``build/``
   Generated LiteX build products such as ``csr.csv`` and bitstreams.

``*.csv``, ``*.txt``, ``*.bin``
   Local experimental outputs created while debugging or analyzing captures.

Core Flow
---------

The Makefile automatically selects an interpreter from the active virtual
environment, then ``./.venv``, then ``~/litex-venv``, and finally falls back to
``python3``.

Typical sequence:

.. code-block:: bash

   cd 3.build
   make install-python
   make build-board
   make build-sw
   make load
   make term PORT=/dev/ttyUSB0

Targets
-----------------

``make install-python``
   Install ``2.soc/8.python`` in editable mode so the SoC entry point can
   be run directly from the repository.

``make build-board``
   Build the gateware using ``python -m uberclock_soc`` and the selected target
   options. Defaults to ``TOOLCHAIN=vivado``; pass ``TOOLCHAIN=openxc7`` for
   the Vivado-free flow (see `openXC7 Toolchain (Vivado-free build)`_ below).

``make build-sw``
   Build the bare-metal software under ``2.soc/2.sw`` against the
   generated LiteX build tree.

``make load``
   Program the FPGA using the repo-local SoC Python entry point.

``make term``
   Open the LiteX UART terminal and load the first ``.bin`` found under
   ``2.soc/2.sw``.

Host-side scripts
-----------------

The python scripts under ``3.build/scripts`` are kept separate from the Makefile and are described below.

``serial_capture_to_csv.py``
   Serial-console capture helper. It sends the low-speed or high-speed capture
   commands, waits for memory fill to finish, requests a text dump, and writes a
   CSV file.

``receive_udp_capture.py``
   UDP receiver for the repository's ``UBD3`` DDR dump format. It reconstructs a
   binary capture, detects missing blocks, validates ramp patterns, and can
   display the resulting waveform.

``capture_bin_to_txt.py``
   Converts a binary ``capture.bin`` file into a plain-text integer sample list.

``plot_capture_bin.py``
   Quick waveform and FFT viewer for an existing binary capture.

``plot_capture_lanes.py``
   Lane-aware plotting helper for interleaved 16-lane DDR captures.

``plot_csv_fft.py``
   FFT inspector for CSV captures, intended for lower-rate serial-dump analysis.

``plot_csv_fft_coherent.py``
   Fixed-parameter FFT view for coherent-tone experiments and frequency labeling.


openXC7 Toolchain (Vivado-free build)
--------------------------------------

Besides Vivado, ``make build-board`` also supports a fully open-source
toolchain: `openXC7 <https://github.com/openXC7/toolchain-installer>`_
(Yosys + nextpnr-xilinx + prjxray). LiteX exposes this through its
``XilinxYosysNextpnrToolchain``, selected with ``TOOLCHAIN=openxc7``.

Setup (one-time)
~~~~~~~~~~~~~~~~~

The Makefile expects an openXC7 install under ``/opt/openxc7`` (or wherever
``OPENXC7_DIR`` points), with a pre-generated chip database for the target
part under ``CHIPDB`` (default ``~/openxc7-chipdb``). It exports ``PATH``,
``CHIPDB``, ``PRJXRAY_DB_DIR`` and ``NEXTPNR_XILINX_PYTHON_DIR`` for the
build automatically when ``TOOLCHAIN=openxc7`` is selected.

The openXC7 packer stage also needs ``fasm2frames``, which is *not* shipped
by the toolchain-installer binary install and is *not* the ``fasm`` PyPI
package. Install it once via:

.. code-block:: bash

   2.soc/setup_dev.sh --with-openxc7

This sparse-clones ``f4pga/prjxray``, patches its ``setup.py`` (the
``utils`` package that the ``fasm2frames`` console-script entry point
depends on is not listed there upstream), and installs it editable into the
LiteX venv.

Building the CPU-only configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cd 3.build
   make build-board TOOLCHAIN=openxc7 \
     OPTIONS='--integrated-main-ram-size=0x10000 --nextpnr-seed=42'

This builds RISC-V CPU + UART + 64 KiB integrated RAM end to end -- Yosys,
nextpnr-xilinx, and prjxray's ``fasm2frames``/``xc7frames2bit`` -- with zero
placement or routing errors, and produces a working ``.bit`` loadable through
the existing ``OpenFPGALoader``-based programmer.

The reduced 64 KiB RAM size is required, not optional: at the default
256 KiB, Yosys infers a depth-cascaded BRAM pair, and nextpnr-xilinx does not
enforce that pair's required column adjacency, producing an unroutable
placement.

Known limitations
~~~~~~~~~~~~~~~~~~

``--with-uberclock`` (the DSP core) does **not** build cleanly under openXC7.
Synthesis itself completes (after keeping hierarchy through
``synth_xilinx`` instead of flattening before optimization, and switching to
Yosys's ``abc9`` LUT mapper via ``--yosys-abc9`` to stay within LUT
capacity), but placement then hits a reproducible **nextpnr-xilinx
placement-legality bug**: its placer cannot legalize a ``CARRY4`` chain
shape that Yosys abc9's ALU-extraction pass produces for the CORDIC
pipeline's adders. This was tested across placers (``heap``, ``sa``) and
with ``-nocarry``; every combination hits an independent variant of the same
bug class. This is an upstream toolchain maturity gap, not something
fixable from this project's RTL or LiteX integration.

``--with-uberddr3``, ``--with-ethernet``, and ``--with-hdmi`` are expected to
need Vivado regardless: ``XilinxYosysNextpnrToolchain`` silently drops the
Vivado-only I/O properties UberDDR3's PHY needs (e.g. ``INTERNAL_VREF``), and
has no real static timing analysis or false-path support, so the DDR3 PHY's
400 MHz domain would be effectively unconstrained and unverified.

Use Vivado for any configuration that includes ``--with-uberclock``,
``--with-uberddr3``, ``--with-ethernet``, or ``--with-hdmi``. openXC7 is
solid for the CPU-only bring-up configuration above. See
``docs/openxc7_port/main.tex`` at the repository root for the full,
chronological debugging log: every error hit, its root cause, and its fix or
current workaround.

Firmware Memory Placement and In-place Builds
-----------------------------------------------

``make build-sw`` builds the source files directly in ``2.soc/2.sw``.
The nested ``2.soc/2.sw/uberClock`` directory is an older generated application
copy and is not used by this target. The source linker script is restored after
the build, including when a build command fails. This target uses
``SW_CODE_MEM``, ``SW_DATA_MEM`` and ``WITH_CXX`` rather than ``DEMO_FLAGS``.

.. code-block:: bash

   cd 3.build
   make build-sw SW_CODE_MEM=main_ram SW_DATA_MEM=main_ram

The default SoC keeps ``main_ram`` in 512 KiB of integrated BRAM.
External DDR3 can instead serve as CPU memory with ``--with-sdram-main-ram``.
That option is mutually exclusive with ``--with-uberddr3`` because both use
the same physical DDR3 device. Select memory regions that exist in the
generated memory map, and rebuild firmware against matching generated CSRs.
