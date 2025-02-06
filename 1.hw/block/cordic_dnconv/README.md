# CORDIC-Based Downconversion

## Overview

This block implements and tests CORDIC-based downconversion using Verilog.

### CORDIC Module (`cordic.v`)
Implements a CORDIC algorithm in **rotation mode**. When the input is set to `X = constant` and `Y = 0`, the module functions as a **Direct Digital Synthesizer (DDS)**. In this example, a **1 MHz sine wave** is synthesized.

### Downconversion using CORDIC
When the input is set to `X = 0, Y = sin(ωc t)`, the CORDIC module acts as a **mixer** for frequency downconversion. In the provided test case:
- A **1 MHz sine wave** is mixed with a **900 kHz** signal.
- After passing through a **low-pass CIC filter (`cic.v`)**, the output is a **downconverted 100 kHz signal**, sampled at a **64x decimated rate**.

## Simulation Results

The following waveform visualization demonstrates the downconversion process:

- The first set of waveforms shows the **input signals**.
- The middle waveforms display the **intermediate CORDIC outputs**.
- The final set of waveforms represents the **downconverted and filtered signal**.

---

![Screenshot from 2025-02-05 21-50-38](https://github.com/user-attachments/assets/d67a455d-1f42-43a5-8563-e40613d3d251)
