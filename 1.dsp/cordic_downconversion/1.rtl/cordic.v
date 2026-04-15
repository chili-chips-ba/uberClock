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
    parameter IW       = 13,  // Input width
    parameter OW       = 13,  // Output width
    parameter NSTAGES  = 16,  // Number of CORDIC stages
    parameter WW       = 16,  // Working width for internal computations
    parameter PW       = 20   // Phase accumulator width
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
  wire [19:0]	cordic_angle [0:(NSTAGES-1)];
	assign	cordic_angle[ 0] = 20'h1_2e40; //  26.565051 deg
	assign	cordic_angle[ 1] = 20'h0_9fb3; //  14.036243 deg
	assign	cordic_angle[ 2] = 20'h0_5111; //   7.125016 deg
	assign	cordic_angle[ 3] = 20'h0_28b0; //   3.576334 deg
	assign	cordic_angle[ 4] = 20'h0_145d; //   1.789911 deg
	assign	cordic_angle[ 5] = 20'h0_0a2f; //   0.895174 deg
	assign	cordic_angle[ 6] = 20'h0_0517; //   0.447614 deg
	assign	cordic_angle[ 7] = 20'h0_028b; //   0.223811 deg
	assign	cordic_angle[ 8] = 20'h0_0145; //   0.111906 deg
	assign	cordic_angle[ 9] = 20'h0_00a2; //   0.055953 deg
	assign	cordic_angle[10] = 20'h0_0051; //   0.027976 deg
	assign	cordic_angle[11] = 20'h0_0028; //   0.013988 deg
	assign	cordic_angle[12] = 20'h0_0014; //   0.006994 deg
	assign	cordic_angle[13] = 20'h0_000a; //   0.003497 deg
	assign	cordic_angle[14] = 20'h0_0005; //   0.001749 deg
	assign	cordic_angle[15] = 20'h0_0002; //   0.000874 deg
  
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
