//==========================================================================
// Copyright (C) 2024-2025 Chili.CHIPS*ba
//--------------------------------------------------------------------------
//                      PROPRIETARY INFORMATION
//
// The information contained in this file is the property of CHILI CHIPS LLC.
// Except as specifically authorized in writing by CHILI CHIPS LLC, the holder
// of this file: (1) shall keep all information contained herein confidential;
// and (2) shall protect the same in whole or in part from disclosure and
// dissemination to all third parties; and (3) shall use the same for operation
// and maintenance purposes only.
//--------------------------------------------------------------------------
// Description:
//   uberClock top-level module
//==========================================================================

module top (
   input        clk_p,        //board clock positive
   input        clk_n,        //board clock negative
   input        rst_n,        //reset, low active

// UART
   input        uart_rx,
   output       uart_tx,

// Keys
   input  user_key1,
   input  user_key2,

// LEDs
   output [3:0] led,
   
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
   wire                            dac_clk;                 //single end clock 
   
  

   assign ad9238_clk_ch0            = adc_clk;
   assign ad9238_clk_ch1            = adc_clk;
   
   
   import csr_pkg::*;
   import soc_pkg::*;

//==========================================================================
// Clock and reset generation
//==========================================================================
   logic               sys_clk;
   logic               sys_rst;
   logic               sys_rst_n;

   clk_rst_gen u_clk_rst_gen (
      .clk_p           (clk_p),
      .clk_n           (clk_n),
      .rst_n           (rst_n),
      .sys_clk         (sys_clk),
      .sys_rst         (sys_rst),
      .sys_rst_n       (sys_rst_n),
      .adc_clk          (adc_clk),
      .dac_clk          (dac_clk)
   );

   csr_pkg::csr__in_t  to_csr;
   csr_pkg::csr__out_t from_csr;

//=================================
// CPU Subsystem
//=================================
   localparam          NUM_WORDS_IMEM = 8192; //=> 32kB InstructionRAM
   localparam          NUM_WORDS_DMEM = 8192; //=> 32kB DataRAM

   soc_if              bus_cpu       (.arst_n(sys_rst_n), .clk(sys_clk));
   soc_if              bus_uart      (.arst_n(sys_rst_n), .clk(sys_clk));
   soc_if              bus_dmem      (.arst_n(sys_rst_n), .clk(sys_clk));
   soc_if              bus_csr       (.arst_n(sys_rst_n), .clk(sys_clk));

   logic               imem_cpu_rstn;
   logic               imem_we;
   logic [31:2]        imem_waddr;
   logic [31:0]        imem_wdat;

//---------------------------------
   soc_cpu #(
      .ADDR_RESET      (32'h 0000_0000),
      .NUM_WORDS_IMEM  (NUM_WORDS_IMEM)
   ) u_cpu (
      .bus             (bus_cpu),       //MST

      .imem_cpu_rstn   (imem_cpu_rstn), //-\ access point
      .imem_we         (imem_we),       //-| for
      .imem_waddr      (imem_waddr),    // | reloading CPU
      .imem_wdat       (imem_wdat)      //-/ program memory
   );

//---------------------------------
   soc_fabric u_fabric (
      .cpu             (bus_cpu),       //SLV
      .uart            (bus_uart),      //SLV

      .dmem            (bus_dmem),      //MST
      .csr             (bus_csr)        //MST
   );

//---------------------------------
   soc_ram #(
      .NUM_WORDS       (NUM_WORDS_DMEM)
   ) u_dmem (
      .bus             (bus_dmem)       //SLV
   );

//---------------------------------
   soc_csr u_soc_csr (
      .bus             (bus_csr),       //SLV
      .hwif_in         (to_csr),        //i
      .hwif_out        (from_csr)       //o
   );

//---------------------------------
   uart u_uart (
      .arst_n          (sys_rst_n),     //i
      .clk             (sys_clk),       //i

      .uart_rx         (uart_rx),   //i
      .uart_tx         (uart_tx),       //o

      .from_csr        (from_csr),      //i
      .to_csr          (to_csr),        //o

      .imem_cpu_rstn   (imem_cpu_rstn), //o
      .imem_we         (imem_we),       //o
      .imem_waddr      (imem_waddr),    //o[31:2]
      .imem_wdat       (imem_wdat),     //o[31:0]
      
      .bus             (bus_uart)       //MST
   );   

   assign da1_clk = sys_clk;
   assign da1_wrt = sys_clk;

   assign da2_clk = sys_clk;
   assign da2_wrt = sys_clk;
   



   // output absolute sampled values
//   assign da1_data = ad9238_data_ch0[11] ? {1'b0, ad9238_data_ch0[10:0], 2'b00} : {ad9238_data_ch0, 2'b00};
//   assign da2_data = ad9238_data_ch1[11] ? {1'b0, ad9238_data_ch1[10:0], 2'b00} : {ad9238_data_ch1, 2'b00};
   // assign da1_data = ad9238_data_ch0[11] ? -({ad9238_data_ch0, 2'b00}) : ({ad9238_data_ch0, 2'b00});
  // assign da2_data = ad9238_data_ch1[11] ? -({ad9238_data_ch1, 2'b00}) : ({ad9238_data_ch1, 2'b00});
 // assign da2_data = ad9238_data_ch1[11] ? -({ad9238_data_ch1, 2'b00}) : ({ad9238_data_ch1, 2'b00});
 
 assign to_csr.adc.ch1.next = ad9238_data_ch0;
 assign to_csr.adc.ch2.next = ad9238_data_ch1;
 
 
 assign da1_data = from_csr.dac.ch1.value;
 assign da2_data = from_csr.dac.ch2.value;


   //assign da1_data = ({ad9238_data_ch0, 2'b00});

//==========================================================================
// GPIO
//==========================================================================
   assign led[1] = ~from_csr.gpio.led2.value;
   assign led[0] = ~from_csr.gpio.led1.value;
   assign to_csr.gpio.key2.next = ~user_key2;  //TODO: Add debounce
   assign to_csr.gpio.key1.next = ~user_key1;  //TODO: Add debounce

endmodule
