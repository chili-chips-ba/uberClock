module dac_control(
    input            sys_clk,     // sys_clk from LiteX
    input            rst_n,       // active-low reset

    // new inputs from CPU/CSR:
    input  [13:0]    data1,       // DAC1 sample
    input            wrt1_en,     // DAC1 write-strobe enable
    input  [13:0]    data2,       // DAC2 sample
    input            wrt2_en,     // DAC2 write-strobe enable

    // unchanged outputs to the pins:
    output           da1_clk,
    output           da1_wrt,
    output [13:0]    da1_data,
    output           da2_clk,
    output           da2_wrt,
    output [13:0]    da2_data
);

  // half-rate clocks via ODDR
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr1_clk (
    .Q(da1_clk), .C(sys_clk), .CE(1'b1),
    .D1(1'b0),  .D2(1'b1), .R(1'b0), .S(1'b0)
  );
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr2_clk (
    .Q(da2_clk), .C(sys_clk), .CE(1'b1),
    .D1(1'b0),  .D2(1'b1), .R(1'b0), .S(1'b0)
  );

  // write strobes driven by wrt?_en CSR
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr1_wrt (
    .Q(da1_wrt), .C(sys_clk), .CE(1'b1),
    .D1(wrt1_en),.D2(wrt1_en),.R(1'b0), .S(1'b0)
  );
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr2_wrt (
    .Q(da2_wrt), .C(sys_clk), .CE(1'b1),
    .D1(wrt2_en),.D2(wrt2_en),.R(1'b0), .S(1'b0)
  );

  // data buses driven by CPU data?_in CSR
  genvar i;
  generate
    for (i = 0; i < 14; i = i + 1) begin : GEN1
      ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr1_data (
        .Q(da1_data[i]), .C(sys_clk), .CE(1'b1),
        .D1(data1[i]),   .D2(data1[i]), .R(1'b0), .S(1'b0)
      );
      ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) odr2_data (
        .Q(da2_data[i]), .C(sys_clk), .CE(1'b1),
        .D1(data2[i]),   .D2(data2[i]), .R(1'b0), .S(1'b0)
      );
    end
  endgenerate

endmodule
