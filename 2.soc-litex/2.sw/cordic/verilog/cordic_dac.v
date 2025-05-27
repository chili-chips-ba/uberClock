// cordic_dac.v
`timescale 1ns/1ps

module cordic_dac #(
    parameter IW      = 12,
    parameter OW      = 12,
    parameter NSTAGES = 15,
    parameter WW      = 15,
    parameter PW      = 19
)(
    input  wire                   clk,
    input  wire                   rst_n,      // active-low

    // CPU-side control
    input  wire [PW-1:0]          phase_in,
    input  wire                   phase_ce,   // sample phase on rising edge

    // DAC outputs
    output wire                   da1_clk,
    output wire                   da1_wrt,
    output wire [13:0]            da1_data,
    output wire                   da2_clk,
    output wire                   da2_wrt,
    output wire [13:0]            da2_data
);

  // internal CORDIC wires
  wire signed [IW-1:0]   x0 = 1 << (IW-1);  // unit vector on X
  wire signed [IW-1:0]   y0 = 0;
  wire signed [OW-1:0]   y_out;
  wire                    aux_out;

  // instantiate the standard CORDIC
  cordic #(
    .IW(IW),
    .OW(OW),
    .NSTAGES(NSTAGES),
    .WW(WW),
    .PW(PW)
  ) u_cordic (
    .i_clk   (clk),
    .i_reset (~rst_n),
    .i_ce    (phase_ce),
    .i_xval  (x0),
    .i_yval  (y0),
    .i_phase (phase_in),
    .i_aux   (1'b1),
    .o_xval  (),
    .o_yval  (y_out),
    .o_aux   (aux_out)
  );

  // zero-extend 12→14 bits for DAC
  wire [13:0] dac_sample = { y_out[OW-1], y_out, 2'b00 };

  // single-channel: feed cordic into DAC1, leave DAC2 idle
  dac_control u_dac (
    .sys_clk   (clk),
    .rst_n     (rst_n),
    .data1     (dac_sample),
    .wrt1_en   (aux_out),
    .data2     (14'd0),
    .wrt2_en   (1'b0),
    .da1_clk   (da1_clk),
    .da1_wrt   (da1_wrt),
    .da1_data  (da1_data),
    .da2_clk   (da2_clk),
    .da2_wrt   (da2_wrt),
    .da2_data  (da2_data)
  );

endmodule
