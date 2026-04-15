`timescale 1 ns / 1 ns
`default_nettype none
module cic_comp_down_opt
               (
                clk,
                clk_enable,
                reset,
                filter_in,
                filter_out,
                ce_out
                );

  input   clk; 
  input   clk_enable; 
  input   reset; 
  input   signed [15:0] filter_in; //sfix12_En11
  output  signed [15:0] filter_out; //sfix16_En15
  output  ce_out; 

////////////////////////////////////////////////////////////////
//Module Architecture: cic_comp_down
////////////////////////////////////////////////////////////////
  // Local Functions
  // Type Definitions
  // Constants
  parameter signed [14:0] coeffphase1_1 = 15'b000000000100100; //sfix15_En15
  parameter signed [14:0] coeffphase1_2 = 15'b000000000001001; //sfix15_En15
  parameter signed [14:0] coeffphase1_3 = 15'b111111000100100; //sfix15_En15
  parameter signed [14:0] coeffphase1_4 = 15'b111111010011100; //sfix15_En15
  parameter signed [14:0] coeffphase1_5 = 15'b000001110100000; //sfix15_En15
  parameter signed [14:0] coeffphase1_6 = 15'b111111110000111; //sfix15_En15
  parameter signed [14:0] coeffphase1_7 = 15'b111011101100010; //sfix15_En15
  parameter signed [14:0] coeffphase1_8 = 15'b000101010001000; //sfix15_En15
  parameter signed [14:0] coeffphase1_9 = 15'b010011100001000; //sfix15_En15
  parameter signed [14:0] coeffphase1_10 = 15'b001101101101100; //sfix15_En15
  parameter signed [14:0] coeffphase1_11 = 15'b111110011000101; //sfix15_En15
  parameter signed [14:0] coeffphase1_12 = 15'b111100111011100; //sfix15_En15
  parameter signed [14:0] coeffphase1_13 = 15'b000001101111000; //sfix15_En15
  parameter signed [14:0] coeffphase1_14 = 15'b000000100101100; //sfix15_En15
  parameter signed [14:0] coeffphase1_15 = 15'b111110110011000; //sfix15_En15
  parameter signed [14:0] coeffphase1_16 = 15'b111111101000001; //sfix15_En15
  parameter signed [14:0] coeffphase1_17 = 15'b000000000111110; //sfix15_En15

  // Signals
  reg  [1:0] ring_count; // ufix2
  wire phase_0; // boolean
  wire phase_1; // boolean
  reg  ce_out_reg; // boolean
  reg  signed [15:0] input_register; // sfix12_En11
  reg  signed [15:0] input_pipeline_phase0 [0:15] ; // sfix12_En11
  reg  signed [15:0] input_pipeline_phase1 [0:16] ; // sfix12_En11
  wire signed [30:0] product_phase0_1; // sfix27_En26
  wire signed [30:0] product_phase0_2; // sfix27_En26
  wire signed [30:0] product_phase0_3; // sfix27_En26
  wire signed [30:0] product_phase0_4; // sfix27_En26
  wire signed [30:0] product_phase0_5; // sfix27_En26
  wire signed [30:0] product_phase0_6; // sfix27_En26
  wire signed [30:0] product_phase0_7; // sfix27_En26
  wire signed [30:0] product_phase0_8; // sfix27_En26
  wire signed [30:0] product_phase0_9; // sfix27_En26
  wire signed [30:0] product_phase0_10; // sfix27_En26
  wire signed [30:0] product_phase0_11; // sfix27_En26
  wire signed [30:0] product_phase0_12; // sfix27_En26
  wire signed [30:0] product_phase0_13; // sfix27_En26
  wire signed [30:0] product_phase0_14; // sfix27_En26
  wire signed [30:0] product_phase0_15; // sfix27_En26
  wire signed [30:0] product_phase0_16; // sfix27_En26
  wire signed [30:0] product_phase0_17; // sfix27_En26
  wire signed [30:0] product_phase1_1; // sfix27_En26
  wire signed [30:0] product_phase1_2; // sfix27_En26
  wire signed [30:0] product_phase1_3; // sfix27_En26
  wire signed [30:0] product_phase1_4; // sfix27_En26
  wire signed [30:0] product_phase1_5; // sfix27_En26
  wire signed [30:0] product_phase1_6; // sfix27_En26
  wire signed [30:0] product_phase1_7; // sfix27_En26
  wire signed [30:0] product_phase1_8; // sfix27_En26
  wire signed [30:0] product_phase1_9; // sfix27_En26
  wire signed [30:0] product_phase1_10; // sfix27_En26
  wire signed [30:0] product_phase1_11; // sfix27_En26
  wire signed [30:0] product_phase1_12; // sfix27_En26
  wire signed [30:0] product_phase1_13; // sfix27_En26
  wire signed [30:0] product_phase1_14; // sfix27_En26
  wire signed [30:0] product_phase1_15; // sfix27_En26
  wire signed [30:0] product_phase1_16; // sfix27_En26
  wire signed [30:0] product_phase1_17; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_1; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_2; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_3; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_4; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_5; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_6; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_7; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_8; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_9; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_10; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_11; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_12; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_13; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_14; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_15; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_16; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase0_17; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_1; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_2; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_3; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_4; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_5; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_6; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_7; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_8; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_9; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_10; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_11; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_12; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_13; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_14; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_15; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_16; // sfix27_En26
  reg  signed [30:0] product_pipeline_phase1_17; // sfix27_En26
