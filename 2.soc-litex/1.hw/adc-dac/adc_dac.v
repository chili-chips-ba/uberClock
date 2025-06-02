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

    // DDR‐output DAC (14-bit output; AN9767 on J13)
    output                    da1_clk,         // DA1 clock (DDR‐output)
    output                    da1_wrt,         // DA1 write strobe (DDR‐output)
    output [13:0]             da1_data,        // DA1 14‐bit data bus (DDR‐output)
    output                    da2_clk,         // DA2 clock (DDR‐output)
    output                    da2_wrt,         // DA2 write strobe (DDR‐output)
    output [13:0]             da2_data         // DA2 14‐bit data bus (DDR‐output)
);

    //======================================================================
    //  Internal signals
    //======================================================================

    assign adc_clk_ch0 = sys_clk;
    assign adc_clk_ch1 = sys_clk;

    //======================================================================
    // Sample each AD channel and produce buffer write signals
    //======================================================================
    // Channel 0 sampler
    adc adc_sample_m0 (
        .adc_clk      (sys_clk),
        .rst          (~rst_n),
        .adc_data     (adc_data_ch0),
        .adc_buf_wr   (/* unused */),
        .adc_buf_addr (/* unused */),
        .adc_buf_data (/* unused */)
    );

    // Channel 1 sampler
    adc adc_sample_m1 (
        .adc_clk      (sys_clk),
        .rst          (~rst_n),
        .adc_data     (adc_data_ch1),
        .adc_buf_wr   (/* unused */),
        .adc_buf_addr (/* unused */),
        .adc_buf_data (/* unused */)
    );

    //======================================================================
    //  Sign‐extend each 12‐bit ADC sample into a 14‐bit two’s‐complement
    //  {MSB,MSB, [11:0]} yields a 14-bit word
    //======================================================================
    wire [13:0] dac1_input = { {2{adc_data_ch0[11]}}, adc_data_ch0 };
    wire [13:0] dac2_input = { {2{adc_data_ch1[11]}}, adc_data_ch1 };

    //======================================================================
    // Instantiate the DDR-output DAC module
    //======================================================================
    dac u_dac (
        .sys_clk   (sys_clk),
        .rst_n     (rst_n),

        .data1     (dac1_input),  // 14-bit sine word from channel 0
        .data2     (dac2_input),  // 14-bit cosine word from channel 1

        .da1_clk   (da1_clk),
        .da1_wrt   (da1_wrt),
        .da1_data  (da1_data),

        .da2_clk   (da2_clk),
        .da2_wrt   (da2_wrt),
        .da2_data  (da2_data)
    );

endmodule
