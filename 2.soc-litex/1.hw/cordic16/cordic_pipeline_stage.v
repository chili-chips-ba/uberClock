module cordic_pipeline_stage #(
    parameter STAGE        = 0,
    parameter WW           = 16,
    parameter PW           = 20
) (
    input  wire                   i_clk,
    input  wire                   i_reset,
    input  wire                   i_ce,
    input  wire signed [WW-1:0]   x_in,
    input  wire signed [WW-1:0]   y_in,
    input  wire [PW-1:0]          phase_in,
    input  wire [PW-1:0]          cordic_angle,
    output reg  signed [WW-1:0]   x_out,
    output reg  signed [WW-1:0]   y_out,
    output reg  [PW-1:0]          phase_out
);

  always @(posedge i_clk) begin
    if (i_reset) begin
      x_out     <= 0;
      y_out     <= 0;
      phase_out <= 0;
    end else if (i_ce) begin
      // If no valid rotation is required or the stage exceeds the working width,
      // simply pass the values along.
      if ((cordic_angle == 0) || (STAGE >= WW)) begin
        x_out     <= x_in;
        y_out     <= y_in;
        phase_out <= phase_in;
      end
      // For negative phase, rotate clockwise.
      else if (phase_in[PW-1] == 1'b1) begin
        x_out     <= x_in + (y_in >>> (STAGE+1));
        y_out     <= y_in - (x_in >>> (STAGE+1));
        phase_out <= phase_in + cordic_angle;
      end else begin
        // For positive phase, rotate counter-clockwise.
        x_out     <= x_in - (y_in >>> (STAGE+1));
        y_out     <= y_in + (x_in >>> (STAGE+1));
        phase_out <= phase_in - cordic_angle;
      end
    end
  end

endmodule
