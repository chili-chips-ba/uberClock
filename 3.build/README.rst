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
