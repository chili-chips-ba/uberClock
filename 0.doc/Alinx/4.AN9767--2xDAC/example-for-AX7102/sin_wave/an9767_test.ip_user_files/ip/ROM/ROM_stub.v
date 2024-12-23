// Copyright 1986-2015 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2015.4 (win64) Build 1412921 Wed Nov 18 09:43:45 MST 2015
// Date        : Thu May 04 10:54:21 2017
// Host        : LAOLUO-PC running 64-bit major release  (build 7600)
// Command     : write_verilog -force -mode synth_stub
//               e:/Project/AN9767/CD/CD/verilog/ad9767_ax7102/sin_wave/an9767_test.srcs/sources_1/ip/ROM/ROM_stub.v
// Design      : ROM
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7a100tfgg484-2
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* x_core_info = "blk_mem_gen_v8_3_1,Vivado 2015.4" *)
module ROM(clka, addra, douta)
/* synthesis syn_black_box black_box_pad_pin="clka,addra[9:0],douta[13:0]" */;
  input clka;
  input [9:0]addra;
  output [13:0]douta;
endmodule
