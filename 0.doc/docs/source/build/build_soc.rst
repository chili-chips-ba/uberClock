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

Besides Vivado, ``make build-board`` supports a fully open-source toolchain:
`openXC7 <https://github.com/openXC7/toolchain-installer>`_ **1.0.0** --
Yosys 0.69, ``nextpnr-himbaechel`` (`openXC7/nextpnr
<https://github.com/openXC7/nextpnr>`_ 1.0.0) and prjxray's
``fasm2frames``/``xc7frames2bit``. LiteX drives it through its
``XilinxYosysNextpnrToolchain``, selected with ``TOOLCHAIN=openxc7``.

The full debugging log -- every error hit, its root cause, and its fix -- is
``docs/openxc7_port/main.tex`` at the repository root. Its "Re-test with
openXC7 1.0.0" section is the current reference.

Status
~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Configuration
     - Result
   * - RISC-V CPU + UART + 64 KiB integrated RAM
     - Builds a ``.bit`` in 1--2 minutes; seed 1 meets 100 MHz timing
   * - ``--with-uberclock`` (DSP core)
     - Fails: nextpnr placement-legality bug (see `Known problems`_)
   * - ``--with-uberddr3``, ``--with-ethernet``, ``--with-hdmi``
     - Untested; expected to need Vivado

The openXC7 1.0.0 bitstreams have not yet been tested on hardware.

Install and build, step by step
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tested on Ubuntu. Run the commands from the repository root.

1. **LiteX environment.** Creates ``~/litex-venv`` and installs
   ``fasm2frames`` (from ``f4pga/prjxray``) into it:

   .. code-block:: bash

      2.soc/setup_dev.sh --with-openxc7

   Add ``--no-gcc`` if a RISC-V GCC is already installed or you have no
   ``sudo``. Otherwise the script stops at the failing ``apt install``
   before it reaches the openXC7 step.

2. **Copy the project's board file into litex-boards.** It defines the
   ADC/DAC pins and accepts a ``toolchain=`` argument; the stock file does
   neither. Repeat after every ``litex_setup.py --update``.

   .. code-block:: bash

      cp 2.soc/6.migen/platforms/alinx_ax7203.py \
         ~/litex/litex-boards/litex_boards/platforms/alinx_ax7203.py

3. **Build openXC7 1.0.0 from source:**

   .. code-block:: bash

      git clone --branch 1.0.0 https://github.com/openXC7/toolchain-installer
      cd toolchain-installer
      ./toolchain-sources-builder.sh all     # installs into /opt/openxc7
      # without sudo:
      # INSTALL_PREFIX=$HOME/opt/openxc7-1.0 ./toolchain-sources-builder.sh all

4. **Generate the chip database once.** LiteX cannot generate it itself with
   1.0.0 (its generator, ``bbaexport.py``, was removed). This takes about
   2.5 minutes and 7 GB of RAM:

   .. code-block:: bash

      P=/opt/openxc7                     # or your INSTALL_PREFIX
      source $P/export.sh                # 1.0.0 python3 and bbasm first on PATH
      mkdir -p ~/openxc7-chipdb
      python3 $P/share/nextpnr/himbaechel/uarch/xilinx/gen/xilinx_gen.py \
        --xray $P/share/nextpnr/prjxray-db/artix7 \
        --device xc7a200t --bba /tmp/xc7a200t.bba
      bbasm -l /tmp/xc7a200t.bba ~/openxc7-chipdb/xc7a200tfbg484.bin
      rm /tmp/xc7a200t.bba

   ``--device`` is the fabric (``xc7a200t``), not the part. The output must
   be named ``xc7a200tfbg484.bin``: LiteX looks for that name, and the
   ``nextpnr-xilinx`` compatibility shim derives nextpnr's ``--device`` from
   it.

