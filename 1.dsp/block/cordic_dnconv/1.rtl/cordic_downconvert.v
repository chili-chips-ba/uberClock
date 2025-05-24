`timescale 1ns/100ps

module cordic_downconvert #(
    parameter IW       = 13,  // Input width
    parameter OW       = 13,  // Output width
    parameter NSTAGES  = 16,  // Number of CORDIC stages
    parameter WW       = 16,  // Working width for internal computations
    parameter PW       = 20   // Phase accumulator width
) (
    input  sys_clk_p,    // system clock positive
    input  sys_clk_n,    // system clock negative
    input  rst_n,        // reset, active low

    output [OW-1:0] down_xval,  // downconverted signal (X output)
    output [OW-1:0] down_yval   // downconverted signal (Y output)
);

   wire sys_clk;  // single-ended system clock 
   IBUFDS sys_clk_ibufgds (
      .O(sys_clk),
      .I(sys_clk_p),
      .IB(sys_clk_n)
   );
   
   /*************************************************************************
    * PLL: generate a 65 MHz clock from the system clock.
    *************************************************************************/
   wire clk_out_65;
   pll_65 my_pll (
      .clk_in1(sys_clk),
      .reset(1'b0),
      .clk_out_65(clk_out_65),
      .locked()
   );
   
   // Use the 65 MHz clock as main clock.
   wire i_clk;
   assign i_clk = clk_out_65;
      
   // Always-enabled clock enable.
   wire i_ce;
   assign i_ce = 1'b1;
   
   /*************************************************************************
    * Phase accumulator for 1 MHz sinewave generation.
    * For a PW=20 phase accumulator (full scale = 2^20 = 1,048,576) and a 65 MHz clock:
    * PHASE_INC_1MHz = (1e6/65e6)*1048576 ≈ 16131.
    *************************************************************************/
   localparam PHASE_INC_1MHz = 16131;
   reg [PW-1:0] phase1;
   always @(posedge i_clk or negedge rst_n) begin
       if (~rst_n)
           phase1 <= 0;
       else
           phase1 <= phase1 + PHASE_INC_1MHz;
   end

   /*************************************************************************
    * Instance 1: CORDIC for 1 MHz sinewave generation.
    * The CORDIC uses a constant input vector:
    *    i_xval = 4095 (unity in Q1.12 format),
    *    i_yval = 0,
    *    i_phase = phase1.
    *
    * Its sine output (o_yval) will be a 1 MHz sinewave.
    *************************************************************************/
   wire signed [OW-1:0] gen_xval;
   wire signed [OW-1:0] gen_yval;
   wire                gen_aux;
   
   // Constant amplitude of unity (4095 in Q1.12)
   wire signed [IW-1:0] gen_i_xval = 4095;
   wire signed [IW-1:0] gen_i_yval = 0;
   wire                gen_i_aux  = 1'b1;
   
   cordic #(
       .IW(IW),
       .OW(OW),
       .NSTAGES(NSTAGES),
       .WW(WW),
       .PW(PW)
   ) cordic_gen (
       .i_clk(i_clk),
       .i_reset(~rst_n),
       .i_ce(i_ce),
       .i_xval(gen_i_xval),
       .i_yval(gen_i_yval),
       .i_phase(phase1),
       .i_aux(gen_i_aux),
       .o_xval(gen_xval),  // cosine output (unused)
       .o_yval(gen_yval),  // sine output: 1 MHz sinewave
       .o_aux(gen_aux)
   );
   
   /*************************************************************************
    * Phase accumulator for downconversion.
    * For a 900 kHz local oscillator at 65 MHz clock:
    *    PHASE_INC_900kHz = (900e3/65e6)*1048576 ≈ 14526.
    *************************************************************************/
   localparam PHASE_INC_900kHz = 14526;
   reg [PW-1:0] phase2;
   always @(posedge i_clk or negedge rst_n) begin
       if (~rst_n)
           phase2 <= 0;
       else
           phase2 <= phase2 + PHASE_INC_900kHz;
   end

   /*************************************************************************
    * Instance 2: CORDIC for downconversion.
    *    - i_xval = 0,
    *    - i_yval = the 1 MHz sinewave (gen_yval) from cordic_gen,
    *    - i_phase = phase2 (corresponding to a 900 kHz oscillator).
    *
    *************************************************************************/
   wire signed [OW-1:0] down_xval_internal;
   wire signed [OW-1:0] down_yval_internal;
   wire                 down_aux;
   
   // For downconversion, use zero for X, Y is taken from the generated sinewave.
   wire signed [IW-1:0] down_i_xval = 0;
   wire signed [IW-1:0] down_i_yval = gen_yval;
   //wire                 down_i_aux  = 1'b1;
   
   cordic #(
       .IW(IW),
       .OW(OW),
       .NSTAGES(NSTAGES),
       .WW(WW),
       .PW(PW)
   ) cordic_down (
       .i_clk(i_clk),
       .i_reset(~rst_n),
       .i_ce(i_ce),
       .i_xval(down_i_xval),
       .i_yval(down_i_yval),
       .i_phase(phase2),
       .i_aux(gen_aux),
       .o_xval(down_xval_internal),
       .o_yval(down_yval_internal),
       .o_aux(down_aux)
   );

   /*************************************************************************
    * The code below instantiates a CIC decimator.
    *************************************************************************/
   localparam CIC_REGISTER_WIDTH   = 43;
   localparam CIC_DECIMATION_RATIO = 64;
   localparam DATA_WIDTH  = 13;
   wire [DATA_WIDTH-1:0] sine_dec, cos_dec;
   wire clk_dec_i, clk_dec_q;
   cic #(
        .DATA_WIDTH      (DATA_WIDTH),
        .REGISTER_WIDTH  (CIC_REGISTER_WIDTH),
        .DECIMATION_RATIO(CIC_DECIMATION_RATIO)
    ) cic_decimator_q (
        .clk     (clk_out_65),
        .arst_n  (rst_n),
        .en(down_aux),
        .data_in (down_yval_internal),
        .data_out(sine_dec),
        .data_clk(clk_dec_q)
    );
       cic #(
        .DATA_WIDTH      (DATA_WIDTH),
        .REGISTER_WIDTH  (CIC_REGISTER_WIDTH),
        .DECIMATION_RATIO(CIC_DECIMATION_RATIO)
    ) cic_decimator_i (
        .clk     (clk_out_65),
        .arst_n  (rst_n),
        .en(down_aux),
        .data_in (down_xval_internal),
        .data_out(cos_dec),
        .data_clk(clk_dec_i)
    );
    
       
   /*************************************************************************
    * Expose the downconverted signals as module outputs.
    *************************************************************************/
   assign down_xval = cos_dec;
   assign down_yval = sine_dec;
   

endmodule
