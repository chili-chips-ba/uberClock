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
//   Clock and reset generation
//==========================================================================

module clk_rst_gen
(
   input  clk_p,
   input  clk_n,
   input  rst_n,
   output sys_clk,
   output sys_rst,
   output sys_rst_n,
   output adc_clk,
   output dac_clk
);

//==========================================================================
// Generate single end clock from differential input clock
//==========================================================================
   wire clk;

   IBUFGDS sys_clk_ibufgds
   (
      .O(clk),
      .I(clk_p),
      .IB(clk_n)
   );

//==========================================================================
// PLL for system clock domain
//==========================================================================
   wire sys_pll_locked;
   wire sys_pll_clk;
   wire sys_reset;

   fpga_pll_80M u_sys_pll (
      .clk(clk),
      .rst_n(rst_n),
      .sys_pll_clk(sys_pll_clk),
      .sys_pll_locked(sys_pll_locked)
   );

   (* srl_style = "register" *)
   logic [3:0] sync_reg;
   
   always @(posedge sys_pll_clk or negedge sys_pll_locked) begin
       if (sys_pll_locked == 1'b0) begin
           sync_reg <= '1;
       end else begin
           sync_reg <= {sync_reg[2:0], 1'b0};
       end
   end

   assign sys_rst = sync_reg[3];
   assign sys_rst_n = ~sync_reg[3];
   assign sys_clk = sys_pll_clk;
   
   
    /*************************************************************************
   Generate the clock required for the AD data processing
   ***************************************************************************/
   adc_pll adc_pll_m0 (
      .clk_out1 (adc_clk),
      .reset    (1'b0),
      .locked   (),
      .clk_in1  (clk)
   );
   

   /*************************************************************************
   Generate the clock required for the DA data 
   **************************************************************************/
   dac_pll dac_pll_0 (
      .clk_out1(dac_clk),
      .reset(1'b0),
      .locked(),
      .clk_in1(clk)
   );

endmodule: clk_rst_gen
