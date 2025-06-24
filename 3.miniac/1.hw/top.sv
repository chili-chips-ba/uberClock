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
      .sys_rst_n       (sys_rst_n)
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
   

// We instantiate the ADC module
    wire [11:0] ad_data_ch1_12;
    wire [11:0] ad_data_ch2_12;

adc u_adc (
        .sys_clk      (sys_clk),
        .rst_n        (rst_n),
        // Raw DDR-pinned inputs from the board
        .adc_data_ch0 (ad9238_data_ch0), //channel 1 input from ADC to the module
        .adc_data_ch1 (ad9238_data_ch1), //channel 2 input from ADC to the module
        // DDR clocks to drive each AD9238 chip
        .adc_clk_ch0  (ad9238_clk_ch0),
        .adc_clk_ch1  (ad9238_clk_ch1),
        // 12-bit, single-clock-domain outputs (rising-edge captures)
        .ad_data_ch0  (ad_data_ch1_12), //Modules output data which we send to CSR; Channel 1
        .ad_data_ch1  (ad_data_ch2_12) //Modules output data which we send to CSR; Channel 2
    );
    
  
 assign to_csr.adc.ch1.next = ad_data_ch1_12;
 assign to_csr.adc.ch2.next = ad_data_ch2_12;
 
   
   wire[13:0] da_data_ch1_14 = from_csr.dac.ch1.value;
   wire[13:0] da_data_ch2_14 = from_csr.dac.ch2.value;
   
   
//We instatiate the DAC module
   dac u_dac (
            .sys_clk   (sys_clk),
            .rst_n     (rst_n),
            .data1     (da_data_ch1_14), //dac1_input_14_reg
            .data2     (da_data_ch2_14), //dac2_input_14_reg
            .da1_clk   (da1_clk),//da1_clk
            .da1_wrt   (da1_wrt),//da1_wrt
            .da1_data  (da1_data),
            .da2_clk   (da2_clk),//da2_clk
            .da2_wrt   (da2_wrt),//da2_wrt
            .da2_data  (da2_data)
    );


//==========================================================================
// GPIO
//==========================================================================
   assign led[1] = ~from_csr.gpio.led2.value;
   assign led[0] = ~from_csr.gpio.led1.value;
   assign to_csr.gpio.key2.next = ~user_key2;  //TODO: Add debounce
   assign to_csr.gpio.key1.next = ~user_key1;  //TODO: Add debounce

endmodule
