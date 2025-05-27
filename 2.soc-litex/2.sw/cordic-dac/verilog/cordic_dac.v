module cordic_dac(
  input  wire        sys_clk,
  input  wire        rst_n,       // active-low reset
  input  wire [18:0] phase_inc,


  // DAC channel 1 - sine
  output wire        da1_clk,
  output wire        da1_wrt,
  output wire [13:0] da1_data,
  // DAC channel 2 - cosine
  output wire        da2_clk,
  output wire        da2_wrt,
  output wire [13:0] da2_data
);


  //-------------------------------------------------------------------------
  // 1) Phase accumulator (19-bit) for ~1 kHz sine/cosine
  //-------------------------------------------------------------------------
  localparam integer PW         = 19;
  //localparam [PW-1:0] PHASE_INC = 19'd8;  // = round(2^19 * 1e3/65e6)
  reg [PW-1:0] phase;
  always @(posedge sys_clk or negedge rst_n) begin
    if (!rst_n)    phase <= 0;
    else           phase <= phase + phase_inc;
  end

  //-------------------------------------------------------------------------
  // 3) CORDIC: rotate (1.0,0.0) by 'phase'
  //-------------------------------------------------------------------------
  localparam IW = 12, OW = 12, NSTAGES = 15, WW = 15;
  wire signed [OW-1:0] cordic_cos, cordic_sin;
  wire                 cordic_aux;
  cordic #(
    .IW(IW), .OW(OW), .NSTAGES(NSTAGES), .WW(WW), .PW(PW)
  ) sine_cos_cordic (
    .i_clk   (sys_clk),
    .i_reset (!locked),
    .i_ce    (1'b1),
    .i_xval  ($signed({1'b0, {IW-1{1'b1}}})), // +2047 ≈ 1.0
    .i_yval  ($signed(0)),
    .i_phase (phase),
    .i_aux   (1'b1),
    .o_xval  (cordic_cos),
    .o_yval  (cordic_sin),
    .o_aux   (cordic_aux)
  );

  //-------------------------------------------------------------------------
  // 4) Scale 12→14 bits and register
  //-------------------------------------------------------------------------
  reg signed [13:0] shift_sin, shift_cos;
    reg               aux_d1;

    always @(posedge sys_clk or negedge rst_n) begin
      if (!rst_n) begin
        shift_sin <= 0;
        shift_cos <= 0;
        aux_d1    <= 1'b0;
      end else begin
        aux_d1    <= cordic_aux;                    // delay the valid flag
        if (cordic_aux) begin
          shift_sin <= cordic_sin <<< 2;            // -8192…+8191
          shift_cos <= cordic_cos <<< 2;
        end
      end
    end

// 4b) Stage-2: add mid-scale offset when the shifted value is ready
reg [13:0] sin_reg, cos_reg;

always @(posedge sys_clk or negedge rst_n) begin
  if (!rst_n) begin
    sin_reg <= 14'd8192;                        // mid-scale on reset
    cos_reg <= 14'd8192;
  end else if (aux_d1) begin                   // use delayed flag
    sin_reg <= shift_sin + 14'd8192;            // 0 … 16383
    cos_reg <= shift_cos + 14'd8192;
  end
end

  //-------------------------------------------------------------------------
  // 5) DDR-output for both channels
  //-------------------------------------------------------------------------
  genvar bit;
  // Clocks & write strobes
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_clk1 (
    .Q(da1_clk), .C(sys_clk), .CE(1'b1), .D1(1'b0), .D2(1'b1), .R(1'b0), .S(1'b0)
  );
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_wrt1 (
    .Q(da1_wrt), .C(sys_clk), .CE(1'b1), .D1(1'b0), .D2(1'b1), .R(1'b0), .S(1'b0)
  );
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_clk2 (
    .Q(da2_clk), .C(sys_clk), .CE(1'b1), .D1(1'b0), .D2(1'b1), .R(1'b0), .S(1'b0)
  );
  ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_wrt2 (
    .Q(da2_wrt), .C(sys_clk), .CE(1'b1), .D1(1'b0), .D2(1'b1), .R(1'b0), .S(1'b0)
  );

  // Data bits: channel1 = sine, channel2 = cosine
  generate
    for (bit = 0; bit < 14; bit = bit + 1) begin: DAC1_DATA
      ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_d1 (
        .Q  (da1_data[bit]),
        .C  (sys_clk),
        .CE (1'b1),
        .D1 (sin_reg[bit]),
        .D2 (sin_reg[bit]),
        .R  (1'b0),
        .S  (1'b0)
      );
    end
    for (bit = 0; bit < 14; bit = bit + 1) begin: DAC2_DATA
      ODDR #(.DDR_CLK_EDGE("SAME_EDGE")) oddr_d2 (
        .Q  (da2_data[bit]),
        .C  (sys_clk),
        .CE (1'b1),
        .D1 (cos_reg[bit]),
        .D2 (cos_reg[bit]),
        .R  (1'b0),
        .S  (1'b0)
      );
    end
  endgenerate

endmodule
