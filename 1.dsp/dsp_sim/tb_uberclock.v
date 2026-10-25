// ------------------------------------------------------------
// Testbench
// ------------------------------------------------------------
`timescale 1ns/1ps

module tb_uberclock;

  localparam integer CLK65_CYCLES_5MS = 325_000;
  localparam integer CLK65_CYCLES_8MS = 520_000;

  // DUT inputs
  reg  sys_clk_p = 1'b0;
  reg  sys_clk_n = 1'b1;
  reg  rst       = 1'b1;

  // Final scaler
  reg  [2:0] final_shift = 3'd2;

  // Phase increments (PW=19 in the DUT)
  reg  [18:0] phase_inc_down_1 = 19'd80656;
  reg  [18:0] phase_inc_down_2 = 19'd80652;
  reg  [18:0] phase_inc_down_3 = 19'd80648;
  reg  [18:0] phase_inc_down_4 = 19'd80644;
  reg  [18:0] phase_inc_down_5 = 19'd80640;

  // Gains
  reg  [31:0] gain1 = 32'h4000_0000;
  reg  [31:0] gain2 = 32'h4000_0000;
  reg  [31:0] gain3 = 32'h4000_0000;
  reg  [31:0] gain4 = 32'h4000_0000;
  reg  [31:0] gain5 = 32'h4000_0000;

  // DUT
  uberclock #(.IW(12), .OW(12), .NSTAGES(15), .WW(15), .PW(19)) dut (
    .sys_clk_p(sys_clk_p),
    .sys_clk_n(sys_clk_n),
    .rst(rst),
    .final_shift(final_shift),
    .phase_inc_down_1(phase_inc_down_1),
    .phase_inc_down_2(phase_inc_down_2),
    .phase_inc_down_3(phase_inc_down_3),
    .phase_inc_down_4(phase_inc_down_4),
    .phase_inc_down_5(phase_inc_down_5),
    .gain1(gain1), .gain2(gain2), .gain3(gain3), .gain4(gain4), .gain5(gain5)
  );

  // ----------------------------------------------------------
  // 200 MHz differential reference clock (5 ns period)
  // ----------------------------------------------------------
  initial begin
    forever begin
      #2.5 sys_clk_p = ~sys_clk_p;  // toggle every 2.5 ns -> 200 MHz
           sys_clk_n = ~sys_clk_p;
    end
  end

  // ----------------------------------------------------------
  // Reset sequence
  // ----------------------------------------------------------
  initial begin
    rst = 1'b1;
    repeat (10) @(posedge clk_fabric);
    rst = 1'b0;
  end


  // convenience aliases (hierarchical taps)
  wire               clk_fabric = dut.sys_clk; // internal fabric clock
  wire signed [11:0] nco_cos    = dut.nco_cos;

  wire signed [15:0] dy1 = dut.downsampled_y1;
  wire signed [15:0] dy2 = dut.downsampled_y2;
  wire signed [15:0] dy3 = dut.downsampled_y3;
  wire signed [15:0] dy4 = dut.downsampled_y4;
  wire signed [15:0] dy5 = dut.downsampled_y5;

  // per-channel CE for downsampled_y* (inside rx_channel)
  wire ce_y1 = dut.rx_1.ce_out_down_y;
  wire ce_y2 = dut.rx_2.ce_out_down_y;
  wire ce_y3 = dut.rx_3.ce_out_down_y;
  wire ce_y4 = dut.rx_4.ce_out_down_y;
  wire ce_y5 = dut.rx_5.ce_out_down_y;

  // tx outputs
  wire signed [15:0] tx1 = dut.tx_channel_output1;
  wire signed [15:0] tx2 = dut.tx_channel_output2;
  wire signed [15:0] tx3 = dut.tx_channel_output3;
  wire signed [15:0] tx4 = dut.tx_channel_output4;
  wire signed [15:0] tx5 = dut.tx_channel_output5;

  // system output
  wire signed [13:0] system_out_14 = dut.system_output_14;

  // helper: true if vector is unknown
function is_xv;
  input [63:0] v;
  begin
    is_xv = (^v === 1'bx);
  end
endfunction


  // ----------------------------------------------------------
  // Run window + close files
  // ----------------------------------------------------------
  integer nco_cos_fd;
  integer dy1_fd, dy2_fd, dy3_fd, dy4_fd, dy5_fd;
  integer tx1_fd, tx2_fd, tx3_fd, tx4_fd, tx5_fd;
  integer clk65_count;

  initial begin
    nco_cos_fd = $fopen("nco_cos.txt", "w");
    dy1_fd     = $fopen("downsampled_y1.txt", "w");
    dy2_fd     = $fopen("downsampled_y2.txt", "w");
    dy3_fd     = $fopen("downsampled_y3.txt", "w");
    dy4_fd     = $fopen("downsampled_y4.txt", "w");
    dy5_fd     = $fopen("downsampled_y5.txt", "w");
    tx1_fd     = $fopen("tx_out1.txt", "w");
    tx2_fd     = $fopen("tx_out2.txt", "w");
    tx3_fd     = $fopen("tx_out3.txt", "w");
    tx4_fd     = $fopen("tx_out4.txt", "w");
    tx5_fd     = $fopen("tx_out5.txt", "w");
    clk65_count = 0;
  end

  always @(posedge clk_fabric) begin
    if (rst) begin
      clk65_count <= 0;
    end else begin
      clk65_count <= clk65_count + 1;

      $fwrite(nco_cos_fd, "%0d\n", nco_cos);

      if (clk65_count >= CLK65_CYCLES_5MS) begin
        if (ce_y1) $fwrite(dy1_fd, "%0d\n", dy1);
        if (ce_y2) $fwrite(dy2_fd, "%0d\n", dy2);
        if (ce_y3) $fwrite(dy3_fd, "%0d\n", dy3);
        if (ce_y4) $fwrite(dy4_fd, "%0d\n", dy4);
        if (ce_y5) $fwrite(dy5_fd, "%0d\n", dy5);
      end

      if (clk65_count >= CLK65_CYCLES_8MS) begin
        $fwrite(tx1_fd, "%0d\n", tx1);
        $fwrite(tx2_fd, "%0d\n", tx2);
        $fwrite(tx3_fd, "%0d\n", tx3);
        $fwrite(tx4_fd, "%0d\n", tx4);
        $fwrite(tx5_fd, "%0d\n", tx5);
      end
    end
  end

  initial begin
    // let it run a while; adjust as needed
    repeat (10_000_000) @(posedge clk_fabric);
    $fclose(nco_cos_fd);
    $fclose(dy1_fd);
    $fclose(dy2_fd);
    $fclose(dy3_fd);
    $fclose(dy4_fd);
    $fclose(dy5_fd);
    $fclose(tx1_fd);
    $fclose(tx2_fd);
    $fclose(tx3_fd);
    $fclose(tx4_fd);
    $fclose(tx5_fd);
    $display("TB finished at %0t", $time);
    $finish;
  end

endmodule
