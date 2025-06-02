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
