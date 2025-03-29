`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/18/2025 06:25:39 PM
// Design Name: 
// Module Name: AD_DA_interface
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module AD_DA_interface(
    // System inputs
    input                sys_clk_p,       // System clock positive
    input                sys_clk_n,       // System clock negative
    input                rst_n,           // Reset (active low)
    
    // ADC input
    input [11:0]         adc_ch1_data,    // ADC channel 1 data
    output               adc_clk,         // ADC sampling clock
    
    // DAC output
    output               dac_ch1_clk,     // DAC channel 1 clock
    output               dac_ch1_wrt,     // DAC channel 1 write enable
    output [13:0]        dac_ch1_data     // DAC channel 1 data output
);
    // Internal signals
    wire                 sys_clk;         // Single-ended clock
    wire                 adc_proc_clk;    // Clock for ADC data processing
    wire                 dac_proc_clk;    // Clock for DAC data processing
    wire [13:0]          adc_ch1_data_ext; // Extended ADC channel 1 data

    // Differential to single-ended clock conversion
    IBUFDS sys_clk_ibufgds (
        .O                (sys_clk),
        .I                (sys_clk_p),
        .IB               (sys_clk_n)
    );

    // Generate ADC processing clock
    adc_pll adc_pll_m0 (
    .clk_in1                        (sys_clk                  ),
    .clk_out1                       (adc_clk                  ),
    .reset                          (1'b0                     ),
    .locked                         (                         )
    );

    // Assign ADC clock signal
    assign adc_clk = adc_proc_clk;

    // Generate DAC processing clock
    ad9238_sample ad9238_sample_m0 (
        .adc_clk          (adc_clk),
        .rst              (~rst_n),
        .adc_data         (ad9238_data_ch1), // Input ADC data
        .adc_buf_wr       (adc_buf_wr),      // Write enable
        .adc_buf_addr     (adc_buf_addr),    // Buffer address
        .adc_buf_data     (adc_buf_data)     // Buffered data
    );

    // Assign DAC clock and write enable signals
    assign dac_ch1_clk = dac_proc_clk;
    assign dac_ch1_wrt = dac_proc_clk;

    // Extend ADC data to match DAC input width (zero padding)
    assign adc_ch1_data_ext = {adc_ch1_data, 2'b00}; // Extend 12-bit to 14-bit

    // Assign ADC data to DAC channel 1
    assign dac_ch1_data = adc_ch1_data_ext; // ADC channel 1 -> DAC channel 1


endmodule
