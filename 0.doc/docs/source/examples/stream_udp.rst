Export DDR3 Data over UDP
=========================

This page describes how captured DDR3 data is transferred to the host and
processed using the provided scripts.

Overview
--------

After capturing data into DDR3, it is streamed to the host using UDP.

The flow:

.. code-block:: text

   DDR3 memory
        ↓
   DMA reader
        ↓
   UDP stream
        ↓
   host receiver
        ↓
   binary reconstruction
        ↓
   analysis / plotting

---

Receiver Script
---------------

The main receiver is:

.. code-block:: text

   3.build/scripts/receive_udp_capture.py

Run it on the host:

.. code-block:: bash

   python 3.build/scripts/receive_udp_capture.py

This script:

- listens for UDP packets
- validates packet headers
- reconstructs binary data
- writes ``capture.bin``

---

Available Analysis Tools
-----------------------

The repository provides several helper scripts:

.. code-block:: text

   capture_bin_to_txt.py
   plot_capture_bin.py
   plot_capture_lanes.py
   plot_csv_fft.py
   plot_csv_fft_coherent.py
   serial_capture_to_csv.py

Each tool serves a specific purpose:

- **capture_bin_to_txt.py**
  Converts binary capture into readable numeric format

- **plot_capture_bin.py**
  Quick visualization of raw waveform

- **plot_capture_lanes.py**
  Debug multi-lane/interleaved data

- **plot_csv_fft.py**
  Frequency-domain analysis

- **plot_csv_fft_coherent.py**
  Coherent FFT (better for precise measurements)

---

Typical Workflow
----------------

.. code-block:: bash

   # 1. receive UDP data
   python 3.build/scripts/receive_udp_capture.py

   # 2. convert to text
   python 3.build/scripts/capture_bin_to_txt.py capture.bin

   # 3. plot waveform
   python 3.build/scripts/plot_capture_bin.py capture.bin

   # 4. FFT analysis
   python 3.build/scripts/plot_csv_fft.py capture.csv

---

Validation Checklist
--------------------

Before using captured data:

- verify no packet loss
- verify correct file size
- validate with ramp mode
- confirm waveform continuity

---

Why This Matters
----------------

This step makes high-speed capture usable:

- data leaves FPGA
- signal becomes visible
- analysis becomes possible

Without it, DDR3 capture cannot be inspected or validated.
