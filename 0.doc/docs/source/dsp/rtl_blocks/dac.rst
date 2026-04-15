.. SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
.. SPDX-License-Identifier: CC-BY-SA-4.0

DAC
===

The DAC interface is the output-side converter block of the reusable DSP
datapath. It takes two parallel 14-bit input words and drives them to two DAC
channels using DDR output primitives.

``dac/dac.v``
-------------

This module is the main board-facing DAC wrapper. It accepts two 14-bit input
words, named ``data1`` and ``data2``, and presents them to two DAC channels as
DDR-driven outputs.

In the current design, the intended mapping is:

- ``data1`` -- first DAC channel input,
- ``data2`` -- second DAC channel input.

Responsibilities
^^^^^^^^^^^^^^^^

The block performs the following tasks:

- generates forwarded DAC clocks on ``da1_clk`` and ``da2_clk``,
- generates DAC write strobes on ``da1_wrt`` and ``da2_wrt``,
- drives the 14-bit DAC data buses ``da1_data`` and ``da2_data``,
- uses vendor ``ODDR`` primitives to place output timing directly at the FPGA
  pins.

Interface summary
^^^^^^^^^^^^^^^^^

System signals:

- ``sys_clk`` -- system clock used to generate the forwarded DAC clocks,
  write strobes, and data outputs,
- ``rst_n`` -- active-low reset input, currently unused by the implementation.

Input data:

- ``data1[13:0]`` -- 14-bit sample word for DAC channel 1,
- ``data2[13:0]`` -- 14-bit sample word for DAC channel 2.

DAC-side outputs:

- ``da1_clk`` -- forwarded clock for DAC channel 1,
- ``da1_wrt`` -- write strobe for DAC channel 1,
- ``da1_data[13:0]`` -- 14-bit output data bus for DAC channel 1,
- ``da2_clk`` -- forwarded clock for DAC channel 2,
- ``da2_wrt`` -- write strobe for DAC channel 2,
- ``da2_data[13:0]`` -- 14-bit output data bus for DAC channel 2.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

The module uses ``ODDR`` primitives for all converter-facing outputs.

Clock and write-strobe generation
"""""""""""""""""""""""""""""""""

For each DAC channel, the forwarded clock and write strobe are generated using
an ``ODDR`` configured with:

.. code-block:: verilog

   .D1(1'b0),
   .D2(1'b1)

This produces a repeating DDR output pattern derived from ``sys_clk`` and is
used to create the required converter-side timing signals.

Data output generation
""""""""""""""""""""""

Each bit of the 14-bit output bus is driven by its own ``ODDR`` instance.
Unlike a true dual-edge serializer carrying different values on rising and
falling edges, this implementation drives the same value on both edges:

.. code-block:: verilog

   .D1(data1[bit]),
   .D2(data1[bit])

for channel 1, and similarly for channel 2.

As a result, each DAC data bit remains logically constant across the full DDR
cycle, while still being emitted through dedicated output DDR resources. This
approach is useful when the interface timing must be aligned with DDR-style
converter clocks and strobes, but the payload itself is updated once per
``sys_clk`` cycle.

Structure
"""""""""

The module is organized into three parts:

- ``ODDR`` generation of ``da1_clk`` and ``da2_clk``,
- ``ODDR`` generation of ``da1_wrt`` and ``da2_wrt``,
- per-bit ``ODDR`` generation for ``da1_data`` and ``da2_data``.

RTL source
^^^^^^^^^^

.. code-block:: verilog

   // ============================================================================
   //  dac.v
   //
   //  This module takes two 14-bit words (data1 = sine, data2 = cosine) and
   //  drives them out via DDR ODDR cells on da?_clk, da?_wrt, da?_data.
   // ============================================================================

   module dac (
       input  wire        sys_clk,
       input  wire        rst_n,

       // 14-bit input words
       input  wire [13:0] data1,
       input  wire [13:0] data2,

       // DDR outputs for DAC #1
       output wire        da1_clk,    // half-rate clock strobe
       output wire        da1_wrt,    // write strobe (always “1” on positive edge)
       output wire [13:0] da1_data,   // 14-bit data bus

       // DDR outputs for DAC #2
       output wire        da2_clk,    // half-rate clock strobe
       output wire        da2_wrt,    // write strobe (always “1” on positive edge)
       output wire [13:0] da2_data    // 14-bit data bus
   );

     genvar bit;

     //----------------------------------------------------------------------------
     // Clocks & write strobes (always toggling 0→1 on SYS_CLK via ODDR)
     //----------------------------------------------------------------------------
     ODDR #(
       .DDR_CLK_EDGE("SAME_EDGE")
     ) oddr_clk1 (
       .Q   (da1_clk),
       .C   (sys_clk),
       .CE  (1'b1),
       .D1  (1'b0),
       .D2  (1'b1),
       .R   (1'b0),
       .S   (1'b0)
     );

     ODDR #(
       .DDR_CLK_EDGE("SAME_EDGE")
     ) oddr_wrt1 (
       .Q   (da1_wrt),
       .C   (sys_clk),
       .CE  (1'b1),
       .D1  (1'b0),
       .D2  (1'b1),
       .R   (1'b0),
       .S   (1'b0)
     );

     ODDR #(
       .DDR_CLK_EDGE("SAME_EDGE")
     ) oddr_clk2 (
       .Q   (da2_clk),
       .C   (sys_clk),
       .CE  (1'b1),
       .D1  (1'b0),
       .D2  (1'b1),
       .R   (1'b0),
       .S   (1'b0)
     );

     ODDR #(
       .DDR_CLK_EDGE("SAME_EDGE")
     ) oddr_wrt2 (
       .Q   (da2_wrt),
       .C   (sys_clk),
       .CE  (1'b1),
       .D1  (1'b0),
       .D2  (1'b1),
       .R   (1'b0),
       .S   (1'b0)
     );

     //----------------------------------------------------------------------------
     // DDR ODDR for each data bit: channel1 = data1, channel2 = data2
     //----------------------------------------------------------------------------
     generate
       for (bit = 0; bit < 14; bit = bit + 1) begin : DAC1_DATA
         ODDR #(
           .DDR_CLK_EDGE("SAME_EDGE")
         ) oddr_d1 (
           .Q   (da1_data[bit]),
           .C   (sys_clk),
           .CE  (1'b1),
           .D1  (data1[bit]),
           .D2  (data1[bit]),
           .R   (1'b0),
           .S   (1'b0)
         );
       end
     endgenerate

     generate
       for (bit = 0; bit < 14; bit = bit + 1) begin : DAC2_DATA
         ODDR #(
           .DDR_CLK_EDGE("SAME_EDGE")
         ) oddr_d2 (
           .Q   (da2_data[bit]),
           .C   (sys_clk),
           .CE  (1'b1),
           .D1  (data2[bit]),
           .D2  (data2[bit]),
           .R   (1'b0),
           .S   (1'b0)
         );
       end
     endgenerate

   endmodule
