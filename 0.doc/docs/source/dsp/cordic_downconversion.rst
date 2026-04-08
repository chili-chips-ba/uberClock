CORDIC-Based Downconversion
===========================

Overview
--------

This block implements and tests CORDIC-based downconversion in Verilog.

CORDIC Module
-------------

The ``cordic.v`` module implements the CORDIC algorithm in rotation mode. When
configured with ``X = constant`` and ``Y = 0``, it behaves as a direct digital
synthesizer and can generate a 1 MHz sine wave in the documented example.

Downconversion Flow
-------------------

When configured with ``X = 0`` and ``Y = sin(omega_c t)``, the same CORDIC
module acts as a mixer for downconversion. In the documented test case:

- A 1 MHz sine wave is mixed with a 900 kHz signal.
- The mixed signal passes through the ``cic.v`` low-pass CIC filter.
- The result is a 100 kHz downconverted signal sampled at a 64x decimated rate.

Simulation Results
------------------

The original README includes a waveform capture of the input signals, the
intermediate CORDIC outputs, and the final downconverted filtered signal.

Waveform capture:

- `Simulation screenshot <https://github.com/user-attachments/assets/d67a455d-1f42-43a5-8563-e40613d3d251>`_
