// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

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



// module cordic_round #(
//     parameter integer WW = 15,   // input width from CORDIC core
//     parameter integer OW = 12    // output width after all ops
// ) (
//     input  wire                   i_clk,
//     input  wire                   i_reset,  // active-high
//     input  wire                   i_ce,
//     input  wire signed [WW-1:0]   x_in,
//     input  wire signed [WW-1:0]   y_in,
//     output reg  signed [OW-1:0]   o_xval,
//     output reg  signed [OW-1:0]   o_yval
// );
//
//   //-------------------------------------------------------------------------
//   // 1) Nearest-even converge WW→OW
//   //-------------------------------------------------------------------------
//   wire signed [WW-1:0] bias_x = $signed({
//       {(OW){1'b0}},
//       x_in [WW-OW],
//       {(WW-OW-1){~x_in [WW-OW]}}
//   });
//   wire signed [WW-1:0] bias_y = $signed({
//       {(OW){1'b0}},
//       y_in [WW-OW],
//       {(WW-OW-1){~y_in [WW-OW]}}
//   });
//
//   wire signed [WW-1:0] pre_x = x_in + bias_x,
//                         pre_y = y_in + bias_y;
//
//   wire signed [OW-1:0] rnd_x = pre_x[WW-1:WW-OW],
//                         rnd_y = pre_y[WW-1:WW-OW];
//
//
   //-------------------------------------------------------------------------
   // 2) Gain removal (×1/1.164435) via Q0.32 multiply→>>32
   //-------------------------------------------------------------------------
//   localparam [31:0]               CORDIC_GAIN = 32'hdbd95b17;
//   wire signed   [32:0]            gain_const_s = {1'b0, CORDIC_GAIN};
//   wire signed [OW+32-1:0]         gain_x_full  = $signed(rnd_x) * gain_const_s;
//   wire signed [OW+32-1:0]         gain_y_full  = $signed(rnd_y) * gain_const_s;

//   // shift-right 32 to remove fractional
//   wire signed [OW-1:0]            corr_x       = gain_x_full >>> 32;
//   wire signed [OW-1:0]            corr_y       = gain_y_full >>> 32;


//   //-------------------------------------------------------------------------
//   // 3) Saturating ×2 (left shift with clamp)
//   //-------------------------------------------------------------------------
//   localparam signed [OW-1:0] SAT_POS = {1'b0, {OW-1{1'b1}}};  // +2^(OW-1)-1
//   localparam signed [OW-1:0] SAT_NEG = {1'b1, {OW-1{1'b0}}};  // -2^(OW-1)

//   wire signed [OW-1:0] sat_x = (corr_x[OW-1] == corr_x[OW-2])
//        ? (corr_x <<< 1)
//        : ( corr_x[OW-1] ? SAT_NEG : SAT_POS );

//   wire signed [OW-1:0] sat_y = (corr_y[OW-1] == corr_y[OW-2])
//        ? (corr_y <<< 1)
//        : ( corr_y[OW-1] ? SAT_NEG : SAT_POS );

//
//   //-------------------------------------------------------------------------
//   // 4) Register the final, saturated result
//   //-------------------------------------------------------------------------
//   always @(posedge i_clk) begin
//     if (i_reset) begin
//       o_xval <= {OW{1'b0}};
//       o_yval <= {OW{1'b0}};
//     end else if (i_ce) begin
//       o_xval <= sat_x;
//       o_yval <= sat_y;
//     end
//   end
//
// endmodule
