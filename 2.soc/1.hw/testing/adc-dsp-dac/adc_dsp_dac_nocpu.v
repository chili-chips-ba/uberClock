// ============================================================================
//  adc_dsp_dac_nocpu.v
// ============================================================================
`timescale 1ns / 1ps
module adc_dsp_dac_nocpu(
    input                     sys_clk,
    input                     rst_n,
    // ADC (12-bit inputs)
    output                    adc_clk_ch0,
    output                    adc_clk_ch1,
    input  [11:0]             adc_data_ch0,
    input  [11:0]             adc_data_ch1,
    // DAC (14-bit outputs)
    output                    da1_clk,
    output                    da1_wrt,
    output [13:0]             da1_data,
    output                    da2_clk,
    output                    da2_wrt,
    output [13:0]             da2_data,
    // Debug outputs
    output [15:0]             debug_downsampledY,
    output [15:0]             debug_upsampledY,
    output                    debug_ce_out_down,
    output                    debug_ce_out_up,
    output [11:0]             debug_adc_input
);

    // ========== ADC Interface ==========
    wire [11:0] ad_data_ch0_12;
    wire [11:0] ad_data_ch1_12;

    adc u_adc (
        .sys_clk      (sys_clk),
        .rst_n        (rst_n),
        .adc_data_ch0 (adc_data_ch0),
        .adc_data_ch1 (adc_data_ch1),
        .adc_clk_ch0  (adc_clk_ch0),
        .adc_clk_ch1  (adc_clk_ch1),
        .ad_data_ch0  (ad_data_ch0_12),
        .ad_data_ch1  (ad_data_ch1_12)
    );

    // ========== DSP Filters ==========
    reg signed [11:0] filter_in;
    always @(posedge sys_clk)
        filter_in <= {~ad_data_ch0_12[11], ad_data_ch0_12[10:0]}; // sign-extend and invert MSB

    wire signed [15:0] downsampledY;
    wire               ce_out_down;

    downsamplerFilter downDsp (
        .clk         (sys_clk),
        .clk_enable  (1'b1),
        .reset       (rst_n), // active-high reset
        .filter_in   (filter_in),
        .filter_out  (downsampledY),
        .ce_out      (ce_out_down)
    );

    wire signed [15:0] upsampledY;
    wire               ce_out_up;

    upsamplerFilter upDsp (
        .clk         (sys_clk),
        .clk_enable  (1'b1),
        .reset       (rst_n), // active-high reset
        .filter_in   (downsampledY),
        .filter_out  (upsampledY),
        .ce_out      (ce_out_up)
    );

    // ========== DAC Output ==========
    wire signed [13:0] dac1_input_14 = downsampledY[15:2] + 14'd8192;
    wire signed [13:0] dac2_input_14 = upsampledY[15:2]   + 14'd8192;

    reg [13:0] dac1_input_14_reg, dac2_input_14_reg;
    always @(posedge sys_clk) begin
        dac1_input_14_reg <= dac1_input_14;
        dac2_input_14_reg <= dac2_input_14;
    end

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

    // ========== Debug outputs ==========
    assign debug_downsampledY = downsampledY;
    assign debug_upsampledY   = upsampledY;
    assign debug_ce_out_down  = ce_out_down;
    assign debug_ce_out_up    = ce_out_up;
    assign debug_adc_input    = filter_in; // extend signed 12-bit to 16-bit for display

endmodule
