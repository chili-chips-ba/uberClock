`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//正选波发生器--2路输出 -10V ~ +10V
//////////////////////////////////////////////////////////////////////////////////
module ad9767_test(
     input sys_clk_p,           // 开发板上差分输入时钟200Mhz， 正极
     input sys_clk_n,           // 开发板上差分输入时钟200Mhz， 负极  
	 
	 output da1_clk,             //DA1 时钟信号
	 output da1_wrt,             //DA1 数据写信号
     output [13:0] da1_data,     //DA1 data
	 
	 output da2_clk,             //DA2 时钟信号
	 output da2_wrt,	         //DA2 数据写信号
     output [13:0] da2_data      //DA2 data

    );


reg [9:0] rom_addr;

wire [13:0] rom_data;
wire clk_50;
wire clk_125;


assign da1_clk=clk_125;
assign da1_wrt=clk_125;
assign da1_data=rom_data;

assign da2_clk=clk_125;
assign da2_wrt=clk_125;
assign da2_data=rom_data;

//===========================================================================
// 差分时钟转换成单端时钟
//===========================================================================
wire sys_clk_ibufg;
 IBUFGDS #
       (
        .DIFF_TERM    ("FALSE"),
        .IBUF_LOW_PWR ("FALSE")
        )
       u_ibufg_sys_clk
         (
          .I  (sys_clk_p),            //差分时钟的正端输入，需要和顶层模块的端口直接连接
          .IB (sys_clk_n),           // 差分时钟的负端输入，需要和顶层模块的端口直接连接
          .O  (sys_clk_ibufg)        //时钟缓冲输出
          );

//DA output sin waveform
always @(negedge clk_125)
begin
     rom_addr <= rom_addr + 1'b1 ;              //一个正选波采样点为1024,输出正选波频率125/1024=122Khz
	 // rom_addr <= rom_addr + 4 ;               //一个正选波采样点为256,输出正选波频率125/256=488Khz 
	 // rom_addr <= rom_addr + 128 ;             //一个正选波采样点为8,输出正选波频率125/1024=15.6Mhz


										
end 



ROM ROM_inst (
  .clka(clk_125), // input clka
  .addra(rom_addr), // input [8 : 0] addra
  .douta(rom_data) // output [7 : 0] douta
);


PLL PLL_inst
   (// Clock in ports
    .clk_in1(sys_clk_ibufg),      // IN
    // Clock out ports
    .clk_out1(clk_50),     // OUT
    .clk_out2(clk_125),     // OUT
    // Status and control signals
    .reset(1'b0),// IN
    .locked());      // OUT


endmodule
