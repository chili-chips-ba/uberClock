`timescale 1ns/1ps

module tb_cordic_downconvert;

  //-------------------------------------------------------------------------
  // Clock Generation: 200 MHz differential clock (5 ns period)
  //-------------------------------------------------------------------------
  reg sys_clk_p;
  reg sys_clk_n;
  initial begin
      sys_clk_p = 0;
      forever #2.5 sys_clk_p = ~sys_clk_p;  // 200 MHz: period = 5 ns
  end

  always @(*) begin
      sys_clk_n = ~sys_clk_p;
  end

  //--------------------------------------------
  // Reset Generation: Active low reset (rst_n) 
  //--------------------------------------------
  reg rst_n;
  initial begin
      rst_n = 0;
      #50;
      rst_n = 1;
  end

  //-------------------------------------------------------------------------
  // Downconversion Module
  //-------------------------------------------------------------------------
  wire [12:0] down_xval;  // Downconverted X output (13-bit wide)
  wire [12:0] down_yval;  // Downconverted Y output (13-bit wide)

  cordic_downconvert uut (
      .sys_clk_p(sys_clk_p),
      .sys_clk_n(sys_clk_n),
      .rst_n(rst_n),
      .down_xval(down_xval),
      .down_yval(down_yval)
  );


  initial begin
      #150000; 
      $finish;
  end

endmodule
