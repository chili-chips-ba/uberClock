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
   //sys
   input                            sys_clk_p,               //system clock positive
   input                            sys_clk_n,               //system clock negative 
   input                            rst_n,                   //reset ,low active
    
   //ADC
   output                           ad9238_clk_ch0,          //AD channel 0 sampling clock
   output                           ad9238_clk_ch1,          //AD channel 1 sampling clock
   input[11:0]                      ad9238_data_ch0,         //AD channel 0 data 
   input[11:0]                      ad9238_data_ch1,         //AD channel 1 data 
    
   //DAC
   output                           da1_clk,                 //DA1 clock signal
   output                           da1_wrt,                 //DA1 data write signal
   output[13:0]                     da1_data,                //DA1 data  
   output                           da2_clk,                 //DA2 clock signal
   output                           da2_wrt,                 //DA2 data write signal
   output[13:0]                     da2_data                 //DA2 data
);

   wire                            adc_clk;                 //ADC data processing clock
   wire                            adc0_buf_wr;             //ADC channel 0 write buf enable
   wire[10:0]                      adc0_buf_addr;           //ADC channel 0 write buf address
   wire[7:0]                       adc0_buf_data;           //ADC channel 0 buf data
   wire                            adc1_buf_wr;             //ADC channel 1 write buf enable       
   wire[10:0]                      adc1_buf_addr;           //ADC channel 1 write buf address
   wire[7:0]                       adc1_buf_data;           //ADC channel 1 data 
   wire                            sys_clk;                 //single end clock 

   assign ad9238_clk_ch0            = adc_clk;
   assign ad9238_clk_ch1            = adc_clk;

   /*************************************************************************
   generate single end clock
   **************************************************************************/
   IBUFDS sys_clk_ibufgds
   (
      .O                              (sys_clk                 ),
      .I                              (sys_clk_p               ),
      .IB                             (sys_clk_n               )
   );

   /*************************************************************************
   Generate the clock required for the AD data processing
   ***************************************************************************/
   adc_pll adc_pll_m0 (
      .clk_out1 (adc_clk),
      .reset    (1'b0),
      .locked   (),
      .clk_in1  (sys_clk)
   );

   /*************************************************************************
   Sampling channel 0 data of the ad9238 and generating RAM signal
   ***************************************************************************/
   ad9238_sample ad9238_sample_m0
   (
      .adc_clk                        (adc_clk                  ),
      .rst                            (~rst_n                   ),
      .adc_data                       (ad9238_data_ch0          ),
      .adc_buf_wr                     (adc0_buf_wr              ),
      .adc_buf_addr                   (adc0_buf_addr            ),
      .adc_buf_data                   (adc0_buf_data            )
   );

   /*************************************************************************
   Sampling channel 1 data of the ad9238 and generating RAM signal
   ***************************************************************************/
   ad9238_sample ad9238_sample_m1
   (
      .adc_clk                        (adc_clk                  ),
      .rst                            (~rst_n                   ),
      .adc_data                       (ad9238_data_ch1          ),
      .adc_buf_wr                     (adc1_buf_wr              ),
      .adc_buf_addr                   (adc1_buf_addr            ),
      .adc_buf_data                   (adc1_buf_data            )
   );


   assign da1_clk = adc_clk;
   assign da1_wrt = adc_clk;

   assign da2_clk = adc_clk;
   assign da2_wrt = adc_clk;


    wire signed [11:0] ad_ch0 = -ad9238_data_ch0;
    wire signed [11:0] ad_ch1 = -ad9238_data_ch1;


//    assign da1_data = ({ad9238_data_ch0, 2'b00});
//    assign da2_data = ({ad9238_data_ch1, 2'b00});

    assign da1_data = {{2{ad9238_data_ch0[11]}} , ad9238_data_ch0};
    assign da2_data = {{2{ad9238_data_ch1[11]}} , ad9238_data_ch1};
endmodule
