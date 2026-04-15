Low-Speed Capture (CSR-Based)
=============================

This page documents the low-speed capture path used for quick inspection and
debugging directly from firmware.

Overview
--------

Low-speed capture uses a small on-chip buffer that can be accessed via CSRs.
It is designed for:

- quick validation of signals,
- debugging DSP stages,
- inspecting small windows of data.

It does **not** use DDR memory or DMA.

Architecture
------------

.. code-block:: text

   DSP / debug source
          ↓
     capture buffer (small)
          ↓
        CSRs
          ↓
       CPU / UART

Key characteristics:

- runs in the system domain
- limited depth (~2k samples)
- CPU-readable

---

What the Hardware Does
----------------------

1. Firmware selects capture source
2. Firmware arms capture
3. Hardware fills the capture buffer
4. Firmware reads samples via CSR

---

Key CSRs / Controls
-------------------

- ``capture_arm``
  Starts a capture

- ``capture_done``
  Indicates capture completion

- ``capture_data``
  Read samples

---

Typical Workflow
----------------

.. code-block:: text

   cap_arm
   cap_status
   cap_dump

Steps:

1. Arm capture
2. Wait for completion
3. Dump samples

---

Use Cases
---------

Low-speed capture is useful for:

- debugging signal correctness
- verifying DSP stages
- checking polarity / scaling
- observing transient behavior

---

Limitations
-----------

- very small buffer
- CPU readout is slow
- cannot capture high-rate continuous signals

---

When to Use
-----------

Use low-speed capture when:

- debugging logic
- validating small datasets
- working interactively over UART

Use high-speed capture (DDR3) when:

- analyzing real signals
- capturing long sequences
- working at full sample rate
