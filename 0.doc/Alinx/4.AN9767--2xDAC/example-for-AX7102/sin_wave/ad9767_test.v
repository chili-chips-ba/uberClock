`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
//��ѡ��������--2·��� -10V ~ +10V
//////////////////////////////////////////////////////////////////////////////////
module ad9767_test(
     input sys_clk_p,           // �������ϲ������ʱ��200Mhz�� ����
     input sys_clk_n,           // �������ϲ������ʱ��200Mhz�� ����  
	 
	 output da1_clk,             //DA1 ʱ���ź�
	 output da1_wrt,             //DA1 ����д�ź�
     output [13:0] da1_data,     //DA1 data
	 
	 output da2_clk,             //DA2 ʱ���ź�
	 output da2_wrt,	         //DA2 ����д�ź�
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
// ���ʱ��ת���ɵ���ʱ��
//===========================================================================
wire sys_clk_ibufg;
 IBUFGDS #
       (
        .DIFF_TERM    ("FALSE"),
        .IBUF_LOW_PWR ("FALSE")
        )
       u_ibufg_sys_clk
         (
          .I  (sys_clk_p),            //���ʱ�ӵ��������룬��Ҫ�Ͷ���ģ��Ķ˿�ֱ������
          .IB (sys_clk_n),           // ���ʱ�ӵĸ������룬��Ҫ�Ͷ���ģ��Ķ˿�ֱ������
          .O  (sys_clk_ibufg)        //ʱ�ӻ������
          );

//DA output sin waveform
always @(negedge clk_125)
begin
     rom_addr <= rom_addr + 1'b1 ;              //һ����ѡ��������Ϊ1024,�����ѡ��Ƶ��125/1024=122Khz
	 // rom_addr <= rom_addr + 4 ;               //һ����ѡ��������Ϊ256,�����ѡ��Ƶ��125/256=488Khz 
	 // rom_addr <= rom_addr + 128 ;             //һ����ѡ��������Ϊ8,�����ѡ��Ƶ��125/1024=15.6Mhz


										
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
