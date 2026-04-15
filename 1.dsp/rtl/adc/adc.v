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
