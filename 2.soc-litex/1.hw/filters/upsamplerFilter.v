`timescale 1 ns / 1 ns
`default_nettype none
module upsamplerFilter (
    input  wire        clk,
    input  wire        clk_enable,
    input  wire        reset,
    input  wire signed [15:0] filter_in,     // Input to half-band interpolator
    output wire signed [15:0] filter_out,    // Final output from CIC interpolator
    output wire        ce_out                // Output enable from CIC
);
    // Intermediate signals
    wire signed [15:0] hb_out;
    wire               hb_ce_out, cic_ce_out;
    wire signed [15:0] comp_out;
    wire               comp_ce_out, clk_out;
    // Stage 1: Half-Band Interpolator
    hb_up_mac #(
    .DW(16),
    .CW(19),
    .POLYPHASE_DEPTH(60),
    .DEPTH(64),
    .COEFF_INIT_FILE  ("coeffs.mem")     
  ) hb_inst (
        .clk(clk),
        .clk_enable(comp_ce_out),
        .reset(reset),
        .filter_in(filter_in),
        .filter_out(hb_out),
        .ce_out(ce_out)
    );
    // Stage 2: CIC Compensation Interpolator
    cic_comp_up_mac #(
    .DW(16),
    .CW(16),
    .POLYPHASE_DEPTH(18),
    .DEPTH(32),
    .COEFF_INIT_FILE  ("coeffs_comp.mem")     
  )  comp_inst (
        .clk(clk),
        .clk_enable(cic_ce_out),
        .reset(reset),
        .filter_in(hb_out),
        .filter_out(comp_out),
        .ce_out(comp_ce_out)
    );
    // Stage 3: CIC Interpolator
//    cic_up cic_inst (
//        .clk(clk),
//        .clk_enable(clk_enable),
//        .reset(reset),
//        .filter_in(comp_out),
//        .filter_out(filter_out),
//        .ce_out(cic_ce_out)
//    );
	
	
	 cic_int #(
		 .I_WIDTH   (16),    // input data width
		 .O_WIDTH   (16),    // output data width
		 .RMAX      (1625),  // decimation factor
		 .M         (1),     // differential delay
		 .N         (5),     // number of stages
		 .REG_WIDTH (71)     // accumulator width (16 + 5·ceil(log2(1625)) = 71)
   )  u_cic_int (
       .clk         (clk),
       .rst         (reset),
       .input_tdata (comp_out),
       .clk_enable  (clk_enable),
       .output_tdata(filter_out),
       .clk_out     (clk_out),
       .ce_out      (cic_ce_out)
   );
	
	
	
endmodule
