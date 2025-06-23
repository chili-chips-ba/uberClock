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

module cordic16 #(
    parameter IW       = 16,
    parameter OW       = 16,
    parameter NSTAGES  = 19,
    parameter WW       = 19,
    parameter PW       = 23
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
  cordic_pre_rotate_16 #(
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
	wire	[22:0]	cordic_angle [0:(NSTAGES-1)];

	assign	cordic_angle[ 0] = 23'h09_7202; //  26.565051 deg
	assign	cordic_angle[ 1] = 23'h04_fd9c; //  14.036243 deg
	assign	cordic_angle[ 2] = 23'h02_8888; //   7.125016 deg
	assign	cordic_angle[ 3] = 23'h01_4586; //   3.576334 deg
	assign	cordic_angle[ 4] = 23'h00_a2eb; //   1.789911 deg
	assign	cordic_angle[ 5] = 23'h00_517b; //   0.895174 deg
	assign	cordic_angle[ 6] = 23'h00_28be; //   0.447614 deg
	assign	cordic_angle[ 7] = 23'h00_145f; //   0.223811 deg
	assign	cordic_angle[ 8] = 23'h00_0a2f; //   0.111906 deg
	assign	cordic_angle[ 9] = 23'h00_0517; //   0.055953 deg
	assign	cordic_angle[10] = 23'h00_028b; //   0.027976 deg
	assign	cordic_angle[11] = 23'h00_0145; //   0.013988 deg
	assign	cordic_angle[12] = 23'h00_00a2; //   0.006994 deg
	assign	cordic_angle[13] = 23'h00_0051; //   0.003497 deg
	assign	cordic_angle[14] = 23'h00_0028; //   0.001749 deg
	assign	cordic_angle[15] = 23'h00_0014; //   0.000874 deg
	assign	cordic_angle[16] = 23'h00_000a; //   0.000437 deg
	assign	cordic_angle[17] = 23'h00_0005; //   0.000219 deg
	assign	cordic_angle[18] = 23'h00_0002; //   0.000109 deg
	
  
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
  wire signed [OW-1:0] round_x, round_y;
  cordic_round #(
    .WW(WW),
    .OW(OW)
  ) cordic_round_inst1 (
    .i_clk  (i_clk),
    .i_reset(i_reset),
    .i_ce   (i_ce),
    .x_in   (x_pipe[NSTAGES]),
    .y_in   (y_pipe[NSTAGES]),
    .o_xval (round_x),
    .o_yval (round_y)
  );
  
   wire signed [OW-1:0] sat_x, sat_y;
  gain_and_saturate #(.OW(OW)) gain_sat_inst (
      .clk  (i_clk),
      .ce   (i_ce),
      .x_in (round_x),
      .y_in (round_y),
      .x_out(sat_x),
      .y_out(sat_y)
    );
  assign o_xval = sat_x;
    assign o_yval = sat_y;

  // The auxiliary output (valid signal)
  assign o_aux = ax[NSTAGES+1];

endmodule
