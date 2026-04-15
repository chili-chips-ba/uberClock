//==========================================================================
// Copyright (C) 2024 Chili.CHIPS*ba
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
// Description: Simulation testbench for FPGA. It provides models of
//              external resources, as connected on the actual board
//==========================================================================

`timescale 1ps/1ps

module tb #(
   parameter int RUN_SIM_US     = 10000
)();
   glbl glbl();

   //localparam DLY_PCB_PAD_PS    = 1_000;

//--------------------------------------------------------------
// Generate clocks and run sim for the specified amount of time
   localparam  HALF_CLK_P_PERIOD_PS      =   2_500; // 200MHz
   localparam  HALF_PERIOD_OTHER_BFMS_PS =   6_250; // 80 MHz

   localparam  RST_CYLES      = 10;

   logic       clk_p, clk_n;
   logic       rst_n;
   logic       clk_80;
   logic [2:1] key;

   initial begin
      clk_p      = 1'b0;
      clk_n      = 1'b0;
      rst_n      = 1'b0;
      clk_80     = 1'b0;
      key        = 2'b0;

      fork
         begin : board_rst_n
            #(HALF_CLK_P_PERIOD_PS*RST_CYLES * 1ps)
               rst_n = 1'b1;
         end

         forever begin: clk_p_gen
            #(HALF_CLK_P_PERIOD_PS * 1ps)
               clk_p  = ~clk_p;
         end

         forever begin: clk_80_gen
            #(HALF_PERIOD_OTHER_BFMS_PS * 1ps)
               clk_80 = ~clk_80;
         end

         begin: run_sim
            #(RUN_SIM_US * 1us);
               $finish(2);
         end
      join
   end

   assign clk_n = ~clk_p;

//--------------------------------------------------------------

   logic        uart_rx, uart_tx;
// verilator lint_off UNUSEDSIGNAL
   logic [3:2]  led;
// verilator lint_off UNUSEDSIGNAL

   top dut (
      .clk_p   (clk_p),
      .clk_n   (clk_n),
      .rst_n   (rst_n),

      // UART
      .uart_rx (uart_rx),
      .uart_tx (uart_tx),

      // Keys
      .key_in  (key),

      // LEDs
      .led     (led)
   );

//--------------------------------------------------------------
// model of external UART
//--------------------------------------------------------------
   bfm_uart bfm_uart (
      .uart_rx  (uart_tx), //i
      .uart_tx  (uart_rx)  //o
   );

//--------------------------------------------------------------
// Error monitor
//--------------------------------------------------------------
/*    always @(dut.csr.misc.error) begin: error_mon
       $write("%t %m (%0d) - ", $time, dut.csr.misc.error);

       case (dut.csr.misc.error)
         4'd0   : $display("ERROR_CLEARED");

         4'd1   : $display("UART_RX_OVERFLOW");
         4'd2   : $display("ANOTHER_CMD_PENDING (drop new Cmd)");
         4'd3   : $display("ANOTHER_CMD_IN_RX (interrupt it and start new Cmd)");
         4'd4   : $display("UNEXPECTED_CHAR");
         4'd5   : $display("ILLEGAL_CMD_CHAR");
         4'd6   : $display("INVALID_ANUM (Cmd dropped)");
         4'd7   : $display("ADC1_CMD_DROPPED");
         4'd8   : $display("ADC2_CMD_DROPPED");
         4'd9   : $display("DAC1_CMD_DROPPED");
         4'd10  : $display("DAC2_CMD_DROPPED");
         4'd11  : $display("INVALID_CMD_ID");

         default: $display("UNKNOWN");
       endcase
    end
*/

//--------------------------------------------------------------
/* Assertions have to be done like this

    always_ff @(posedge clk) begin

       $display("a = %b, b = %b, c = %b, y = %b",a,b,c,y);

       if ($past(b) > 2'b0) begin
          if (y !== 1'b1) $fatal("Assertion failed for Test Case: b > 2'b0");
       end
       else begin
          if (y !== 1'b0) $fatal("Assertion failed for Test Case: b <= 2'b0");
       end
    end
*/

endmodule: tb

/*
------------------------------------------------------------------------------
Version History:
------------------------------------------------------------------------------
 2024/02/14 TI: initial creation
 2024/02/25 JI: adapted to Verilation limitations in handling HiZ and INOUT ports
 2024/03/28 JI: added DAC
 2024/10/31 SS: Made compatible with current dut ports and commented out
                non-compiling test components
*/
