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
