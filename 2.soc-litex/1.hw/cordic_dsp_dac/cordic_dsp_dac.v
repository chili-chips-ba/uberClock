`timescale 1ns / 1ps
`default_nettype none
module cordic_dsp_dac#(
    parameter IW       = 12,   // CORDIC input width
    parameter OW       = 12,   // CORDIC output width
    parameter NSTAGES  = 15,   // pipeline stages
    parameter WW       = 15,   // working width
    parameter PW       = 19    // phase accumulator width
)(
    input                     sys_clk,
    input                     rst,

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

    //Phase Increment
    input  [PW-1:0]           phase_inc_nco,
    input  [PW-1:0]           phase_inc_down,

    input                     input_select,  // 0=use ADC, 1=use internal NCO
    input  [1:0]              output_select,

    input  [31:0]             gain1,
    input  [31:0]             gain2,

    // Debug outputs
    output [IW-1:0]           dbg_nco_cos,
    output [IW-1:0]           dbg_nco_sin,
    output [PW-1:0]           dbg_phase_acc_down,
    output [11:0]             dbg_x_downconverted,
    output [11:0]             dbg_y_downconverted,
    output [15:0]             dbg_downsampled_x,
    output [15:0]             dbg_downsampled_y,
    output [15:0]             dbg_upsampled_x,
    output [15:0]             dbg_upsampled_y,
    output [22:0]             dbg_phase_inv,
    output [15:0]             dbg_x_upconverted,
    output [15:0]             dbg_y_upconverted,
    output                    dbg_ce_down_x,
    output                    dbg_ce_up_x,
    output                    dbg_cic_ce_x,
    output                    dbg_comp_ce_x,
    output                    dbg_hb_ce_x,
    output signed [11:0]      dbg_cic_out_x,
    output signed [15:0]      dbg_comp_out_x
    );

    //======================================================================
    // Instantiate the “adc” module
    //======================================================================
    wire [11:0] ad_data_ch0_12;
    wire [11:0] ad_data_ch1_12;
    adc u_adc (
        .sys_clk      (sys_clk),
        .rst_n        (rst),
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

    // ----------------------------------------------------------------------
    // Phase accumulator for NCO
    // ----------------------------------------------------------------------
    reg [PW-1:0] phase_acc_nco_reg;
    always @(posedge sys_clk or posedge rst) begin
       if (rst)
           phase_acc_nco_reg <= 0;
       else
           phase_acc_nco_reg <= phase_acc_nco_reg + phase_inc_nco;
    end

    wire signed [IW-1:0] nco_cos, nco_sin;
    wire                 nco_aux;
    cordic #(
       .IW(IW),
       .OW(OW),
       .NSTAGES(NSTAGES),
       .WW(WW),
       .PW(PW)
    ) cordic_nco (
        .i_clk   (sys_clk),
        .i_reset (rst),
        .i_ce    (1'b1),
        .i_xval  (12'sd1000),
        .i_yval  (12'sd0),
        .i_phase (phase_acc_nco_reg),
        .i_aux   (1'b1),
        .o_xval  (nco_cos),
        .o_yval  (nco_sin),
        .o_aux   (nco_aux)
    );

    reg signed [11:0] filter_in;
    always @(posedge sys_clk) begin
        filter_in <= {~ad_data_ch0_12[11], ad_data_ch0_12[10:0]};
    end

    // ----------------------------------------------------------------------
    // Phase accumulator for downconversion
    // ----------------------------------------------------------------------
    reg [PW-1:0] phase_acc_down_reg;
    always @(posedge sys_clk or posedge rst) begin
        if (rst)
            phase_acc_down_reg <= 0;
        else
            phase_acc_down_reg <= phase_acc_down_reg + phase_inc_down;
    end
    wire signed [IW-1:0] x_downconverted, y_downconverted;
    wire                 down_aux;
    wire signed [IW-1:0] selected_input;
    assign selected_input = input_select ? nco_cos : filter_in;

   cordic #(
       .IW(IW),
       .OW(OW),
       .NSTAGES(NSTAGES),
       .WW(WW),
       .PW(PW)
   ) cordic_down (
       .i_clk  (sys_clk),
       .i_reset(rst),
       .i_ce   (1'b1),
       .i_xval (0),
       .i_yval (selected_input),
       .i_phase(phase_acc_down_reg),
       .i_aux  (1'b1),
       .o_xval (x_downconverted),
       .o_yval (y_downconverted),
       .o_aux  (down_aux)
   );

    // ----------------------------------------------------------------------
    // Downsampling filters
    // ----------------------------------------------------------------------
    wire                cic_ce_x, comp_ce_x, hb_ce_x;
    wire signed [11:0]  cic_out_x;
    wire signed [15:0]  comp_out_x;
    wire signed [15:0]  downsampled_x, downsampled_y;

    downsamplerFilter down_x (
        .clk           (sys_clk),
        .clk_enable    (1'b1),
        .reset         (rst),
        .filter_in     (x_downconverted),
        .filter_out    (downsampled_x),
        .ce_out        (ce_out_down_x),
        .debug_cic_ce  (cic_ce_x),
        .debug_comp_ce (comp_ce_x),
        .debug_hb_ce   (hb_ce_x),
        .debug_cic_out (cic_out_x),
        .debug_comp_out(comp_out_x)
    );

    downsamplerFilter down_y (
        .clk        (sys_clk),
        .clk_enable (1'b1),
        .reset      (rst),
        .filter_in  (y_downconverted),
        .filter_out (downsampled_y),
        .ce_out     (ce_out_down_y)
    );

    // ----------------------------------------------------------------------
    // Upsampling filters
    // ----------------------------------------------------------------------

    wire signed [48:0] y_gained , x_gained ;
    wire signed [32:0] gain_signed_1 = {1'b0, gain1};
    wire signed [32:0] gain_signed_2 = {1'b0, gain2};

    assign y_gained = downsampled_y * gain_signed_1;
    assign x_gained = downsampled_x * gain_signed_1;

    wire signed [15:0] upsampled_x, upsampled_y;
    wire signed [15:0] upsampled_gain_x, upsampled_gain_y;
    assign upsampled_gain_y = y_gained >>> 30;
    assign upsampled_gain_x = x_gained >>> 30;

    upsamplerFilter up_x (
        .clk        (sys_clk),
        .clk_enable (1'b1),
        .reset      (rst),
        .filter_in  (upsampled_gain_x),
        .filter_out (upsampled_x),
        .ce_out     (ce_out_up_x)
    );

    upsamplerFilter up_y (
        .clk        (sys_clk),
        .clk_enable (1'b1),
        .reset      (rst),
        .filter_in  (upsampled_gain_y),
        .filter_out (upsampled_y),
        .ce_out     (ce_out_up_y)
    );


    // ----------------------------------------------------------------------
    // Upconversion CORDIC
    // ----------------------------------------------------------------------
    wire [22:0] phase_inv = (1 << 23) - (phase_acc_down_reg << 4);
    wire signed [15:0] x_upconverted, y_upconverted;
    wire up_aux;

    cordic16 #(
        .IW(16),
        .OW(16),
        .NSTAGES(19),
        .WW(19),
        .PW(23)
    ) cordic_up (
        .i_clk   (sys_clk),
        .i_reset (rst),
        .i_ce    (1'b1),
        .i_xval  (upsampled_y),
        .i_yval  (upsampled_x),
        .i_phase (phase_inv),
        .i_aux   (ce_out_up_x),
        .o_xval  (x_upconverted),
        .o_yval  (y_upconverted),
        .o_aux   (up_aux)
    );

    // ----------------------------------------------------------------------
    // DAC data preparation
    // ----------------------------------------------------------------------

    wire [13:0] dac1_data_in =   (output_select == 2'b00) ? downsampled_y[15:2]
                                : (output_select == 2'b01) ? upsampled_y[15:2]
                                : (output_select == 2'b10) ? y_downconverted << 2
                                : y_upconverted[15:2];
    wire [13:0] dac2_data_in = upsampled_gain_y[15:2];
    reg  [13:0] dac1_data_reg, dac2_data_reg;

    always @(posedge sys_clk) begin
        dac1_data_reg <= dac1_data_in + 14'd8192;
        dac2_data_reg <= dac2_data_in + 14'd8192;
    end


     // ----------------------------------------------------------------------
    // DDR-output DAC module
    // ----------------------------------------------------------------------
    dac u_dac (
        .sys_clk  (sys_clk),
        .rst_n    (rst),
        .data1    (dac1_data_reg),
        .data2    (dac2_data_reg),
        .da1_clk  (da1_clk),
        .da1_wrt  (da1_wrt),
        .da1_data (da1_data),
        .da2_clk  (da2_clk),
        .da2_wrt  (da2_wrt),
        .da2_data (da2_data)
    );

    // ----------------------------------------------------------------------
    // Debug signal assignments
    // ----------------------------------------------------------------------
    assign dbg_nco_cos         = nco_cos;
    assign dbg_nco_sin         = nco_sin;
    assign dbg_phase_acc_down  = phase_acc_down_reg;
    assign dbg_x_downconverted = x_downconverted;
    assign dbg_y_downconverted = y_downconverted;
    assign dbg_downsampled_x   = downsampled_x;
    assign dbg_downsampled_y   = downsampled_y;
    assign dbg_upsampled_x     = upsampled_x;
    assign dbg_upsampled_y     = upsampled_y;
    assign dbg_phase_inv       = phase_inv;
    assign dbg_x_upconverted   = x_upconverted;
    assign dbg_y_upconverted   = y_upconverted;
    assign dbg_ce_down_x       = ce_out_down_x;
    assign dbg_ce_up_x         = ce_out_up_x;
    assign dbg_cic_ce_x        = cic_ce_x;
    assign dbg_comp_ce_x       = comp_ce_x;
    assign dbg_hb_ce_x         = hb_ce_x;
    assign dbg_cic_out_x       = cic_out_x;
    assign dbg_comp_out_x      = comp_out_x;
endmodule
