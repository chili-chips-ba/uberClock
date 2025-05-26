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
   input [1:0]  key_in,

// LEDs
   output [1:0] led
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

//==========================================================================
// GPIO
//==========================================================================
   assign led[1] = ~from_csr.gpio.led2.value;
   assign led[0] = ~from_csr.gpio.led1.value;
   assign to_csr.gpio.key2.next = ~key_in[1];  //TODO: Add debounce
   assign to_csr.gpio.key1.next = ~key_in[0];  //TODO: Add debounce

endmodule
