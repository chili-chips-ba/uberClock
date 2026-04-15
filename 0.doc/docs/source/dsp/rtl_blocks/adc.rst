.. SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
.. SPDX-License-Identifier: CC-BY-SA-4.0

ADC
===

The ADC interface is the entry point of the sampled signal path. It captures
incoming converter data and presents it in the internal format used by the rest
of the design.

``adc/adc.v``
-------------

This module is the board-facing ADC wrapper. It forwards the sampling clock to
two ADC channels and captures incoming parallel data using DDR input
registers. In the current implementation, only the rising-edge samples are
propagated to the internal datapath.

Responsibilities
^^^^^^^^^^^^^^^^

The block performs three main tasks:

- forwards ``sys_clk`` to the external ADC channels as ``adc_clk_ch0`` and
  ``adc_clk_ch1``,
- captures DDR input data from ``adc_data_ch0`` and ``adc_data_ch1`` using
  vendor ``IDDR`` primitives,
- outputs the rising-edge samples on ``ad_data_ch0`` and ``ad_data_ch1``.

Interface summary
^^^^^^^^^^^^^^^^^

System signals:

- ``sys_clk`` -- system clock used for ADC clock forwarding and input capture,
- ``rst_n`` -- active-low reset input, currently unused by the implementation.

ADC-side signals:

- ``adc_clk_ch0`` -- forwarded ADC clock for channel 0,
- ``adc_clk_ch1`` -- forwarded ADC clock for channel 1,
- ``adc_data_ch0[11:0]`` -- 12-bit ADC data bus for channel 0,
- ``adc_data_ch1[11:0]`` -- 12-bit ADC data bus for channel 1,
- ``ad_data_ch0[11:0]`` -- captured output data for channel 0,
- ``ad_data_ch1[11:0]`` -- captured output data for channel 1.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

Each ADC input bit is connected to an ``IDDR`` primitive configured with
``DDR_CLK_EDGE = "SAME_EDGE_PIPELINED"``. This produces two sampled values:

- ``Q1`` -- rising-edge sampled data,
- ``Q2`` -- falling-edge sampled data.

Although both rising-edge and falling-edge samples are captured internally, the
current module exports only the rising-edge values:

.. code-block:: verilog

   assign ad_data_ch0 = adc0_rising;
   assign ad_data_ch1 = adc1_rising;

The forwarded ADC clocks are generated using ``ODDR`` primitives configured to
emit a 50% duty-cycle clock derived from ``sys_clk``.

RTL source
^^^^^^^^^^

.. code-block:: verilog

   // SPDX-FileCopyrightText: 2026 Ahmed Imamovic Tarik Hamedovic
   // SPDX-License-Identifier: CC-BY-SA-4.0

   `timescale 1ns/1ps
   `default_nettype none

   module adc(
      // sys
      input  wire        sys_clk,        // system clock
      input  wire        rst_n,          // active-low reset (unused here)

      // ADC
      output wire        adc_clk_ch0,    // AD channel 0 sampling clock
      output wire        adc_clk_ch1,    // AD channel 1 sampling clock
      input  wire [11:0] adc_data_ch0,   // AD channel 0 data
      input  wire [11:0] adc_data_ch1,   // AD channel 1 data
      output wire [11:0] ad_data_ch0,    // single-edge (rising) captured data
      output wire [11:0] ad_data_ch1
   );

       wire [11:0] adc0_rising,  adc0_falling;
       wire [11:0] adc1_rising,  adc1_falling;

       genvar i;
       generate
         for (i = 0; i < 12; i = i + 1) begin : gen_iddr_ch0
           IDDR #(
             .DDR_CLK_EDGE ("SAME_EDGE_PIPELINED"),
             .INIT_Q1      (1'b0),
             .INIT_Q2      (1'b0)
           ) iddr_ch0_i (
             .Q1 (adc0_rising[i]),
             .Q2 (adc0_falling[i]),
             .C  (sys_clk),
             .CE (1'b1),
             .D  (adc_data_ch0[i]),
             .R  (1'b0),
             .S  (1'b0)
           );
         end
         for (i = 0; i < 12; i = i + 1) begin : gen_iddr_ch1
           IDDR #(
             .DDR_CLK_EDGE ("SAME_EDGE_PIPELINED"),
             .INIT_Q1      (1'b0),
             .INIT_Q2      (1'b0)
           ) iddr_ch1_i (
             .Q1 (adc1_rising[i]),
             .Q2 (adc1_falling[i]),
             .C  (sys_clk),
             .CE (1'b1),
             .D  (adc_data_ch1[i]),
             .R  (1'b0),
             .S  (1'b0)
           );
         end
       endgenerate

       // Generate forwarded ADC clocks (50% duty) from sys_clk
       ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) ODDR_clk_ch0 (
         .Q (adc_clk_ch0), .C(sys_clk), .CE(1'b1),
         .D1(1'b1),        .D2(1'b0),   .R(1'b0), .S(1'b0)
       );
       ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) ODDR_clk_ch1 (
         .Q (adc_clk_ch1), .C(sys_clk), .CE(1'b1),
         .D1(1'b1),        .D2(1'b0),   .R(1'b0), .S(1'b0)
       );

       // Choose rising-edge samples (or combine rising/falling if desired)
       assign ad_data_ch0 = adc0_rising;
       assign ad_data_ch1 = adc1_rising;

   endmodule

   `default_nettype wire
