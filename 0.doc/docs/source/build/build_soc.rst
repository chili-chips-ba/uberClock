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
   options.

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