//  wire signed [31:0] sumvector1 [0:16] ; // sfix32_En26
//  wire signed [26:0] add_signext; // sfix27_En26
//  wire signed [26:0] add_signext_1; // sfix27_En26
//  wire signed [27:0] add_temp; // sfix28_En26
//  wire signed [26:0] add_signext_2; // sfix27_En26
//  wire signed [26:0] add_signext_3; // sfix27_En26
//  wire signed [27:0] add_temp_1; // sfix28_En26
//  wire signed [26:0] add_signext_4; // sfix27_En26
//  wire signed [26:0] add_signext_5; // sfix27_En26
//  wire signed [27:0] add_temp_2; // sfix28_En26
//  wire signed [26:0] add_signext_6; // sfix27_En26
//  wire signed [26:0] add_signext_7; // sfix27_En26
//  wire signed [27:0] add_temp_3; // sfix28_En26
//  wire signed [26:0] add_signext_8; // sfix27_En26
//  wire signed [26:0] add_signext_9; // sfix27_En26
//  wire signed [27:0] add_temp_4; // sfix28_En26
//  wire signed [26:0] add_signext_10; // sfix27_En26
//  wire signed [26:0] add_signext_11; // sfix27_En26
//  wire signed [27:0] add_temp_5; // sfix28_En26
//  wire signed [26:0] add_signext_12; // sfix27_En26
//  wire signed [26:0] add_signext_13; // sfix27_En26
//  wire signed [27:0] add_temp_6; // sfix28_En26
//  wire signed [26:0] add_signext_14; // sfix27_En26
//  wire signed [26:0] add_signext_15; // sfix27_En26
//  wire signed [27:0] add_temp_7; // sfix28_En26
//  wire signed [26:0] add_signext_16; // sfix27_En26
//  wire signed [26:0] add_signext_17; // sfix27_En26
//  wire signed [27:0] add_temp_8; // sfix28_En26
//  wire signed [26:0] add_signext_18; // sfix27_En26
//  wire signed [26:0] add_signext_19; // sfix27_En26
//  wire signed [27:0] add_temp_9; // sfix28_En26
//  wire signed [26:0] add_signext_20; // sfix27_En26
//  wire signed [26:0] add_signext_21; // sfix27_En26
//  wire signed [27:0] add_temp_10; // sfix28_En26
//  wire signed [26:0] add_signext_22; // sfix27_En26
//  wire signed [26:0] add_signext_23; // sfix27_En26
//  wire signed [27:0] add_temp_11; // sfix28_En26
//  wire signed [26:0] add_signext_24; // sfix27_En26
//  wire signed [26:0] add_signext_25; // sfix27_En26
//  wire signed [27:0] add_temp_12; // sfix28_En26
//  wire signed [26:0] add_signext_26; // sfix27_En26
//  wire signed [26:0] add_signext_27; // sfix27_En26
//  wire signed [27:0] add_temp_13; // sfix28_En26
//  wire signed [26:0] add_signext_28; // sfix27_En26
//  wire signed [26:0] add_signext_29; // sfix27_En26
//  wire signed [27:0] add_temp_14; // sfix28_En26
//  wire signed [26:0] add_signext_30; // sfix27_En26
//  wire signed [26:0] add_signext_31; // sfix27_En26
//  wire signed [27:0] add_temp_15; // sfix28_En26
//  wire signed [26:0] add_signext_32; // sfix27_En26
//  wire signed [26:0] add_signext_33; // sfix27_En26
//  wire signed [27:0] add_temp_16; // sfix28_En26
//  reg  signed [31:0] sumdelay_pipeline1 [0:16] ; // sfix32_En26
//  wire signed [31:0] sumvector2 [0:8] ; // sfix32_En26
//  wire signed [31:0] add_signext_34; // sfix32_En26
//  wire signed [31:0] add_signext_35; // sfix32_En26
//  wire signed [32:0] add_temp_17; // sfix33_En26
//  wire signed [31:0] add_signext_36; // sfix32_En26
//  wire signed [31:0] add_signext_37; // sfix32_En26
//  wire signed [32:0] add_temp_18; // sfix33_En26
//  wire signed [31:0] add_signext_38; // sfix32_En26
//  wire signed [31:0] add_signext_39; // sfix32_En26
//  wire signed [32:0] add_temp_19; // sfix33_En26
//  wire signed [31:0] add_signext_40; // sfix32_En26
//  wire signed [31:0] add_signext_41; // sfix32_En26
//  wire signed [32:0] add_temp_20; // sfix33_En26
//  wire signed [31:0] add_signext_42; // sfix32_En26
//  wire signed [31:0] add_signext_43; // sfix32_En26
//  wire signed [32:0] add_temp_21; // sfix33_En26
//  wire signed [31:0] add_signext_44; // sfix32_En26
//  wire signed [31:0] add_signext_45; // sfix32_En26
//  wire signed [32:0] add_temp_22; // sfix33_En26
//  wire signed [31:0] add_signext_46; // sfix32_En26
//  wire signed [31:0] add_signext_47; // sfix32_En26
//  wire signed [32:0] add_temp_23; // sfix33_En26
//  wire signed [31:0] add_signext_48; // sfix32_En26
//  wire signed [31:0] add_signext_49; // sfix32_En26
//  wire signed [32:0] add_temp_24; // sfix33_En26
//  reg  signed [31:0] sumdelay_pipeline2 [0:8] ; // sfix32_En26
//  wire signed [31:0] sumvector3 [0:4] ; // sfix32_En26
//  wire signed [31:0] add_signext_50; // sfix32_En26
//  wire signed [31:0] add_signext_51; // sfix32_En26
//  wire signed [32:0] add_temp_25; // sfix33_En26
//  wire signed [31:0] add_signext_52; // sfix32_En26
//  wire signed [31:0] add_signext_53; // sfix32_En26
//  wire signed [32:0] add_temp_26; // sfix33_En26
//  wire signed [31:0] add_signext_54; // sfix32_En26
//  wire signed [31:0] add_signext_55; // sfix32_En26
//  wire signed [32:0] add_temp_27; // sfix33_En26
//  wire signed [31:0] add_signext_56; // sfix32_En26
//  wire signed [31:0] add_signext_57; // sfix32_En26
//  wire signed [32:0] add_temp_28; // sfix33_En26
//  reg  signed [31:0] sumdelay_pipeline3 [0:4] ; // sfix32_En26
//  wire signed [31:0] sumvector4 [0:2] ; // sfix32_En26
//  wire signed [31:0] add_signext_58; // sfix32_En26
//  wire signed [31:0] add_signext_59; // sfix32_En26
//  wire signed [32:0] add_temp_29; // sfix33_En26
//  wire signed [31:0] add_signext_60; // sfix32_En26
//  wire signed [31:0] add_signext_61; // sfix32_En26
//  wire signed [32:0] add_temp_30; // sfix33_En26
//  reg  signed [31:0] sumdelay_pipeline4 [0:2] ; // sfix32_En26
//  wire signed [31:0] sumvector5 [0:1] ; // sfix32_En26
//  wire signed [31:0] add_signext_62; // sfix32_En26
//  wire signed [31:0] add_signext_63; // sfix32_En26
//  wire signed [32:0] add_temp_31; // sfix33_En26
//  reg  signed [31:0] sumdelay_pipeline5 [0:1] ; // sfix32_En26
//  wire signed [31:0] sum6; // sfix32_En26
//  wire signed [31:0] add_signext_64; // sfix32_En26
//  wire signed [31:0] add_signext_65; // sfix32_En26
//  wire signed [32:0] add_temp_32; // sfix33_En26
//  wire signed [15:0] output_typeconvert; // sfix16_En15

  wire signed [35:0] sumvector1 [0:16] ; // sfix36_En29
  wire signed [30:0] add_signext; // sfix31_En29
  wire signed [30:0] add_signext_1; // sfix31_En29
  wire signed [31:0] add_temp; // sfix32_En29
  wire signed [30:0] add_signext_2; // sfix31_En29
  wire signed [30:0] add_signext_3; // sfix31_En29
  wire signed [31:0] add_temp_1; // sfix32_En29
  wire signed [30:0] add_signext_4; // sfix31_En29
  wire signed [30:0] add_signext_5; // sfix31_En29
  wire signed [31:0] add_temp_2; // sfix32_En29
  wire signed [30:0] add_signext_6; // sfix31_En29
  wire signed [30:0] add_signext_7; // sfix31_En29
  wire signed [31:0] add_temp_3; // sfix32_En29
  wire signed [30:0] add_signext_8; // sfix31_En29
  wire signed [30:0] add_signext_9; // sfix31_En29
  wire signed [31:0] add_temp_4; // sfix32_En29
  wire signed [30:0] add_signext_10; // sfix31_En29
  wire signed [30:0] add_signext_11; // sfix31_En29
  wire signed [31:0] add_temp_5; // sfix32_En29
  wire signed [30:0] add_signext_12; // sfix31_En29
  wire signed [30:0] add_signext_13; // sfix31_En29
  wire signed [31:0] add_temp_6; // sfix32_En29
  wire signed [30:0] add_signext_14; // sfix31_En29
  wire signed [30:0] add_signext_15; // sfix31_En29
  wire signed [31:0] add_temp_7; // sfix32_En29
  wire signed [30:0] add_signext_16; // sfix31_En29
  wire signed [30:0] add_signext_17; // sfix31_En29
  wire signed [31:0] add_temp_8; // sfix32_En29
  wire signed [30:0] add_signext_18; // sfix31_En29
  wire signed [30:0] add_signext_19; // sfix31_En29
  wire signed [31:0] add_temp_9; // sfix32_En29
  wire signed [30:0] add_signext_20; // sfix31_En29
  wire signed [30:0] add_signext_21; // sfix31_En29
  wire signed [31:0] add_temp_10; // sfix32_En29
  wire signed [30:0] add_signext_22; // sfix31_En29
  wire signed [30:0] add_signext_23; // sfix31_En29
  wire signed [31:0] add_temp_11; // sfix32_En29
  wire signed [30:0] add_signext_24; // sfix31_En29
  wire signed [30:0] add_signext_25; // sfix31_En29
  wire signed [31:0] add_temp_12; // sfix32_En29
  wire signed [30:0] add_signext_26; // sfix31_En29
  wire signed [30:0] add_signext_27; // sfix31_En29
  wire signed [31:0] add_temp_13; // sfix32_En29
  wire signed [30:0] add_signext_28; // sfix31_En29
  wire signed [30:0] add_signext_29; // sfix31_En29
  wire signed [31:0] add_temp_14; // sfix32_En29
  wire signed [30:0] add_signext_30; // sfix31_En29
  wire signed [30:0] add_signext_31; // sfix31_En29
  wire signed [31:0] add_temp_15; // sfix32_En29
  wire signed [30:0] add_signext_32; // sfix31_En29
  wire signed [30:0] add_signext_33; // sfix31_En29
  wire signed [31:0] add_temp_16; // sfix32_En29
  reg  signed [35:0] sumdelay_pipeline1 [0:16] ; // sfix36_En29
  wire signed [35:0] sumvector2 [0:8] ; // sfix36_En29
  wire signed [35:0] add_signext_34; // sfix36_En29
  wire signed [35:0] add_signext_35; // sfix36_En29
  wire signed [36:0] add_temp_17; // sfix37_En29
  wire signed [35:0] add_signext_36; // sfix36_En29
  wire signed [35:0] add_signext_37; // sfix36_En29
  wire signed [36:0] add_temp_18; // sfix37_En29
  wire signed [35:0] add_signext_38; // sfix36_En29
  wire signed [35:0] add_signext_39; // sfix36_En29
  wire signed [36:0] add_temp_19; // sfix37_En29
  wire signed [35:0] add_signext_40; // sfix36_En29
  wire signed [35:0] add_signext_41; // sfix36_En29
  wire signed [36:0] add_temp_20; // sfix37_En29
  wire signed [35:0] add_signext_42; // sfix36_En29
  wire signed [35:0] add_signext_43; // sfix36_En29
  wire signed [36:0] add_temp_21; // sfix37_En29
  wire signed [35:0] add_signext_44; // sfix36_En29
  wire signed [35:0] add_signext_45; // sfix36_En29
  wire signed [36:0] add_temp_22; // sfix37_En29
  wire signed [35:0] add_signext_46; // sfix36_En29
  wire signed [35:0] add_signext_47; // sfix36_En29
  wire signed [36:0] add_temp_23; // sfix37_En29
  wire signed [35:0] add_signext_48; // sfix36_En29
  wire signed [35:0] add_signext_49; // sfix36_En29
  wire signed [36:0] add_temp_24; // sfix37_En29
  reg  signed [35:0] sumdelay_pipeline2 [0:8] ; // sfix36_En29
  wire signed [35:0] sumvector3 [0:4] ; // sfix36_En29
  wire signed [35:0] add_signext_50; // sfix36_En29
  wire signed [35:0] add_signext_51; // sfix36_En29
  wire signed [36:0] add_temp_25; // sfix37_En29
  wire signed [35:0] add_signext_52; // sfix36_En29
  wire signed [35:0] add_signext_53; // sfix36_En29
  wire signed [36:0] add_temp_26; // sfix37_En29
  wire signed [35:0] add_signext_54; // sfix36_En29
  wire signed [35:0] add_signext_55; // sfix36_En29
  wire signed [36:0] add_temp_27; // sfix37_En29
  wire signed [35:0] add_signext_56; // sfix36_En29
  wire signed [35:0] add_signext_57; // sfix36_En29
  wire signed [36:0] add_temp_28; // sfix37_En29
  reg  signed [35:0] sumdelay_pipeline3 [0:4] ; // sfix36_En29
  wire signed [35:0] sumvector4 [0:2] ; // sfix36_En29
  wire signed [35:0] add_signext_58; // sfix36_En29
  wire signed [35:0] add_signext_59; // sfix36_En29
  wire signed [36:0] add_temp_29; // sfix37_En29
  wire signed [35:0] add_signext_60; // sfix36_En29
  wire signed [35:0] add_signext_61; // sfix36_En29
  wire signed [36:0] add_temp_30; // sfix37_En29
  reg  signed [35:0] sumdelay_pipeline4 [0:2] ; // sfix36_En29
  wire signed [35:0] sumvector5 [0:1] ; // sfix36_En29
  wire signed [35:0] add_signext_62; // sfix36_En29
  wire signed [35:0] add_signext_63; // sfix36_En29
  wire signed [36:0] add_temp_31; // sfix37_En29
  reg  signed [35:0] sumdelay_pipeline5 [0:1] ; // sfix36_En29
  wire signed [35:0] sum6; // sfix36_En29
  wire signed [35:0] add_signext_64; // sfix36_En29
  wire signed [35:0] add_signext_65; // sfix36_En29
  wire signed [36:0] add_temp_32; // sfix37_En29
  wire signed [15:0] output_typeconvert; // sfix16_En14
  reg  ce_delayline1; // boolean
  reg  ce_delayline2; // boolean
  reg  ce_delayline3; // boolean
  reg  ce_delayline4; // boolean
  reg  ce_delayline5; // boolean
  reg  ce_delayline6; // boolean
  reg  ce_delayline7; // boolean
  reg  ce_delayline8; // boolean
  reg  ce_delayline9; // boolean
  reg  ce_delayline10; // boolean
  reg  ce_delayline11; // boolean
  reg  ce_delayline12; // boolean
  reg  ce_delayline13; // boolean
  reg  ce_delayline14; // boolean
  wire ce_gated; // boolean
  reg  signed [15:0] output_register; // sfix16_En15

  // Block Statements
  always @ (posedge clk or posedge reset)
    begin: ce_output
      if (reset == 1'b1) begin
        ring_count <= 1;
      end
      else begin
                if (clk_enable == 1'b1) begin
        ring_count <= {ring_count[0], ring_count[1]};
              end
            end
    end // ce_output

  assign  phase_0 = ring_count[0]  && clk_enable;

  assign  phase_1 = ring_count[1]  && clk_enable;

  //   ------------------ CE Output Register ------------------

  always @ (posedge clk or posedge reset)
    begin: ce_output_register
      if (reset == 1'b1) begin
        ce_out_reg <= 1'b0;
      end
      else begin
          ce_out_reg <= phase_1;
      end
    end // ce_output_register

  always @ (posedge clk or posedge reset)
    begin: input_reg_process
      if (reset == 1'b1) begin
        input_register <= 0;
      end
      else begin
        if (clk_enable == 1'b1) begin
          input_register <= filter_in;
        end
      end
    end // input_reg_process

  always @( posedge clk or posedge reset)
    begin: Delay_Pipeline_Phase0_process
      if (reset == 1'b1) begin
        input_pipeline_phase0[0] <= 0;
        input_pipeline_phase0[1] <= 0;
        input_pipeline_phase0[2] <= 0;
        input_pipeline_phase0[3] <= 0;
        input_pipeline_phase0[4] <= 0;
        input_pipeline_phase0[5] <= 0;
        input_pipeline_phase0[6] <= 0;
        input_pipeline_phase0[7] <= 0;
        input_pipeline_phase0[8] <= 0;
        input_pipeline_phase0[9] <= 0;
        input_pipeline_phase0[10] <= 0;
        input_pipeline_phase0[11] <= 0;
        input_pipeline_phase0[12] <= 0;
        input_pipeline_phase0[13] <= 0;
        input_pipeline_phase0[14] <= 0;
        input_pipeline_phase0[15] <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          input_pipeline_phase0[0] <= input_register;
          input_pipeline_phase0[1] <= input_pipeline_phase0[0];
          input_pipeline_phase0[2] <= input_pipeline_phase0[1];
          input_pipeline_phase0[3] <= input_pipeline_phase0[2];
          input_pipeline_phase0[4] <= input_pipeline_phase0[3];
          input_pipeline_phase0[5] <= input_pipeline_phase0[4];
          input_pipeline_phase0[6] <= input_pipeline_phase0[5];
          input_pipeline_phase0[7] <= input_pipeline_phase0[6];
          input_pipeline_phase0[8] <= input_pipeline_phase0[7];
          input_pipeline_phase0[9] <= input_pipeline_phase0[8];
          input_pipeline_phase0[10] <= input_pipeline_phase0[9];
          input_pipeline_phase0[11] <= input_pipeline_phase0[10];
          input_pipeline_phase0[12] <= input_pipeline_phase0[11];
          input_pipeline_phase0[13] <= input_pipeline_phase0[12];
          input_pipeline_phase0[14] <= input_pipeline_phase0[13];
          input_pipeline_phase0[15] <= input_pipeline_phase0[14];
        end
      end
    end // Delay_Pipeline_Phase0_process


  always @( posedge clk or posedge reset)
    begin: Delay_Pipeline_Phase1_process
      if (reset == 1'b1) begin
        input_pipeline_phase1[0] <= 0;
        input_pipeline_phase1[1] <= 0;
        input_pipeline_phase1[2] <= 0;
        input_pipeline_phase1[3] <= 0;
        input_pipeline_phase1[4] <= 0;
        input_pipeline_phase1[5] <= 0;
        input_pipeline_phase1[6] <= 0;
        input_pipeline_phase1[7] <= 0;
        input_pipeline_phase1[8] <= 0;
        input_pipeline_phase1[9] <= 0;
        input_pipeline_phase1[10] <= 0;
        input_pipeline_phase1[11] <= 0;
        input_pipeline_phase1[12] <= 0;
        input_pipeline_phase1[13] <= 0;
        input_pipeline_phase1[14] <= 0;
        input_pipeline_phase1[15] <= 0;
        input_pipeline_phase1[16] <= 0;
      end
      else begin
        if (phase_0 == 1'b1) begin
          input_pipeline_phase1[0] <= input_register;
          input_pipeline_phase1[1] <= input_pipeline_phase1[0];
          input_pipeline_phase1[2] <= input_pipeline_phase1[1];
          input_pipeline_phase1[3] <= input_pipeline_phase1[2];
          input_pipeline_phase1[4] <= input_pipeline_phase1[3];
          input_pipeline_phase1[5] <= input_pipeline_phase1[4];
          input_pipeline_phase1[6] <= input_pipeline_phase1[5];
          input_pipeline_phase1[7] <= input_pipeline_phase1[6];
          input_pipeline_phase1[8] <= input_pipeline_phase1[7];
          input_pipeline_phase1[9] <= input_pipeline_phase1[8];
          input_pipeline_phase1[10] <= input_pipeline_phase1[9];
          input_pipeline_phase1[11] <= input_pipeline_phase1[10];
          input_pipeline_phase1[12] <= input_pipeline_phase1[11];
          input_pipeline_phase1[13] <= input_pipeline_phase1[12];
          input_pipeline_phase1[14] <= input_pipeline_phase1[13];
          input_pipeline_phase1[15] <= input_pipeline_phase1[14];
          input_pipeline_phase1[16] <= input_pipeline_phase1[15];
        end
      end
    end // Delay_Pipeline_Phase1_process


  assign product_phase0_1 = input_register * coeffphase1_1;

  assign product_phase0_2 = input_pipeline_phase0[0] * coeffphase1_2;

  assign product_phase0_3 = input_pipeline_phase0[1] * coeffphase1_3;

  assign product_phase0_4 = input_pipeline_phase0[2] * coeffphase1_4;

  assign product_phase0_5 = input_pipeline_phase0[3] * coeffphase1_5;

  assign product_phase0_6 = input_pipeline_phase0[4] * coeffphase1_6;

  assign product_phase0_7 = input_pipeline_phase0[5] * coeffphase1_7;

  assign product_phase0_8 = input_pipeline_phase0[6] * coeffphase1_8;

  assign product_phase0_9 = input_pipeline_phase0[7] * coeffphase1_9;

  assign product_phase0_10 = input_pipeline_phase0[8] * coeffphase1_10;

  assign product_phase0_11 = input_pipeline_phase0[9] * coeffphase1_11;

  assign product_phase0_12 = input_pipeline_phase0[10] * coeffphase1_12;

  assign product_phase0_13 = input_pipeline_phase0[11] * coeffphase1_13;

  assign product_phase0_14 = input_pipeline_phase0[12] * coeffphase1_14;

  assign product_phase0_15 = input_pipeline_phase0[13] * coeffphase1_15;

  assign product_phase0_16 = input_pipeline_phase0[14] * coeffphase1_16;

  assign product_phase0_17 = input_pipeline_phase0[15] * coeffphase1_17;

  assign product_phase1_1 = input_pipeline_phase1[0] * coeffphase1_17;

  assign product_phase1_2 = input_pipeline_phase1[1] * coeffphase1_16;

  assign product_phase1_3 = input_pipeline_phase1[2] * coeffphase1_15;

  assign product_phase1_4 = input_pipeline_phase1[3] * coeffphase1_14;

  assign product_phase1_5 = input_pipeline_phase1[4] * coeffphase1_13;

  assign product_phase1_6 = input_pipeline_phase1[5] * coeffphase1_12;

  assign product_phase1_7 = input_pipeline_phase1[6] * coeffphase1_11;

  assign product_phase1_8 = input_pipeline_phase1[7] * coeffphase1_10;

  assign product_phase1_9 = input_pipeline_phase1[8] * coeffphase1_9;

  assign product_phase1_10 = input_pipeline_phase1[9] * coeffphase1_8;

  assign product_phase1_11 = input_pipeline_phase1[10] * coeffphase1_7;

  assign product_phase1_12 = input_pipeline_phase1[11] * coeffphase1_6;

  assign product_phase1_13 = input_pipeline_phase1[12] * coeffphase1_5;

  assign product_phase1_14 = input_pipeline_phase1[13] * coeffphase1_4;

  assign product_phase1_15 = input_pipeline_phase1[14] * coeffphase1_3;

  assign product_phase1_16 = input_pipeline_phase1[15] * coeffphase1_2;

  assign product_phase1_17 = input_pipeline_phase1[16] * coeffphase1_1;

  always @ (posedge clk or posedge reset)
    begin: product_pipeline_process1
      if (reset == 1'b1) begin
        product_pipeline_phase0_1 <= 0;
        product_pipeline_phase1_1 <= 0;
        product_pipeline_phase0_2 <= 0;
        product_pipeline_phase1_2 <= 0;
        product_pipeline_phase0_3 <= 0;
        product_pipeline_phase1_3 <= 0;
        product_pipeline_phase0_4 <= 0;
        product_pipeline_phase1_4 <= 0;
        product_pipeline_phase0_5 <= 0;
        product_pipeline_phase1_5 <= 0;
        product_pipeline_phase0_6 <= 0;
        product_pipeline_phase1_6 <= 0;
        product_pipeline_phase0_7 <= 0;
        product_pipeline_phase1_7 <= 0;
        product_pipeline_phase0_8 <= 0;
        product_pipeline_phase1_8 <= 0;
        product_pipeline_phase0_9 <= 0;
        product_pipeline_phase1_9 <= 0;
        product_pipeline_phase0_10 <= 0;
        product_pipeline_phase1_10 <= 0;
        product_pipeline_phase0_11 <= 0;
        product_pipeline_phase1_11 <= 0;
        product_pipeline_phase0_12 <= 0;
        product_pipeline_phase1_12 <= 0;
        product_pipeline_phase0_13 <= 0;
        product_pipeline_phase1_13 <= 0;
        product_pipeline_phase0_14 <= 0;
        product_pipeline_phase1_14 <= 0;
        product_pipeline_phase0_15 <= 0;
        product_pipeline_phase1_15 <= 0;
        product_pipeline_phase0_16 <= 0;
        product_pipeline_phase1_16 <= 0;
        product_pipeline_phase0_17 <= 0;
        product_pipeline_phase1_17 <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          product_pipeline_phase0_1 <= product_phase0_1;
          product_pipeline_phase1_1 <= product_phase1_1;
          product_pipeline_phase0_2 <= product_phase0_2;
          product_pipeline_phase1_2 <= product_phase1_2;
          product_pipeline_phase0_3 <= product_phase0_3;
          product_pipeline_phase1_3 <= product_phase1_3;
          product_pipeline_phase0_4 <= product_phase0_4;
          product_pipeline_phase1_4 <= product_phase1_4;
          product_pipeline_phase0_5 <= product_phase0_5;
          product_pipeline_phase1_5 <= product_phase1_5;
          product_pipeline_phase0_6 <= product_phase0_6;
          product_pipeline_phase1_6 <= product_phase1_6;
          product_pipeline_phase0_7 <= product_phase0_7;
          product_pipeline_phase1_7 <= product_phase1_7;
          product_pipeline_phase0_8 <= product_phase0_8;
          product_pipeline_phase1_8 <= product_phase1_8;
          product_pipeline_phase0_9 <= product_phase0_9;
          product_pipeline_phase1_9 <= product_phase1_9;
          product_pipeline_phase0_10 <= product_phase0_10;
          product_pipeline_phase1_10 <= product_phase1_10;
          product_pipeline_phase0_11 <= product_phase0_11;
          product_pipeline_phase1_11 <= product_phase1_11;
          product_pipeline_phase0_12 <= product_phase0_12;
          product_pipeline_phase1_12 <= product_phase1_12;
          product_pipeline_phase0_13 <= product_phase0_13;
          product_pipeline_phase1_13 <= product_phase1_13;
          product_pipeline_phase0_14 <= product_phase0_14;
          product_pipeline_phase1_14 <= product_phase1_14;
          product_pipeline_phase0_15 <= product_phase0_15;
          product_pipeline_phase1_15 <= product_phase1_15;
          product_pipeline_phase0_16 <= product_phase0_16;
          product_pipeline_phase1_16 <= product_phase1_16;
          product_pipeline_phase0_17 <= product_phase0_17;
          product_pipeline_phase1_17 <= product_phase1_17;
        end
      end
    end // product_pipeline_process1

  assign add_signext = product_pipeline_phase1_1;
  assign add_signext_1 = product_pipeline_phase1_2;
  assign add_temp = add_signext + add_signext_1;
  assign sumvector1[0] = $signed({{4{add_temp[31]}}, add_temp});

  assign add_signext_2 = product_pipeline_phase1_3;
  assign add_signext_3 = product_pipeline_phase1_4;
  assign add_temp_1 = add_signext_2 + add_signext_3;
  assign sumvector1[1] = $signed({{4{add_temp_1[31]}}, add_temp_1});

  assign add_signext_4 = product_pipeline_phase1_5;
  assign add_signext_5 = product_pipeline_phase1_6;
  assign add_temp_2 = add_signext_4 + add_signext_5;
  assign sumvector1[2] = $signed({{4{add_temp_2[31]}}, add_temp_2});

  assign add_signext_6 = product_pipeline_phase1_7;
  assign add_signext_7 = product_pipeline_phase1_8;
  assign add_temp_3 = add_signext_6 + add_signext_7;
  assign sumvector1[3] = $signed({{4{add_temp_3[31]}}, add_temp_3});

  assign add_signext_8 = product_pipeline_phase1_9;
  assign add_signext_9 = product_pipeline_phase1_10;
  assign add_temp_4 = add_signext_8 + add_signext_9;
  assign sumvector1[4] = $signed({{4{add_temp_4[31]}}, add_temp_4});

  assign add_signext_10 = product_pipeline_phase1_11;
  assign add_signext_11 = product_pipeline_phase1_12;
  assign add_temp_5 = add_signext_10 + add_signext_11;
  assign sumvector1[5] = $signed({{4{add_temp_5[31]}}, add_temp_5});

  assign add_signext_12 = product_pipeline_phase1_13;
  assign add_signext_13 = product_pipeline_phase1_14;
  assign add_temp_6 = add_signext_12 + add_signext_13;
  assign sumvector1[6] = $signed({{4{add_temp_6[31]}}, add_temp_6});

  assign add_signext_14 = product_pipeline_phase1_15;
  assign add_signext_15 = product_pipeline_phase1_16;
  assign add_temp_7 = add_signext_14 + add_signext_15;
  assign sumvector1[7] = $signed({{4{add_temp_7[31]}}, add_temp_7});

  assign add_signext_16 = product_pipeline_phase1_17;
  assign add_signext_17 = product_pipeline_phase0_1;
  assign add_temp_8 = add_signext_16 + add_signext_17;
  assign sumvector1[8] = $signed({{4{add_temp_8[31]}}, add_temp_8});

  assign add_signext_18 = product_pipeline_phase0_2;
  assign add_signext_19 = product_pipeline_phase0_3;
  assign add_temp_9 = add_signext_18 + add_signext_19;
  assign sumvector1[9] = $signed({{4{add_temp_9[31]}}, add_temp_9});

  assign add_signext_20 = product_pipeline_phase0_4;
  assign add_signext_21 = product_pipeline_phase0_5;
  assign add_temp_10 = add_signext_20 + add_signext_21;
  assign sumvector1[10] = $signed({{4{add_temp_10[31]}}, add_temp_10});

  assign add_signext_22 = product_pipeline_phase0_6;
  assign add_signext_23 = product_pipeline_phase0_7;
  assign add_temp_11 = add_signext_22 + add_signext_23;
  assign sumvector1[11] = $signed({{4{add_temp_11[31]}}, add_temp_11});

  assign add_signext_24 = product_pipeline_phase0_8;
  assign add_signext_25 = product_pipeline_phase0_9;
  assign add_temp_12 = add_signext_24 + add_signext_25;
  assign sumvector1[12] = $signed({{4{add_temp_12[31]}}, add_temp_12});

  assign add_signext_26 = product_pipeline_phase0_10;
  assign add_signext_27 = product_pipeline_phase0_11;
  assign add_temp_13 = add_signext_26 + add_signext_27;
  assign sumvector1[13] = $signed({{4{add_temp_13[31]}}, add_temp_13});

  assign add_signext_28 = product_pipeline_phase0_12;
  assign add_signext_29 = product_pipeline_phase0_13;
  assign add_temp_14 = add_signext_28 + add_signext_29;
  assign sumvector1[14] = $signed({{4{add_temp_14[31]}}, add_temp_14});

  assign add_signext_30 = product_pipeline_phase0_14;
  assign add_signext_31 = product_pipeline_phase0_15;
  assign add_temp_15 = add_signext_30 + add_signext_31;
  assign sumvector1[15] = $signed({{4{add_temp_15[31]}}, add_temp_15});

  assign add_signext_32 = product_pipeline_phase0_16;
  assign add_signext_33 = product_pipeline_phase0_17;
  assign add_temp_16 = add_signext_32 + add_signext_33;
  assign sumvector1[16] = $signed({{4{add_temp_16[31]}}, add_temp_16});

  always @ (posedge clk or posedge reset)
    begin: sumdelay_pipeline_process1
      if (reset == 1'b1) begin
        sumdelay_pipeline1[0] <= 0;
        sumdelay_pipeline1[1] <= 0;
        sumdelay_pipeline1[2] <= 0;
        sumdelay_pipeline1[3] <= 0;
        sumdelay_pipeline1[4] <= 0;
        sumdelay_pipeline1[5] <= 0;
        sumdelay_pipeline1[6] <= 0;
        sumdelay_pipeline1[7] <= 0;
        sumdelay_pipeline1[8] <= 0;
        sumdelay_pipeline1[9] <= 0;
        sumdelay_pipeline1[10] <= 0;
        sumdelay_pipeline1[11] <= 0;
        sumdelay_pipeline1[12] <= 0;
        sumdelay_pipeline1[13] <= 0;
        sumdelay_pipeline1[14] <= 0;
        sumdelay_pipeline1[15] <= 0;
        sumdelay_pipeline1[16] <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          sumdelay_pipeline1[0] <= sumvector1[0];
          sumdelay_pipeline1[1] <= sumvector1[1];
          sumdelay_pipeline1[2] <= sumvector1[2];
          sumdelay_pipeline1[3] <= sumvector1[3];
          sumdelay_pipeline1[4] <= sumvector1[4];
          sumdelay_pipeline1[5] <= sumvector1[5];
          sumdelay_pipeline1[6] <= sumvector1[6];
          sumdelay_pipeline1[7] <= sumvector1[7];
          sumdelay_pipeline1[8] <= sumvector1[8];
          sumdelay_pipeline1[9] <= sumvector1[9];
          sumdelay_pipeline1[10] <= sumvector1[10];
          sumdelay_pipeline1[11] <= sumvector1[11];
          sumdelay_pipeline1[12] <= sumvector1[12];
          sumdelay_pipeline1[13] <= sumvector1[13];
          sumdelay_pipeline1[14] <= sumvector1[14];
          sumdelay_pipeline1[15] <= sumvector1[15];
          sumdelay_pipeline1[16] <= sumvector1[16];
        end
      end
    end // sumdelay_pipeline_process1

//  assign add_signext_34 = sumdelay_pipeline1[0];
//  assign add_signext_35 = sumdelay_pipeline1[1];
//  assign add_temp_17 = add_signext_34 + add_signext_35;
//  assign sumvector2[0] = (add_temp_17[36] == 1'b0 & add_temp_17[35] != 1'b0) ? 36'b01111111111111111111111111111111 : 
//      (add_temp_17[32] == 1'b1 && add_temp_17[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_17[31:0];

//  assign add_signext_36 = sumdelay_pipeline1[2];
//  assign add_signext_37 = sumdelay_pipeline1[3];
//  assign add_temp_18 = add_signext_36 + add_signext_37;
//  assign sumvector2[1] = (add_temp_18[32] == 1'b0 & add_temp_18[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_18[32] == 1'b1 && add_temp_18[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_18[31:0];

//  assign add_signext_38 = sumdelay_pipeline1[4];
//  assign add_signext_39 = sumdelay_pipeline1[5];
//  assign add_temp_19 = add_signext_38 + add_signext_39;
//  assign sumvector2[2] = (add_temp_19[32] == 1'b0 & add_temp_19[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_19[32] == 1'b1 && add_temp_19[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_19[31:0];

//  assign add_signext_40 = sumdelay_pipeline1[6];
//  assign add_signext_41 = sumdelay_pipeline1[7];
//  assign add_temp_20 = add_signext_40 + add_signext_41;
//  assign sumvector2[3] = (add_temp_20[32] == 1'b0 & add_temp_20[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_20[32] == 1'b1 && add_temp_20[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_20[31:0];

//  assign add_signext_42 = sumdelay_pipeline1[8];
//  assign add_signext_43 = sumdelay_pipeline1[9];
//  assign add_temp_21 = add_signext_42 + add_signext_43;
//  assign sumvector2[4] = (add_temp_21[32] == 1'b0 & add_temp_21[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_21[32] == 1'b1 && add_temp_21[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_21[31:0];

//  assign add_signext_44 = sumdelay_pipeline1[10];
//  assign add_signext_45 = sumdelay_pipeline1[11];
//  assign add_temp_22 = add_signext_44 + add_signext_45;
//  assign sumvector2[5] = (add_temp_22[32] == 1'b0 & add_temp_22[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_22[32] == 1'b1 && add_temp_22[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_22[31:0];

//  assign add_signext_46 = sumdelay_pipeline1[12];
//  assign add_signext_47 = sumdelay_pipeline1[13];
//  assign add_temp_23 = add_signext_46 + add_signext_47;
//  assign sumvector2[6] = (add_temp_23[32] == 1'b0 & add_temp_23[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_23[32] == 1'b1 && add_temp_23[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_23[31:0];

//  assign add_signext_48 = sumdelay_pipeline1[14];
//  assign add_signext_49 = sumdelay_pipeline1[15];
//  assign add_temp_24 = add_signext_48 + add_signext_49;
//  assign sumvector2[7] = (add_temp_24[32] == 1'b0 & add_temp_24[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_24[32] == 1'b1 && add_temp_24[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_24[31:0];

//  assign sumvector2[8] = sumdelay_pipeline1[16];

assign add_signext_34 = sumdelay_pipeline1[0];
  assign add_signext_35 = sumdelay_pipeline1[1];
  assign add_temp_17 = add_signext_34 + add_signext_35;
  assign sumvector2[0] = (add_temp_17[36] == 1'b0 & add_temp_17[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_17[36] == 1'b1 && add_temp_17[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_17[35:0];
  assign add_signext_36 = sumdelay_pipeline1[2];
  assign add_signext_37 = sumdelay_pipeline1[3];
  assign add_temp_18 = add_signext_36 + add_signext_37;
  assign sumvector2[1] = (add_temp_18[36] == 1'b0 & add_temp_18[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_18[36] == 1'b1 && add_temp_18[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_18[35:0];
  assign add_signext_38 = sumdelay_pipeline1[4];
  assign add_signext_39 = sumdelay_pipeline1[5];
  assign add_temp_19 = add_signext_38 + add_signext_39;
  assign sumvector2[2] = (add_temp_19[36] == 1'b0 & add_temp_19[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_19[36] == 1'b1 && add_temp_19[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_19[35:0];
  assign add_signext_40 = sumdelay_pipeline1[6];
  assign add_signext_41 = sumdelay_pipeline1[7];
  assign add_temp_20 = add_signext_40 + add_signext_41;
  assign sumvector2[3] = (add_temp_20[36] == 1'b0 & add_temp_20[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_20[36] == 1'b1 && add_temp_20[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_20[35:0];
  assign add_signext_42 = sumdelay_pipeline1[8];
  assign add_signext_43 = sumdelay_pipeline1[9];
  assign add_temp_21 = add_signext_42 + add_signext_43;
  assign sumvector2[4] = (add_temp_21[36] == 1'b0 & add_temp_21[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_21[36] == 1'b1 && add_temp_21[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_21[35:0];
  assign add_signext_44 = sumdelay_pipeline1[10];
  assign add_signext_45 = sumdelay_pipeline1[11];
  assign add_temp_22 = add_signext_44 + add_signext_45;
  assign sumvector2[5] = (add_temp_22[36] == 1'b0 & add_temp_22[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_22[36] == 1'b1 && add_temp_22[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_22[35:0];
  assign add_signext_46 = sumdelay_pipeline1[12];
  assign add_signext_47 = sumdelay_pipeline1[13];
  assign add_temp_23 = add_signext_46 + add_signext_47;
  assign sumvector2[6] = (add_temp_23[36] == 1'b0 & add_temp_23[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_23[36] == 1'b1 && add_temp_23[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_23[35:0];
  assign add_signext_48 = sumdelay_pipeline1[14];
  assign add_signext_49 = sumdelay_pipeline1[15];
  assign add_temp_24 = add_signext_48 + add_signext_49;
  assign sumvector2[7] = (add_temp_24[36] == 1'b0 & add_temp_24[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_24[36] == 1'b1 && add_temp_24[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_24[35:0];
  assign sumvector2[8] = sumdelay_pipeline1[16];


  always @ (posedge clk or posedge reset)
    begin: sumdelay_pipeline_process2
      if (reset == 1'b1) begin
        sumdelay_pipeline2[0] <= 0;
        sumdelay_pipeline2[1] <= 0;
        sumdelay_pipeline2[2] <= 0;
        sumdelay_pipeline2[3] <= 0;
        sumdelay_pipeline2[4] <= 0;
        sumdelay_pipeline2[5] <= 0;
        sumdelay_pipeline2[6] <= 0;
        sumdelay_pipeline2[7] <= 0;
        sumdelay_pipeline2[8] <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          sumdelay_pipeline2[0] <= sumvector2[0];
          sumdelay_pipeline2[1] <= sumvector2[1];
          sumdelay_pipeline2[2] <= sumvector2[2];
          sumdelay_pipeline2[3] <= sumvector2[3];
          sumdelay_pipeline2[4] <= sumvector2[4];
          sumdelay_pipeline2[5] <= sumvector2[5];
          sumdelay_pipeline2[6] <= sumvector2[6];
          sumdelay_pipeline2[7] <= sumvector2[7];
          sumdelay_pipeline2[8] <= sumvector2[8];
        end
      end
    end // sumdelay_pipeline_process2

//  assign add_signext_50 = sumdelay_pipeline2[0];
//  assign add_signext_51 = sumdelay_pipeline2[1];
//  assign add_temp_25 = add_signext_50 + add_signext_51;
//  assign sumvector3[0] = (add_temp_25[32] == 1'b0 & add_temp_25[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_25[32] == 1'b1 && add_temp_25[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_25[31:0];

//  assign add_signext_52 = sumdelay_pipeline2[2];
//  assign add_signext_53 = sumdelay_pipeline2[3];
//  assign add_temp_26 = add_signext_52 + add_signext_53;
//  assign sumvector3[1] = (add_temp_26[32] == 1'b0 & add_temp_26[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_26[32] == 1'b1 && add_temp_26[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_26[31:0];

//  assign add_signext_54 = sumdelay_pipeline2[4];
//  assign add_signext_55 = sumdelay_pipeline2[5];
//  assign add_temp_27 = add_signext_54 + add_signext_55;
//  assign sumvector3[2] = (add_temp_27[32] == 1'b0 & add_temp_27[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_27[32] == 1'b1 && add_temp_27[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_27[31:0];

//  assign add_signext_56 = sumdelay_pipeline2[6];
//  assign add_signext_57 = sumdelay_pipeline2[7];
//  assign add_temp_28 = add_signext_56 + add_signext_57;
//  assign sumvector3[3] = (add_temp_28[32] == 1'b0 & add_temp_28[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_28[32] == 1'b1 && add_temp_28[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_28[31:0];

//  assign sumvector3[4] = sumdelay_pipeline2[8];

assign add_signext_50 = sumdelay_pipeline2[0];
  assign add_signext_51 = sumdelay_pipeline2[1];
  assign add_temp_25 = add_signext_50 + add_signext_51;
  assign sumvector3[0] = (add_temp_25[36] == 1'b0 & add_temp_25[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_25[36] == 1'b1 && add_temp_25[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_25[35:0];
  assign add_signext_52 = sumdelay_pipeline2[2];
  assign add_signext_53 = sumdelay_pipeline2[3];
  assign add_temp_26 = add_signext_52 + add_signext_53;
  assign sumvector3[1] = (add_temp_26[36] == 1'b0 & add_temp_26[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_26[36] == 1'b1 && add_temp_26[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_26[35:0];
  assign add_signext_54 = sumdelay_pipeline2[4];
  assign add_signext_55 = sumdelay_pipeline2[5];
  assign add_temp_27 = add_signext_54 + add_signext_55;
  assign sumvector3[2] = (add_temp_27[36] == 1'b0 & add_temp_27[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_27[36] == 1'b1 && add_temp_27[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_27[35:0];
  assign add_signext_56 = sumdelay_pipeline2[6];
  assign add_signext_57 = sumdelay_pipeline2[7];
  assign add_temp_28 = add_signext_56 + add_signext_57;
  assign sumvector3[3] = (add_temp_28[36] == 1'b0 & add_temp_28[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_28[36] == 1'b1 && add_temp_28[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_28[35:0];
  assign sumvector3[4] = sumdelay_pipeline2[8];

  always @ (posedge clk or posedge reset)
    begin: sumdelay_pipeline_process3
      if (reset == 1'b1) begin
        sumdelay_pipeline3[0] <= 0;
        sumdelay_pipeline3[1] <= 0;
        sumdelay_pipeline3[2] <= 0;
        sumdelay_pipeline3[3] <= 0;
        sumdelay_pipeline3[4] <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          sumdelay_pipeline3[0] <= sumvector3[0];
          sumdelay_pipeline3[1] <= sumvector3[1];
          sumdelay_pipeline3[2] <= sumvector3[2];
          sumdelay_pipeline3[3] <= sumvector3[3];
          sumdelay_pipeline3[4] <= sumvector3[4];
        end
      end
    end // sumdelay_pipeline_process3

//  assign add_signext_58 = sumdelay_pipeline3[0];
//  assign add_signext_59 = sumdelay_pipeline3[1];
//  assign add_temp_29 = add_signext_58 + add_signext_59;
//  assign sumvector4[0] = (add_temp_29[32] == 1'b0 & add_temp_29[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_29[32] == 1'b1 && add_temp_29[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_29[31:0];

//  assign add_signext_60 = sumdelay_pipeline3[2];
//  assign add_signext_61 = sumdelay_pipeline3[3];
//  assign add_temp_30 = add_signext_60 + add_signext_61;
//  assign sumvector4[1] = (add_temp_30[32] == 1'b0 & add_temp_30[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_30[32] == 1'b1 && add_temp_30[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_30[31:0];

//  assign sumvector4[2] = sumdelay_pipeline3[4];

assign add_signext_58 = sumdelay_pipeline3[0];
  assign add_signext_59 = sumdelay_pipeline3[1];
  assign add_temp_29 = add_signext_58 + add_signext_59;
  assign sumvector4[0] = (add_temp_29[36] == 1'b0 & add_temp_29[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_29[36] == 1'b1 && add_temp_29[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_29[35:0];
  assign add_signext_60 = sumdelay_pipeline3[2];
  assign add_signext_61 = sumdelay_pipeline3[3];
  assign add_temp_30 = add_signext_60 + add_signext_61;
  assign sumvector4[1] = (add_temp_30[36] == 1'b0 & add_temp_30[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_30[36] == 1'b1 && add_temp_30[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_30[35:0];
  assign sumvector4[2] = sumdelay_pipeline3[4];

  always @ (posedge clk or posedge reset)
    begin: sumdelay_pipeline_process4
      if (reset == 1'b1) begin
        sumdelay_pipeline4[0] <= 0;
        sumdelay_pipeline4[1] <= 0;
        sumdelay_pipeline4[2] <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          sumdelay_pipeline4[0] <= sumvector4[0];
          sumdelay_pipeline4[1] <= sumvector4[1];
          sumdelay_pipeline4[2] <= sumvector4[2];
        end
      end
    end // sumdelay_pipeline_process4

//  assign add_signext_62 = sumdelay_pipeline4[0];
//  assign add_signext_63 = sumdelay_pipeline4[1];
//  assign add_temp_31 = add_signext_62 + add_signext_63;
//  assign sumvector5[0] = (add_temp_31[32] == 1'b0 & add_temp_31[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_31[32] == 1'b1 && add_temp_31[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_31[31:0];

//  assign sumvector5[1] = sumdelay_pipeline4[2];
assign add_signext_62 = sumdelay_pipeline4[0];
  assign add_signext_63 = sumdelay_pipeline4[1];
  assign add_temp_31 = add_signext_62 + add_signext_63;
  assign sumvector5[0] = (add_temp_31[36] == 1'b0 & add_temp_31[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_31[36] == 1'b1 && add_temp_31[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_31[35:0];
  assign sumvector5[1] = sumdelay_pipeline4[2];

  always @ (posedge clk or posedge reset)
    begin: sumdelay_pipeline_process5
      if (reset == 1'b1) begin
        sumdelay_pipeline5[0] <= 0;
        sumdelay_pipeline5[1] <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          sumdelay_pipeline5[0] <= sumvector5[0];
          sumdelay_pipeline5[1] <= sumvector5[1];
        end
      end
    end // sumdelay_pipeline_process5

//  assign add_signext_64 = sumdelay_pipeline5[0];
//  assign add_signext_65 = sumdelay_pipeline5[1];
//  assign add_temp_32 = add_signext_64 + add_signext_65;
//  assign sum6 = (add_temp_32[32] == 1'b0 & add_temp_32[31] != 1'b0) ? 32'b01111111111111111111111111111111 : 
//      (add_temp_32[32] == 1'b1 && add_temp_32[31] != 1'b1) ? 32'b10000000000000000000000000000000 : add_temp_32[31:0];

//  assign output_typeconvert = (sum6[31] == 1'b0 & sum6[30:26] != 5'b00000) ? 16'b0111111111111111 : 
//      (sum6[31] == 1'b1 && sum6[30:26] != 5'b11111) ? 16'b1000000000000000 : $signed({1'b0, sum6[26:11]} + (sum6[31] & |sum6[10:0]));

assign add_signext_64 = sumdelay_pipeline5[0];
  assign add_signext_65 = sumdelay_pipeline5[1];
  assign add_temp_32 = add_signext_64 + add_signext_65;
  assign sum6 = (add_temp_32[36] == 1'b0 & add_temp_32[35] != 1'b0) ? 36'b011111111111111111111111111111111111 :
      (add_temp_32[36] == 1'b1 && add_temp_32[35] != 1'b1) ? 36'b100000000000000000000000000000000000 : add_temp_32[35:0];
  assign output_typeconvert = (sum6[35] == 1'b0 & sum6[34:30] != 5'b00000) ? 16'b0111111111111111 :
      (sum6[35] == 1'b1 && sum6[34:30] != 5'b11111) ? 16'b1000000000000000 : $signed({1'b0, sum6[30:15]} + (sum6[35] & |sum6[14:0]));

  always @ (posedge clk or posedge reset)
    begin: ce_delay
      if (reset == 1'b1) begin
        ce_delayline1 <= 1'b0;
        ce_delayline2 <= 1'b0;
        ce_delayline3 <= 1'b0;
        ce_delayline4 <= 1'b0;
        ce_delayline5 <= 1'b0;
        ce_delayline6 <= 1'b0;
        ce_delayline7 <= 1'b0;
        ce_delayline8 <= 1'b0;
        ce_delayline9 <= 1'b0;
        ce_delayline10 <= 1'b0;
        ce_delayline11 <= 1'b0;
        ce_delayline12 <= 1'b0;
        ce_delayline13 <= 1'b0;
        ce_delayline14 <= 1'b0;
      end
      else begin
        if (clk_enable == 1'b1) begin
          ce_delayline1 <= clk_enable;
          ce_delayline2 <= ce_delayline1;
          ce_delayline3 <= ce_delayline2;
          ce_delayline4 <= ce_delayline3;
          ce_delayline5 <= ce_delayline4;
          ce_delayline6 <= ce_delayline5;
          ce_delayline7 <= ce_delayline6;
          ce_delayline8 <= ce_delayline7;
          ce_delayline9 <= ce_delayline8;
          ce_delayline10 <= ce_delayline9;
          ce_delayline11 <= ce_delayline10;
          ce_delayline12 <= ce_delayline11;
          ce_delayline13 <= ce_delayline12;
          ce_delayline14 <= ce_delayline13;
        end
      end
    end // ce_delay

  assign ce_gated =  ce_delayline14 & ce_out_reg;

  always @ (posedge clk or posedge reset)
    begin: output_register_process
      if (reset == 1'b1) begin
        output_register <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          output_register <= output_typeconvert;
        end
      end
    end // output_register_process

  // Assignment Statements
  assign ce_out = ce_gated;
  assign filter_out = output_register;
endmodule  // cic_comp_down
