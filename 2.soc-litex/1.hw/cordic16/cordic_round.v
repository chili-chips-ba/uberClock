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