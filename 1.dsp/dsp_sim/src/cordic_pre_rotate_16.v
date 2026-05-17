module cordic_pre_rotate_16 #(
    parameter IW = 12,
    parameter WW = 15,
    parameter PW = 19
) (
    input  wire                   i_clk,
    input  wire                   i_reset,
    input  wire                   i_ce,
    input  wire signed [IW-1:0]   i_xval,
    input  wire signed [IW-1:0]   i_yval,
    input  wire [PW-1:0]          i_phase,
    output reg  signed [WW-1:0]   o_xval,
    output reg  signed [WW-1:0]   o_yval,
    output reg  [PW-1:0]          o_phase
);

  // Sign-extend the inputs to the working width.
  wire signed [WW-1:0] e_xval = { i_xval[IW-1], i_xval, {(WW-IW-1){1'b0}} };
  wire signed [WW-1:0] e_yval = { i_yval[IW-1], i_yval, {(WW-IW-1){1'b0}} };

 always @(posedge i_clk) begin
  if (i_reset) begin
    o_xval  <= 0;
    o_yval  <= 0;
    o_phase <= 0;
  end else if (i_ce) begin
    // Pre-rotate based on the top 3 bits of the phase.
    case (i_phase[PW-1:PW-3])
      3'b000, 3'b111: begin
        o_xval  <= e_xval;
        o_yval  <= e_yval;
        o_phase <= i_phase;
      end
      3'b001, 3'b010: begin
        o_xval  <= -e_yval;
        o_yval  <=  e_xval;
        o_phase <= i_phase - 23'h200000; 
      end
      3'b011, 3'b100: begin
        o_xval  <= -e_xval;
        o_yval  <= -e_yval;
        o_phase <= i_phase - 23'h400000; 
      end
      3'b101, 3'b110: begin
        o_xval  <=  e_yval;
        o_yval  <= -e_xval;
        o_phase <= i_phase - 23'h600000; 
      end
      default: begin
        o_xval  <= e_xval;
        o_yval  <= e_yval;
        o_phase <= i_phase;
      end
    endcase
  end
end


endmodule