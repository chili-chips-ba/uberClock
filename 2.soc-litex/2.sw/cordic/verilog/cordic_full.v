// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2017-2024, Gisselquist Technology, LLC
//
// The CORDIC related project set is free software (firmware): you can
// redistribute it and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// The CORDIC related project set is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
// General Public License for more details.
////////////////////////////////////////////////////////////////////////////////

module cordic_pre_rotate #(
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
      case ( i_phase[PW-1:PW-3] )
        3'b000, 3'b111: begin
          o_xval  <= e_xval;
          o_yval  <= e_yval;
          o_phase <= i_phase;
        end
        3'b001, 3'b010: begin
          o_xval  <= -e_yval;
          o_yval  <=  e_xval;
          o_phase <= i_phase - 19'h20000;
        end
        3'b011, 3'b100: begin
          o_xval  <= -e_xval;
          o_yval  <= -e_yval;
          o_phase <= i_phase - 19'h40000;
        end
        3'b101, 3'b110: begin
          o_xval  <=  e_yval;
          o_yval  <= -e_xval;
          o_phase <= i_phase - 19'h60000;
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

module cordic_round #(
   parameter WW = 15,
   parameter OW = 12
) (
   input  wire                   i_clk,
   input  wire                   i_reset,
   input  wire                   i_ce,
   input  wire signed [WW-1:0]   x_in,
   input  wire signed [WW-1:0]   y_in,
   output reg  signed [OW-1:0]   o_xval,
   output reg  signed [OW-1:0]   o_yval
);

 wire signed [WW-1:0] pre_xval;
 wire signed [WW-1:0] pre_yval;

 assign pre_xval = x_in + $signed({ {(OW){1'b0}},
                                     x_in[WW-OW],
                                     {(WW-OW-1){~x_in[WW-OW]}}
                                   });
 assign pre_yval = y_in + $signed({ {(OW){1'b0}},
                                     y_in[WW-OW],
                                     {(WW-OW-1){~y_in[WW-OW]}}
                                   });

 always @(posedge i_clk) begin
   if (i_reset) begin
     o_xval <= 0;
     o_yval <= 0;
   end else if (i_ce) begin
     o_xval <= pre_xval[WW-1:WW-OW];  // Extract the top OW bits.
     o_yval <= pre_yval[WW-1:WW-OW];
   end
 end

endmodule

module cordic #(
    parameter IW       = 12,  // Input width
    parameter OW       = 12,  // Output width
    parameter NSTAGES  = 15,  // Number of CORDIC stages
    parameter WW       = 15,  // Working width for internal computations
    parameter PW       = 19   // Phase accumulator width
) (
    input  wire                   i_clk,
    input  wire                   i_reset,
    input  wire                   i_ce,
    input  wire signed [IW-1:0]   i_xval,
    input  wire signed [IW-1:0]   i_yval,
    input  wire [PW-1:0]          i_phase,
    input  wire                   i_aux,
    output wire signed [OW-1:0]   o_xval,
    output wire signed [OW-1:0]   o_yval,
    output wire                   o_aux
);

  //--------------------------------------------------------------------------
  // Auxiliary (valid) signal pipeline: shift in i_aux for NSTAGES+2 cycles
  //--------------------------------------------------------------------------
  reg [NSTAGES+1:0] ax;
  always @(posedge i_clk) begin
    if (i_reset)
      ax <= 0;
    else if (i_ce)
      ax <= { ax[NSTAGES:0], i_aux };
  end

  //--------------------------------------------------------------------------
  // Pipeline signals for CORDIC data (x, y, and phase)
  //--------------------------------------------------------------------------
  wire signed [WW-1:0]  x_pipe  [0:NSTAGES];
  wire signed [WW-1:0]  y_pipe  [0:NSTAGES];
  wire        [PW-1:0]  ph_pipe [0:NSTAGES];

  //--------------------------------------------------------------------------
  // Pre-Rotation Stage
  //--------------------------------------------------------------------------
  cordic_pre_rotate #(
    .IW(IW),
    .WW(WW),
    .PW(PW)
  ) pre_stage (
    .i_clk   (i_clk),
    .i_reset (i_reset),
    .i_ce    (i_ce),
    .i_xval  (i_xval),
    .i_yval  (i_yval),
    .i_phase (i_phase),
    .o_xval  (x_pipe[0]),
    .o_yval  (y_pipe[0]),
    .o_phase (ph_pipe[0])
  );

  //--------------------------------------------------------------------------
  // Pipeline Stages: Each stage rotates the vector by a predetermined angle.
  //--------------------------------------------------------------------------
  wire [18:0]	cordic_angle [0:(NSTAGES-1)];
	assign	cordic_angle[ 0] = 19'h0_9720; //  26.565051 deg
	assign	cordic_angle[ 1] = 19'h0_4fd9; //  14.036243 deg
	assign	cordic_angle[ 2] = 19'h0_2888; //   7.125016 deg
	assign	cordic_angle[ 3] = 19'h0_1458; //   3.576334 deg
	assign	cordic_angle[ 4] = 19'h0_0a2e; //   1.789911 deg
	assign	cordic_angle[ 5] = 19'h0_0517; //   0.895174 deg
	assign	cordic_angle[ 6] = 19'h0_028b; //   0.447614 deg
	assign	cordic_angle[ 7] = 19'h0_0145; //   0.223811 deg
	assign	cordic_angle[ 8] = 19'h0_00a2; //   0.111906 deg
	assign	cordic_angle[ 9] = 19'h0_0051; //   0.055953 deg
	assign	cordic_angle[10] = 19'h0_0028; //   0.027976 deg
	assign	cordic_angle[11] = 19'h0_0014; //   0.013988 deg
	assign	cordic_angle[12] = 19'h0_000a; //   0.006994 deg
	assign	cordic_angle[13] = 19'h0_0005; //   0.003497 deg
	assign	cordic_angle[14] = 19'h0_0002; //   0.001749 deg
	// {{{
	// Std-Dev    : 0.00 (Units)
	// Phase Quantization: 0.000030 (Radians)
	// Gain is 1.164435
	// You can annihilate this gain by multiplying by 32'hdbd95b17
	// and right shifting by 32 bits.
	// }}}
	// }}}

  genvar i;
  generate
    for (i = 0; i < NSTAGES; i = i + 1) begin : cordic_stages
      cordic_pipeline_stage #(
        .STAGE         (i),
        .WW            (WW),
        .PW            (PW)
      ) stage_inst (
        .i_clk    (i_clk),
        .i_reset  (i_reset),
        .i_ce     (i_ce),
        .x_in     (x_pipe[i]),
        .y_in     (y_pipe[i]),
        .phase_in (ph_pipe[i]),
        .cordic_angle(cordic_angle[i]),
        .x_out    (x_pipe[i+1]),
        .y_out    (y_pipe[i+1]),
        .phase_out(ph_pipe[i+1])
      );
    end
  endgenerate

  //--------------------------------------------------------------------------
  // Final Rounding Stage
  //--------------------------------------------------------------------------
  cordic_round #(
    .WW(WW),
    .OW(OW)
  ) cordic_round_inst (
    .i_clk  (i_clk),
    .i_reset(i_reset),
    .i_ce   (i_ce),
    .x_in   (x_pipe[NSTAGES]),
    .y_in   (y_pipe[NSTAGES]),
    .o_xval (o_xval),
    .o_yval (o_yval)
  );

  // The auxiliary output (valid signal)
  assign o_aux = ax[NSTAGES+1];

endmodule
