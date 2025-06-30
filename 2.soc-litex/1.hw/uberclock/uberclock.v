// ============================================================================
// uberclock.v
// ============================================================================
// Top-level wrapper that lets a CPU choose between multiple signal paths:
//   000: ADC → DAC
//   001: CORDIC → DAC
//   010: Full chain (ADC → CORDIC → filters → CORDIC → DAC)
//   011: ADC → CORDIC → DAC (no filters)
//   100: Generate with CORDIC only (no ADC)
// ----------------------------------------------------------------------------

`timescale 1ns / 1ps
module uberclock (
    input           sys_clk,
    input           rst_n,
    //------------------------------------------------------------------------------
    // CPU-driven path selector: 3'b000 .. 3'b100
    input   [2:0]   mode_sel,
    //------------------------------------------------------------------------------
    // ADC interface
    output          adc_clk_ch0,
    output          adc_clk_ch1,
    input   [11:0]  adc_data_ch0,
    input   [11:0]  adc_data_ch1,
    //------------------------------------------------------------------------------
    // Phase increment for the CORDIC(s)
    input   [18:0]  phase_inc,
    //------------------------------------------------------------------------------
    // DAC interface
    output          da1_clk,
    output          da1_wrt,
    output  [13:0]  da1_data,
    output          da2_clk,
    output          da2_wrt,
    output  [13:0]  da2_data
);

    //==========================================================================
    // 1) Sample the ADC and expose 12-bit data
    //==========================================================================
    wire [11:0] ad0, ad1;
    adc u_adc (
      .sys_clk      (sys_clk),
      .rst_n        (rst_n),
       // Raw DDR-pinned inputs from the board
      .adc_data_ch0 (adc_data_ch0),
      .adc_data_ch1 (adc_data_ch1),
       // DDR clocks to drive each AD9238 chip
      .adc_clk_ch0  (adc_clk_ch0),
      .adc_clk_ch1  (adc_clk_ch1),
       // 12-bit, single-clock-domain outputs (rising-edge captures)
      .ad_data_ch0  (ad0),
      .ad_data_ch1  (ad1)
    );

    //==========================================================================
    // 2) Down-convert with CORDIC
    //==========================================================================
    reg  [18:0] phase_acc;
    always @(posedge sys_clk or negedge rst_n) begin
      if (!rst_n)      phase_acc <= 0;
      else             phase_acc <= phase_acc + phase_inc;
    end

    wire signed [11:0] cordic_x, cordic_y;
    cordic #(
      .IW(12), .OW(12), .NSTAGES(15), .WW(15), .PW(19)
    ) u_cordic_down (
      .i_clk   (sys_clk),
      .i_reset (rst_n),
      .i_ce    (1'b1),
      .i_xval  (12'd0),        // for down-conversion your carrier is on Y
      .i_yval  (ad0),
      .i_phase (phase_acc),
      .i_aux   (1'b1),
      .o_xval  (cordic_x),
      .o_yval  (cordic_y),
      .o_aux   ()
    );

    //==========================================================================
    // 3) Optional filters (downsampler / upsampler) and up-conversion
    //    We assume you have submodules downsamplerFilter, upsamplerFilter and
    //    a second cordic16 for the up-conversion path just as in your dsp core.
    //==========================================================================
    wire signed [15:0] ds_y, us_y;
    wire               ds_ce, us_ce;

    downsamplerFilter u_ds (
      .clk        (sys_clk),
      .clk_enable (1'b1),
      .reset      (rst_n),
      .filter_in  (cordic_y),
      .filter_out (ds_y),
      .ce_out     (ds_ce)
    );

    upsamplerFilter u_us (
      .clk        (sys_clk),
      .clk_enable (1'b1),
      .reset      (rst_n),
      .filter_in  (ds_y),
      .filter_out (us_y),
      .ce_out     (us_ce)
    );

    // up-conversion phase (inverse phase accumulator)
    wire [22:0] phase_inv = (1<<23) - {phase_acc,4'd0};
    wire signed [15:0] xc, yc;
    cordic16 #(
      .IW(16), .OW(16), .NSTAGES(19), .WW(19), .PW(23)
    ) u_cordic_up (
      .i_clk   (sys_clk),
      .i_reset (rst_n),
      .i_ce    (1'b1),
      .i_xval  (us_y),       // note: swapped inputs to re-mix
      .i_yval  (us_y),
      .i_phase (phase_inv),
      .i_aux   (us_ce),
      .o_xval  (xc),
      .o_yval  (yc),
      .o_aux   ()
    );

    //==========================================================================
    // 4) Build all candidate 14-bit DAC words
    //    — ADC→DAC (direct, zero-extend):
    //    — CORDIC→DAC (down-cordic outputs)
    //    — Filtered chain: up-converted outputs xc,yc
    //    — ADC→CORDIC→DAC: feed cordic_x,cordic_y directly into DAC
    //    — Pure CORDIC generator: run u_cordic_up alone (e.g. inputs = 0)
    //==========================================================================
    wire [13:0] dac_adc0  = {ad0,    2'b00};
    wire [13:0] dac_adc1  = {ad1,    2'b00};
    wire [13:0] dac_cd0   = {{cordic_x[11]}, cordic_x[10:0], 1'b0}; // sign-extend
    wire [13:0] dac_cd1   = {{cordic_y[11]}, cordic_y[10:0], 1'b0};
    wire [13:0] dac_f0    = xc[15:2];
    wire [13:0] dac_f1    = yc[15:2];

    //==========================================================================
    // 5) Mux selector
    //==========================================================================
    reg [13:0] sel0, sel1;
    always @(*) begin
      case(mode_sel)
        3'b000: begin                // ADC → DAC
          sel0 = dac_adc0;
          sel1 = dac_adc1;
        end
        3'b001: begin                // CORDIC → DAC
          sel0 = dac_cd0;
          sel1 = dac_cd1;
        end
        3'b010: begin                // Full chain (filters & up-conv)
          sel0 = dac_f0;
          sel1 = dac_f1;
        end
        3'b011: begin                // ADC → CORDIC → DAC (no filters)
          sel0 = dac_cd0;
          sel1 = dac_cd1;
        end
        3'b100: begin                // Pure CORDIC generator (up only)
          sel0 = dac_f0;
          sel1 = dac_f1;
        end
        default: begin
          sel0 = dac_adc0;
          sel1 = dac_adc1;
        end
      endcase
    end

    //==========================================================================
    // 6) Hook the chosen words into the DDR-DAC core
    //==========================================================================
    dac u_dac (
      .sys_clk  (sys_clk),
      .rst_n    (rst_n),
      .data1    (sel0),
      .data2    (sel1),
      .da1_clk  (da1_clk),
      .da1_wrt  (da1_wrt),
      .da1_data (da1_data),
      .da2_clk  (da2_clk),
      .da2_wrt  (da2_wrt),
      .da2_data (da2_data)
    );

endmodule
