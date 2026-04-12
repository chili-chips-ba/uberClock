`timescale 1 ns / 1 ns
`default_nettype none
module cic #(
    parameter  DATA_WIDTH_I       = 12,
    parameter DATA_WIDTH_O      = 16,
    parameter  REGISTER_WIDTH   = 64,
    parameter  DECIMATION_RATIO = 8
) (
    input  wire                          clk,
    input  wire                          arst_n,
    input  wire                          en,
    input  wire signed [DATA_WIDTH_I-1:0]  data_in,
    output wire signed [DATA_WIDTH_O-1:0]  data_out,
    output wire                          data_clk
);
  localparam COUNT_WIDTH = $clog2(DECIMATION_RATIO);
  //=============================//
  //       Internal signals      //
  //=============================//
  reg signed [REGISTER_WIDTH-1:0] integrator_tmp, integrator_d_tmp;
  reg signed [REGISTER_WIDTH-1:0] integrator1, integrator2, integrator3, integrator4;
  reg signed [REGISTER_WIDTH-1:0] comb5, comb_d5, comb6, comb_d6, comb7, comb_d7, comb8;
  reg                             valid_comb;
  reg                             decimation_clk;
  reg        [COUNT_WIDTH-1:0]    count=0; 
  //=============================//
  //    Integrator section       //
  //=============================//
  always @(posedge clk or negedge arst_n) begin
    if (!arst_n) begin
        count          <= {COUNT_WIDTH{1'b0}};
        integrator1    <= 0;
        integrator2    <= 0;
        integrator3    <= 0;
        integrator4    <= 0;
        integrator_tmp <= 0;

        decimation_clk <= 1'b0;
        valid_comb     <= 1'b0;
    end else if (en) begin
        // Integrator logic
        integrator1 <= integrator1 + data_in;
        integrator2 <= integrator1 + integrator2;
        integrator3 <= integrator2 + integrator3;
        integrator4 <= integrator3 + integrator4;
        // Decimation logic
		  decimation_clk <= 1'b0;
        if (count == DECIMATION_RATIO - 1) begin
            count          <= {COUNT_WIDTH{1'b0}};
            integrator_tmp <= integrator4;
            decimation_clk <= 1'b1;
            valid_comb     <= 1'b1;
        end else if (count == DECIMATION_RATIO >> 1) begin
            count          <= count + 1;
            valid_comb     <= 1'b0;
        end else begin
            count          <= count + 1;
            valid_comb     <= 1'b0;
        end
    end
  end
  //=============================//
  //       Comb section          //
  //=============================//
  always @(posedge clk or negedge arst_n) begin
    if (!arst_n) begin
        comb5   <= 0;
        comb_d5 <= 0;
        comb6   <= 0;
        comb_d6 <= 0;
        comb7   <= 0;
        comb_d7 <= 0;
        comb8   <= 0;
        integrator_d_tmp <= 0;
    end else if (valid_comb == 1'b1) begin
        integrator_d_tmp <= integrator_tmp;
        comb5            <= integrator_tmp - integrator_d_tmp;
        comb_d5          <= comb5;
        comb6            <= comb5 - comb_d5;
        comb_d6          <= comb6;
        comb7            <= comb6 - comb_d6;
        comb_d7          <= comb7;
        comb8            <= comb7 - comb_d7;
    end
  end
  //=============================//
  //       Output section        //
  //=============================//
  assign data_out = (comb8 >>> (REGISTER_WIDTH - DATA_WIDTH_O - 1));
//assign data_out = comb8[54:39];
//  assign data_out = comb8[55:44] << 2;
  assign data_clk = decimation_clk;
endmodule
