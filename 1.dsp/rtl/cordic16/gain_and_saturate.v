module gain_and_saturate #(
  parameter integer OW = 12
) (
  input  wire                  clk,
  input  wire                  ce,
  input  wire signed [OW-1:0]  x_in,
  input  wire signed [OW-1:0]  y_in,
  output reg  signed [OW-1:0]  x_out,
  output reg  signed [OW-1:0]  y_out
);

  localparam [31:0]              CORDIC_GAIN = 32'hdbd95b17;
  // extend to 33 bits to make signed multiplication unambiguous
  wire signed [32:0]             gain_const_s = {1'b0, CORDIC_GAIN};

  // full-precision products: OW+32 bits wide
  reg  signed [OW+32-1:0]        gain_x_full_reg;
  reg  signed [OW+32-1:0]        gain_y_full_reg;

  // a) multiplication stage
  always @(posedge clk)
    if (ce) begin
      gain_x_full_reg <= $signed(x_in) * gain_const_s;
      gain_y_full_reg <= $signed(y_in) * gain_const_s;
    end

  //-------------------------------------------------------------------------
  // 2) Shift-right 32 to remove fractional and then saturating ×2
  //-------------------------------------------------------------------------

  // clamp values
  localparam signed [OW-1:0]   SAT_POS = {1'b0, {OW-1{1'b1}}};  // +2^(OW-1)-1
  localparam signed [OW-1:0]   SAT_NEG = {1'b1, {OW-1{1'b0}}};  // -2^(OW-1)

  // intermediate registers
  reg signed [OW-1:0] corr_x_reg, corr_y_reg;
  reg signed [OW-1:0] sat_x_reg,  sat_y_reg;

  always @(posedge clk)
    if (ce) begin
      // remove fractional bits
      corr_x_reg <= gain_x_full_reg >>> 32;
      corr_y_reg <= gain_y_full_reg >>> 32;

      // saturating left-shift by 1 (×2)
      sat_x_reg <= (corr_x_reg[OW-1] == corr_x_reg[OW-2])
                   ? (corr_x_reg <<< 1)
                   : (corr_x_reg[OW-1] ? SAT_NEG : SAT_POS);

      sat_y_reg <= (corr_y_reg[OW-1] == corr_y_reg[OW-2])
                   ? (corr_y_reg <<< 1)
                   : (corr_y_reg[OW-1] ? SAT_NEG : SAT_POS);
    end

  // final register stage to line up timing (optional; you could skip and wire sat_*_reg → x_out directly,
  // but this adds a uniform two-cycle pipeline)
  always @(posedge clk)
    if (ce) begin
      x_out <= sat_x_reg;
      y_out <= sat_y_reg;
    end

endmodule
