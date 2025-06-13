// ============================================================================
//  adc_dac.v
// ============================================================================
`timescale 1ns / 1ps

module adc_dac(
    input                     sys_clk,
    input                     rst_n,

    // ADC (12-bit inputs; AD9238 on J11)
    output                    adc_clk_ch0,  // AD channel 0 sampling clock
    output                    adc_clk_ch1,  // AD channel 1 sampling clock
    input  [11:0]             adc_data_ch0, // AD channel 0 data
    input  [11:0]             adc_data_ch1, // AD channel 1 data

    
     // ————————————————————————————————————————————
     // Debug outputs for LiteScope
     output [11:0]             debug_adc_data_ch0,
     output [11:0]             debug_adc_data_ch1,
     output [13:0]             debug_dac_data1,
     output [13:0]             debug_dac_data2,
     // ————————————————————————————————————————————

    // DDR‐output DAC (14-bit output; AN9767 on J13)
    output                    da1_clk,         // DA1 clock (DDR‐output)
    output                    da1_wrt,         // DA1 write strobe (DDR‐output)
    output [13:0]             da1_data,        // DA1 14‐bit data bus (DDR‐output)
    output                    da2_clk,         // DA2 clock (DDR‐output)
    output                    da2_wrt,         // DA2 write strobe (DDR‐output)
    output [13:0]             da2_data         // DA2 14‐bit data bus (DDR‐output)
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

    //======================================================================
    //  Sign‐extend each 12‐bit ADC sample into a 14‐bit two’s‐complement
    //  {MSB,MSB, [11:0]} yields a 14-bit word
    //======================================================================
    // wire [13:0] dac1_input_14 = {{2{ad_data_ch0_12[11]}}, ad_data_ch0_12};
    // wire [13:0] dac2_input_14 = {{2{ad_data_ch1_12[11]}}, ad_data_ch1_12};

    wire [13:0] dac1_input_14 = ad_data_ch0_12 << 2;
    wire [13:0] dac2_input_14 = ad_data_ch1_12 << 2;
    //======================================================================
    // Instantiate the DDR-output DAC module
    //======================================================================
    dac u_dac (
            .sys_clk   (sys_clk),
            .rst_n     (rst_n),

            .data1     (dac1_input_14),
            .data2     (dac2_input_14),

            .da1_clk   (da1_clk),
            .da1_wrt   (da1_wrt),
            .da1_data  (da1_data),

            .da2_clk   (da2_clk),
            .da2_wrt   (da2_wrt),
            .da2_data  (da2_data)
    );

   assign debug_adc_data_ch0 = ad_data_ch0_12;
   assign debug_adc_data_ch1 = ad_data_ch1_12;
   assign debug_dac_data1    = dac1_input_14;
   assign debug_dac_data2    = dac2_input_14;
   
endmodule
