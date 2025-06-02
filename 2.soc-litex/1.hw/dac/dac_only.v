module dac_only(
    input            sys_clk,     // single-ended sys clock from LiteX
    input            rst_n,       // active-low reset

    output           da1_clk,     // DAC1 clock
    output           da1_wrt,     // DAC1 write-strobe
    output [13:0]    da1_data,    // DAC1 data bus
    output           da2_clk,     // DAC2 clock
    output           da2_wrt,     // DAC2 write-strobe
    output [13:0]    da2_data     // DAC2 data bus
);

  // DAC clocks: toggle 0→1 every cycle → half-rate square wave
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr_dac1_clk (
    .Q (da1_clk), .C(sys_clk), .CE(1'b1),
    .D1(1'b0), .D2(1'b1), .R(1'b0), .S(1'b0)
  );
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr_dac2_clk (
    .Q (da2_clk), .C(sys_clk), .CE(1'b1),
    .D1(1'b0), .D2(1'b1), .R(1'b0), .S(1'b0)
  );

  // Write-strobes held high
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr_dac1_wrt (
    .Q (da1_wrt), .C(sys_clk), .CE(1'b1),
    .D1(1'b1), .D2(1'b1), .R(1'b0), .S(1'b0)
  );
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr_dac2_wrt (
    .Q (da2_wrt), .C(sys_clk), .CE(1'b1),
    .D1(1'b1), .D2(1'b1), .R(1'b0), .S(1'b0)
  );

  // Data buses full-scale (all ones)
  genvar i;
  generate
    for (i = 0; i < 14; i = i + 1) begin : GEN_DAC1_DATA
      ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr1 (
        .Q (da1_data[i]), .C(sys_clk), .CE(1'b1),
        .D1(1'b1), .D2(1'b1), .R(1'b0), .S(1'b0)
      );
    end
    for (i = 0; i < 14; i = i + 1) begin : GEN_DAC2_DATA
      ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr2 (
        .Q (da2_data[i]), .C(sys_clk), .CE(1'b1),
        .D1(1'b1), .D2(1'b1), .R(1'b0), .S(1'b0)
      );
    end
  endgenerate

endmodule