5. **Build the CPU-only SoC:**

   .. code-block:: bash

      source ~/litex-venv/bin/activate
      cd 3.build
      make build-board TOOLCHAIN=openxc7 \
        OPTIONS='--integrated-main-ram-size=0x10000 --nextpnr-seed=1'

   The Makefile exports ``PATH``, ``CHIPDB``, ``PRJXRAY_DB_DIR`` and
   ``NEXTPNR_XILINX_PYTHON_DIR`` from ``OPENXC7_DIR`` (default
   ``/opt/openxc7``) and ``CHIPDB`` (default ``~/openxc7-chipdb``). For other
   locations add e.g. ``OPENXC7_DIR=$HOME/opt/openxc7-1.0``. In zsh, pass
   these as separate words or ``export`` them; putting them all in one
   variable and writing ``make build-board $VARS`` does not split them.

6. **Check timing before using the bitstream.** LiteX lets nextpnr write a
   bitstream even when timing fails, so check the last
   ``Max frequency for clock 's7mmcm0_clkout_buf'`` line of the output. If
   it says ``FAIL``, try another ``--nextpnr-seed`` or lower the clock with
   ``FREQ=90e6``. The bitstream is
   ``3.build/build/alinx_ax7203/gateware/alinx_ax7203.bit``; program it with
   ``make load``.

Builds regenerate files under ``3.build/build/alinx_ax7203/`` that are
tracked in git; restore them with ``git checkout -- 3.build/build`` before
committing.

What was changed to make it work
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- ``soc.py`` builds the platform through ``_make_platform()``. It falls back
  to stock litex-boards (enough for CPU-only builds) and uses the dash-free
  part name ``xc7a200tfbg484-2`` that prjxray-db, the chip database and
  nextpnr expect. LiteX only maps a couple of parts to that spelling itself.
- ``soc.py`` and ``clocking.py`` only instantiate ``IDELAYCTRL`` when DDR3 or
  Ethernet uses I/ODELAYs; nextpnr-himbaechel rejects an unused one
  (``ERROR: Found IDELAYCTRL but no I/ODELAYs``).
- The ``OPENXC7_PLACER`` override in ``soc.py`` now reaches nextpnr. Current
  LiteX passes no ``--placer`` option, so the old text replacement did
  nothing.
- ``3.build/Makefile`` uses the 1.0.0 install layout
  (``share/nextpnr/prjxray-db``, ``lib/python``).

Known problems
~~~~~~~~~~~~~~

- ``--with-uberclock`` **does not build.** Synthesis completes, and the
  design fits (70% LUTs, 48% ``CARRY4``), but nextpnr cannot legally place a
  ``CARRY4`` from a CORDIC stage adder::

     ERROR: Unable to find legal placement for cell
       '...uberclock.tx_5.cordic_up.cordic_stages[17]...carry4...' of type 'CARRY4'

  ``OPENXC7_NOCARRY=1`` moves the same failure to a flip-flop cell, and
  ``OPENXC7_PLACER=sa`` stalls in initial placement. The old nextpnr-xilinx
  had the same bug; openXC7 1.0.0 does not fix it.
- **Timing at 100 MHz is marginal.** Sys-clock Fmax across seeds ranged from
  86 to 107 MHz and only seed 1 passed. The critical path is inside VexRiscv
  and is mostly routing delay.
- **64 KiB integrated RAM is required.** With the default size, the old
  toolchain placed a cascaded BRAM pair in non-adjacent columns
  (unroutable). This has not been re-tested with 1.0.0.
- ``--with-uberddr3``, ``--with-ethernet`` and ``--with-hdmi`` are
  expected to need Vivado: LiteX drops Vivado-only I/O properties such as
  ``INTERNAL_VREF`` for this toolchain, and there is no false-path support
  for the 400 MHz DDR3 PHY domain.

Use Vivado for any configuration that includes ``--with-uberclock``,
``--with-uberddr3``, ``--with-ethernet`` or ``--with-hdmi``.

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
