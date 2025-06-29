
// ============================================================================
//  adc_dac.v
// ============================================================================
`timescale 1ns / 1ps
module adc_cordic_dsp_dac(
    input                     sys_clk,
    input                     rst_n,
    // ADC (12-bit inputs; AD9238 on J11)
    output                    adc_clk_ch0,  // AD channel 0 sampling clock
    output                    adc_clk_ch1,  // AD channel 1 sampling clock
    input  [11:0]             adc_data_ch0, // AD channel 0 data
    input  [11:0]             adc_data_ch1, // AD channel 1 data
    // DDR‐output DAC (14-bit output; AN9767 on J13)
    output                    da1_clk,         // DA1 clock (DDR‐output)
    output                    da1_wrt,         // DA1 write strobe (DDR‐output)
    output [13:0]             da1_data,        // DA1 14‐bit data bus (DDR‐output)
    output                    da2_clk,         // DA2 clock (DDR‐output)
    output                    da2_wrt,         // DA2 write strobe (DDR‐output)
    output [13:0]             da2_data,        // DA2 14‐bit data bus (DDR‐output)
    //phase inc input
    input  [18:0]             phase_inc,       // Phase increment for CORD
    output [11:0] debug_filter_in,
    output [18:0] debug_phase2,
    output [11:0] debug_xval_downconverted,
    output [11:0] debug_yval_downconverted,
    output [15:0] debug_downsampledX,
    output [15:0] debug_downsampledY,
    output [15:0] debug_upsampledX,
    output [15:0] debug_upsampledY,
    output [22:0] debug_phase2_inv_alt,
    output [15:0] debug_xval_upconverted,
    output [15:0] debug_yval_upconverted,
    output        debug_ce_out_down_x,
    output        debug_ce_out_up_x,

    output wire debug_cic_ce_x,
    output wire debug_comp_ce_x,
    output wire debug_hb_ce_x,
    output wire signed [11:0] debug_cic_out_x,
    output wire signed [15:0] debug_comp_out_x
    );
    //======================================================================
    // Instantiate the “adc” module
    //======================================================================
    wire [11:0] ad_data_ch0_12;
    wire [11:0] ad_data_ch1_12;
    adc u_adc (
        .sys_clk      (sys_clk),
        .rst_n        (rst_n),
        // Raw DDR-pinned inputs from the board
        .adc_data_ch0 (adc_data_ch0),
        .adc_data_ch1 (adc_data_ch1),
        // DDR clocks to drive each AD9238 chip
        .adc_clk_ch0  (adc_clk_ch0),
        .adc_clk_ch1  (adc_clk_ch1),
        // 12-bit, single-clock-domain outputs (rising-edge captures)
        .ad_data_ch0  (ad_data_ch0_12),
        .ad_data_ch1  (ad_data_ch1_12)
    );

    wire signed [15:0] downsampledY,downsampledX ;
    wire signed [15:0] upsampledY, upsampledX;
    wire ce_out_down_x, ce_out_down_y, ce_out_up_x, ce_out_up_y;

    reg signed [11:0] filter_in;
    always @(posedge sys_clk) begin
        filter_in <= {~ad_data_ch0_12[11], ad_data_ch0_12[10:0]};
    end
    
     /*************************************************************************
    * Phase accumulator for downconversion.
    * For a 999 kHz local oscillator at 65 MHz clock:
    *    PHASE_INC_999kHz = (999e3/65e6)*2^19 ≈ 80652.
    *************************************************************************/
   wire [18:0] PHASE_INC_999kHz = phase_inc;
   reg [18:0] phase2;
   always @(posedge sys_clk or posedge rst_n) begin
       if (rst_n)
           phase2 <= 0;
       else
           phase2 <= phase2 + PHASE_INC_999kHz;
   end
   wire signed [11:0] xval_downconverted;
   wire signed [11:0] yval_downconverted;
   wire                 o_down_aux;
   

   wire signed [11:0] down_i_xval = 0;
   wire signed [11:0] down_i_yval = filter_in;
   
   cordic #(
       .IW(12),
       .OW(12),
       .NSTAGES(15),
       .WW(15),
       .PW(19)
   ) cordic_down (
       .i_clk(sys_clk),
       .i_reset(rst_n),
       .i_ce(1'b1),
       .i_xval(down_i_xval),
       .i_yval(down_i_yval),
       .i_phase(phase2),
       .i_aux(1'b1),
       .o_xval(xval_downconverted),
       .o_yval(yval_downconverted),
       .o_aux(o_down_aux)
   );
    wire cic_ce_x;
    wire comp_ce_x;
    wire hb_ce_x;
    wire signed [11:0] ds_cic_out_x;
    wire signed [15:0] ds_comp_out_x;
    downsamplerFilter downDsp_x (
        .clk(sys_clk),
        .clk_enable(1'b1),
        .reset(rst_n),
        .filter_in(xval_downconverted),
        .filter_out(downsampledX),
        .ce_out(ce_out_down_x),
        .debug_cic_ce  (cic_ce_x),
        .debug_comp_ce (comp_ce_x),
        .debug_hb_ce   (hb_ce_x),
        .debug_cic_out (ds_cic_out_x),
        .debug_comp_out(ds_comp_out_x)
    );
    downsamplerFilter downDsp_y (
        .clk(sys_clk),
        .clk_enable(1'b1),
        .reset(rst_n),
        .filter_in(yval_downconverted),
        .filter_out(downsampledY),
        .ce_out(ce_out_down_y)
    );

    wire signed [15:0] upsamplerInputX = downsampledX;
    wire signed [15:0] upsamplerInputY = downsampledY;    

    upsamplerFilter upDsp_x (
        .clk(sys_clk),
        .clk_enable(1'b1),
        .reset(rst_n),
        .filter_in(upsamplerInputX),
        .filter_out(upsampledX),
        .ce_out(ce_out_up_x)
    );
    upsamplerFilter upDsp_y (
        .clk(sys_clk),
        .clk_enable(1'b1),
        .reset(rst_n),
        .filter_in(upsamplerInputY),
        .filter_out(upsampledY),
        .ce_out(ce_out_up_y)
    );

    wire [22:0] phase2_inv_alt = (1<<23) - (phase2 << 4);
    wire signed [15:0] xval_upconverted;
    wire signed [15:0] yval_upconverted;
    wire up_aux;

    cordic16 #(
       .IW(16),
       .OW(16),
       .NSTAGES(19),
       .WW(19),
       .PW(23)
   ) cordic_up (
       .i_clk(sys_clk),
       .i_reset(rst_n),
       .i_ce(1'b1),
       .i_xval(upsampledY),
       .i_yval(upsampledX),
       .i_phase(phase2_inv_alt),
       .i_aux(ce_out_up_x),
       .o_xval(xval_upconverted),
       .o_yval(yval_upconverted),
       .o_aux(up_aux)
   );   


    wire [13:0] dac1_input_14 = downsampledY[15:2];
    wire [13:0] dac2_input_14 = upsampledY[15:2];
    reg [13:0] dac1_input_14_reg, dac2_input_14_reg;

    always @(posedge sys_clk) begin
       dac1_input_14_reg <= dac1_input_14 + 14'd8192;
       dac2_input_14_reg <= dac2_input_14 + 14'd8192;
    end


    //======================================================================
    // Instantiate the DDR-output DAC module
    //======================================================================
    dac u_dac (
            .sys_clk   (sys_clk),
            .rst_n     (rst_n),
            .data1     (dac1_input_14_reg),
            .data2     (dac2_input_14_reg),
            .da1_clk   (da1_clk),
            .da1_wrt   (da1_wrt),
            .da1_data  (da1_data),
            .da2_clk   (da2_clk),
            .da2_wrt   (da2_wrt),
            .da2_data  (da2_data)
    );

    assign debug_filter_in         = filter_in;
    assign debug_phase2            = phase2;
    assign debug_xval_downconverted = xval_downconverted;
    assign debug_yval_downconverted = yval_downconverted;
    assign debug_downsampledX      = downsampledX;
    assign debug_downsampledY      = downsampledY;
    assign debug_upsampledX        = upsampledX;
    assign debug_upsampledY        = upsampledY;
    assign debug_phase2_inv_alt    = phase2_inv_alt;
    assign debug_xval_upconverted  = xval_upconverted;
    assign debug_yval_upconverted  = yval_upconverted;
    assign debug_ce_out_down_x     = ce_out_down_x;
    assign debug_ce_out_up_x       = ce_out_up_x;
    assign debug_cic_ce_x          = cic_ce_x;
    assign debug_comp_ce_x         = comp_ce_x;
    assign debug_hb_ce_x           = hb_ce_x;
    assign debug_cic_out_x         = ds_cic_out_x;
    assign debug_comp_out_x        = ds_comp_out_x;
endmodule
