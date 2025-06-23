

`timescale 1 ns / 1 ns

module hb_down_opt
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
  input   signed [15:0] filter_in; //sfix16_En15
  output  signed [15:0] filter_out; //sfix16_En15
  output  ce_out; 

////////////////////////////////////////////////////////////////
//Module Architecture: hb_down
////////////////////////////////////////////////////////////////
  // Local Functions
  // Type Definitions
  // Constants
  parameter signed [18:0] coeffphase1_1 = 19'b1111111111111101001; //sfix19_En18
  parameter signed [18:0] coeffphase1_2 = 19'b0000000000000100011; //sfix19_En18
  parameter signed [18:0] coeffphase1_3 = 19'b1111111111111000011; //sfix19_En18
  parameter signed [18:0] coeffphase1_4 = 19'b0000000000001100010; //sfix19_En18
  parameter signed [18:0] coeffphase1_5 = 19'b1111111111101101011; //sfix19_En18
  parameter signed [18:0] coeffphase1_6 = 19'b0000000000011011001; //sfix19_En18
  parameter signed [18:0] coeffphase1_7 = 19'b1111111111011001101; //sfix19_En18
  parameter signed [18:0] coeffphase1_8 = 19'b0000000000110101000; //sfix19_En18
  parameter signed [18:0] coeffphase1_9 = 19'b1111111110111000100; //sfix19_En18
  parameter signed [18:0] coeffphase1_10 = 19'b0000000001011110101; //sfix19_En18
  parameter signed [18:0] coeffphase1_11 = 19'b1111111110000100110; //sfix19_En18
  parameter signed [18:0] coeffphase1_12 = 19'b0000000010011110101; //sfix19_En18
  parameter signed [18:0] coeffphase1_13 = 19'b1111111100110110010; //sfix19_En18
  parameter signed [18:0] coeffphase1_14 = 19'b0000000011111110011; //sfix19_En18
  parameter signed [18:0] coeffphase1_15 = 19'b1111111011000001000; //sfix19_En18
  parameter signed [18:0] coeffphase1_16 = 19'b0000000110001110100; //sfix19_En18
  parameter signed [18:0] coeffphase1_17 = 19'b1111111000001110000; //sfix19_En18
  parameter signed [18:0] coeffphase1_18 = 19'b0000001001110001010; //sfix19_En18
  parameter signed [18:0] coeffphase1_19 = 19'b1111110011100110001; //sfix19_En18
  parameter signed [18:0] coeffphase1_20 = 19'b0000010000000101011; //sfix19_En18
  parameter signed [18:0] coeffphase1_21 = 19'b1111101010010100110; //sfix19_En18
  parameter signed [18:0] coeffphase1_22 = 19'b0000011111011011100; //sfix19_En18
  parameter signed [18:0] coeffphase1_23 = 19'b1111001010011000011; //sfix19_En18
  parameter signed [18:0] coeffphase1_24 = 19'b0010100010101111010; //sfix19_En18
  parameter signed [18:0] coeffphase2_24 = 19'b0100000000000000000;  

  // Signals
  reg  [1:0] ring_count; // ufix2
  wire phase_0; // boolean
  wire phase_1; // boolean
  reg  ce_out_reg; // boolean
  reg  signed [15:0] input_register; // sfix16_En15
  reg  signed [15:0] input_pipeline_phase0 [0:46] ; // sfix16_En15
  reg  signed [15:0] input_pipeline_phase1 [0:23] ; // sfix16_En15
  wire signed [16:0] tapsum1; // sfix17_En15
  wire signed [16:0] tapsum2; // sfix17_En15
  wire signed [16:0] tapsum3; // sfix17_En15
  wire signed [16:0] tapsum4; // sfix17_En15
  wire signed [16:0] tapsum5; // sfix17_En15
  wire signed [16:0] tapsum6; // sfix17_En15
  wire signed [16:0] tapsum7; // sfix17_En15
  wire signed [16:0] tapsum8; // sfix17_En15
  wire signed [16:0] tapsum9; // sfix17_En15
  wire signed [16:0] tapsum10; // sfix17_En15
  wire signed [16:0] tapsum11; // sfix17_En15
  wire signed [16:0] tapsum12; // sfix17_En15
  wire signed [16:0] tapsum13; // sfix17_En15
  wire signed [16:0] tapsum14; // sfix17_En15
  wire signed [16:0] tapsum15; // sfix17_En15
  wire signed [16:0] tapsum16; // sfix17_En15
  wire signed [16:0] tapsum17; // sfix17_En15
  wire signed [16:0] tapsum18; // sfix17_En15
  wire signed [16:0] tapsum19; // sfix17_En15
  wire signed [16:0] tapsum20; // sfix17_En15
  wire signed [16:0] tapsum21; // sfix17_En15
  wire signed [16:0] tapsum22; // sfix17_En15
  wire signed [16:0] tapsum23; // sfix17_En15
  wire signed [16:0] tapsum24; // sfix17_En15

  wire signed [35:0] product_phase0_1; // sfix35_En33
  wire signed [35:0] product_phase0_2; // sfix35_En33
  wire signed [35:0] product_phase0_3; // sfix35_En33
  wire signed [35:0] product_phase0_4; // sfix35_En33
  wire signed [35:0] product_phase0_5; // sfix35_En33
  wire signed [35:0] product_phase0_6; // sfix35_En33
  wire signed [35:0] product_phase0_7; // sfix35_En33
  wire signed [35:0] product_phase0_8; // sfix35_En33
  wire signed [35:0] product_phase0_9; // sfix35_En33
  wire signed [35:0] product_phase0_10; // sfix35_En33
  wire signed [35:0] product_phase0_11; // sfix35_En33
  wire signed [35:0] product_phase0_12; // sfix35_En33
  wire signed [35:0] product_phase0_13; // sfix35_En33
  wire signed [35:0] product_phase0_14; // sfix35_En33
  wire signed [35:0] product_phase0_15; // sfix35_En33
  wire signed [35:0] product_phase0_16; // sfix35_En33
  wire signed [35:0] product_phase0_17; // sfix35_En33
  wire signed [35:0] product_phase0_18; // sfix35_En33
  wire signed [35:0] product_phase0_19; // sfix35_En33
  wire signed [35:0] product_phase0_20; // sfix35_En33
  wire signed [35:0] product_phase0_21; // sfix35_En33
  wire signed [35:0] product_phase0_22; // sfix35_En33
  wire signed [35:0] product_phase0_23; // sfix35_En33
  wire signed [35:0] product_phase0_24; // sfix35_En33
  wire signed [35:0] product_phase1_24;

  reg  signed [35:0] product_pipeline_phase0_1; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_2; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_3; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_4; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_5; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_6; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_7; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_8; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_9; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_10; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_11; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_12; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_13; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_14; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_15; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_16; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_17; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_18; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_19; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_20; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_21; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_22; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_23; // sfix35_En33
  reg  signed [35:0] product_pipeline_phase0_24; // sfix35_En33

  reg  signed [34:0] product_pipeline_phase1_24; // sfix35_En33
  
  wire signed [40:0] sumvector1 [0:12] ; // sfix41_En33

  wire signed [35:0] add_signext_48; // sfix35_En33
  wire signed [35:0] add_signext_49; // sfix35_En33
  wire signed [36:0] add_temp; // sfix36_En33

  wire signed [35:0] add_signext_50;
  wire signed [35:0] add_signext_51;
  wire signed [36:0] add_temp_1;

  wire signed [35:0] add_signext_52;
  wire signed [35:0] add_signext_53;
  wire signed [36:0] add_temp_2;

  wire signed [35:0] add_signext_54;
  wire signed [35:0] add_signext_55;
  wire signed [36:0] add_temp_3;

  wire signed [35:0] add_signext_56;
  wire signed [35:0] add_signext_57;
  wire signed [36:0] add_temp_4;

  wire signed [35:0] add_signext_58;
  wire signed [35:0] add_signext_59;
  wire signed [36:0] add_temp_5;

  wire signed [35:0] add_signext_60;
  wire signed [35:0] add_signext_61;
  wire signed [36:0] add_temp_6;

  wire signed [35:0] add_signext_62;
  wire signed [35:0] add_signext_63;
  wire signed [36:0] add_temp_7;

  wire signed [35:0] add_signext_64;
  wire signed [35:0] add_signext_65;
  wire signed [36:0] add_temp_8;

  wire signed [35:0] add_signext_66;
  wire signed [35:0] add_signext_67;
  wire signed [36:0] add_temp_9;

  wire signed [35:0] add_signext_68;
  wire signed [35:0] add_signext_69;
  wire signed [36:0] add_temp_10;

  wire signed [35:0] add_signext_70;
  wire signed [35:0] add_signext_71;
  wire signed [36:0] add_temp_11;

  wire signed [35:0] add_signext_72;
  wire signed [35:0] add_signext_73;
  wire signed [36:0] add_temp_12;

  wire signed [35:0] add_signext_74;
  wire signed [35:0] add_signext_75;
  wire signed [36:0] add_temp_13;

  wire signed [35:0] add_signext_76;
  wire signed [35:0] add_signext_77;
  wire signed [36:0] add_temp_14;

  wire signed [35:0] add_signext_78;
  wire signed [35:0] add_signext_79;
  wire signed [36:0] add_temp_15;

  wire signed [35:0] add_signext_80;
  wire signed [35:0] add_signext_81;
  wire signed [36:0] add_temp_16;

  wire signed [35:0] add_signext_82;
  wire signed [35:0] add_signext_83;
  wire signed [36:0] add_temp_17;

  wire signed [35:0] add_signext_84;
  wire signed [35:0] add_signext_85;
  wire signed [36:0] add_temp_18;

  wire signed [35:0] add_signext_86;
  wire signed [35:0] add_signext_87;
  wire signed [36:0] add_temp_19;

  wire signed [35:0] add_signext_88;
  wire signed [35:0] add_signext_89;
  wire signed [36:0] add_temp_20;

  wire signed [35:0] add_signext_90;
  wire signed [35:0] add_signext_91;
  wire signed [36:0] add_temp_21;

  wire signed [35:0] add_signext_92;
  wire signed [35:0] add_signext_93;
  wire signed [36:0] add_temp_22;

  wire signed [35:0] add_signext_94;
  wire signed [35:0] add_signext_95;
  wire signed [36:0] add_temp_23;

  reg  signed [40:0] sumdelay_pipeline1 [0:12] ; // sfix41_En33
  wire signed [40:0] sumvector2 [0:6] ; // sfix41_En33
  wire signed [40:0] add_signext_96; // sfix41_En33
  wire signed [40:0] add_signext_97; // sfix41_En33
  wire signed [41:0] add_temp_24; // sfix42_En33
  wire signed [40:0] add_signext_98; // sfix41_En33
  wire signed [40:0] add_signext_99; // sfix41_En33
  wire signed [41:0] add_temp_25; // sfix42_En33
  wire signed [40:0] add_signext_100; // sfix41_En33
  wire signed [40:0] add_signext_101; // sfix41_En33
  wire signed [41:0] add_temp_26; // sfix42_En33
  wire signed [40:0] add_signext_102; // sfix41_En33
  wire signed [40:0] add_signext_103; // sfix41_En33
  wire signed [41:0] add_temp_27; // sfix42_En33
  wire signed [40:0] add_signext_104; // sfix41_En33
  wire signed [40:0] add_signext_105; // sfix41_En33
  wire signed [41:0] add_temp_28; // sfix42_En33
  wire signed [40:0] add_signext_106; // sfix41_En33
  wire signed [40:0] add_signext_107; // sfix41_En33
  wire signed [41:0] add_temp_29; // sfix42_En33
  wire signed [40:0] add_signext_108; // sfix41_En33
  wire signed [40:0] add_signext_109; // sfix41_En33
  wire signed [41:0] add_temp_30; // sfix42_En33
  wire signed [40:0] add_signext_110; // sfix41_En33
  wire signed [40:0] add_signext_111; // sfix41_En33
  wire signed [41:0] add_temp_31; // sfix42_En33
  wire signed [40:0] add_signext_112; // sfix41_En33
  wire signed [40:0] add_signext_113; // sfix41_En33
  wire signed [41:0] add_temp_32; // sfix42_En33
  wire signed [40:0] add_signext_114; // sfix41_En33
  wire signed [40:0] add_signext_115; // sfix41_En33
  wire signed [41:0] add_temp_33; // sfix42_En33
  wire signed [40:0] add_signext_116; // sfix41_En33
  wire signed [40:0] add_signext_117; // sfix41_En33
  wire signed [41:0] add_temp_34; // sfix42_En33
  wire signed [40:0] add_signext_118; // sfix41_En33
  wire signed [40:0] add_signext_119; // sfix41_En33
  wire signed [41:0] add_temp_35; // sfix42_En33
  
  reg  signed [40:0] sumdelay_pipeline2 [0:6] ; // sfix41_En33
  wire signed [40:0] sumvector3 [0:3] ; // sfix41_En33
  wire signed [40:0] add_signext_120; // sfix41_En33
  wire signed [40:0] add_signext_121; // sfix41_En33
  wire signed [41:0] add_temp_36; // sfix42_En33
  wire signed [40:0] add_signext_122; // sfix41_En33
  wire signed [40:0] add_signext_123; // sfix41_En33
  wire signed [41:0] add_temp_37; // sfix42_En33
  wire signed [40:0] add_signext_124; // sfix41_En33
  wire signed [40:0] add_signext_125; // sfix41_En33
  wire signed [41:0] add_temp_38; // sfix42_En33
  wire signed [40:0] add_signext_126; // sfix41_En33
  wire signed [40:0] add_signext_127; // sfix41_En33
  wire signed [41:0] add_temp_39; // sfix42_En33
  wire signed [40:0] add_signext_128; // sfix41_En33
  wire signed [40:0] add_signext_129; // sfix41_En33
  wire signed [41:0] add_temp_40; // sfix42_En33
  wire signed [40:0] add_signext_130; // sfix41_En33
  wire signed [40:0] add_signext_131; // sfix41_En33
  wire signed [41:0] add_temp_41; // sfix42_En33
  
  reg  signed [40:0] sumdelay_pipeline3 [0:6] ; // sfix41_En33
  wire signed [40:0] sumvector4 [0:3] ; // sfix41_En33
  wire signed [40:0] add_signext_132; // sfix41_En33
  wire signed [40:0] add_signext_133; // sfix41_En33
  wire signed [41:0] add_temp_42; // sfix42_En33
  wire signed [40:0] add_signext_134; // sfix41_En33
  wire signed [40:0] add_signext_135; // sfix41_En33
  wire signed [41:0] add_temp_43; // sfix42_En33
  wire signed [40:0] add_signext_136; // sfix41_En33
  wire signed [40:0] add_signext_137; // sfix41_En33
  wire signed [41:0] add_temp_44; // sfix42_En33
  reg  signed [40:0] sumdelay_pipeline4 [0:3] ; // sfix41_En33
  wire signed [40:0] sumvector5; // sfix41_En33
  wire signed [40:0] add_signext_138; // sfix41_En33
  wire signed [40:0] add_signext_139; // sfix41_En33
  wire signed [41:0] add_temp_45; // sfix42_En33
  wire signed [15:0] output_typeconvert; // sfix16_En15
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
        input_pipeline_phase0[16] <= 0;
        input_pipeline_phase0[17] <= 0;
        input_pipeline_phase0[18] <= 0;
        input_pipeline_phase0[19] <= 0;
        input_pipeline_phase0[20] <= 0;
        input_pipeline_phase0[21] <= 0;
        input_pipeline_phase0[22] <= 0;
        input_pipeline_phase0[23] <= 0;
        input_pipeline_phase0[24] <= 0;
        input_pipeline_phase0[25] <= 0;
        input_pipeline_phase0[26] <= 0;
        input_pipeline_phase0[27] <= 0;
        input_pipeline_phase0[28] <= 0;
        input_pipeline_phase0[29] <= 0;
        input_pipeline_phase0[30] <= 0;
        input_pipeline_phase0[31] <= 0;
        input_pipeline_phase0[32] <= 0;
        input_pipeline_phase0[33] <= 0;
        input_pipeline_phase0[34] <= 0;
        input_pipeline_phase0[35] <= 0;
        input_pipeline_phase0[36] <= 0;
        input_pipeline_phase0[37] <= 0;
        input_pipeline_phase0[38] <= 0;
        input_pipeline_phase0[39] <= 0;
        input_pipeline_phase0[40] <= 0;
        input_pipeline_phase0[41] <= 0;
        input_pipeline_phase0[42] <= 0;
        input_pipeline_phase0[43] <= 0;
        input_pipeline_phase0[44] <= 0;
        input_pipeline_phase0[45] <= 0;
        input_pipeline_phase0[46] <= 0;
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
          input_pipeline_phase0[16] <= input_pipeline_phase0[15];
          input_pipeline_phase0[17] <= input_pipeline_phase0[16];
          input_pipeline_phase0[18] <= input_pipeline_phase0[17];
          input_pipeline_phase0[19] <= input_pipeline_phase0[18];
          input_pipeline_phase0[20] <= input_pipeline_phase0[19];
          input_pipeline_phase0[21] <= input_pipeline_phase0[20];
          input_pipeline_phase0[22] <= input_pipeline_phase0[21];
          input_pipeline_phase0[23] <= input_pipeline_phase0[22];
          input_pipeline_phase0[24] <= input_pipeline_phase0[23];
          input_pipeline_phase0[25] <= input_pipeline_phase0[24];
          input_pipeline_phase0[26] <= input_pipeline_phase0[25];
          input_pipeline_phase0[27] <= input_pipeline_phase0[26];
          input_pipeline_phase0[28] <= input_pipeline_phase0[27];
          input_pipeline_phase0[29] <= input_pipeline_phase0[28];
          input_pipeline_phase0[30] <= input_pipeline_phase0[29];
          input_pipeline_phase0[31] <= input_pipeline_phase0[30];
          input_pipeline_phase0[32] <= input_pipeline_phase0[31];
          input_pipeline_phase0[33] <= input_pipeline_phase0[32];
          input_pipeline_phase0[34] <= input_pipeline_phase0[33];
          input_pipeline_phase0[35] <= input_pipeline_phase0[34];
          input_pipeline_phase0[36] <= input_pipeline_phase0[35];
          input_pipeline_phase0[37] <= input_pipeline_phase0[36];
          input_pipeline_phase0[38] <= input_pipeline_phase0[37];
          input_pipeline_phase0[39] <= input_pipeline_phase0[38];
          input_pipeline_phase0[40] <= input_pipeline_phase0[39];
          input_pipeline_phase0[41] <= input_pipeline_phase0[40];
          input_pipeline_phase0[42] <= input_pipeline_phase0[41];
          input_pipeline_phase0[43] <= input_pipeline_phase0[42];
          input_pipeline_phase0[44] <= input_pipeline_phase0[43];
          input_pipeline_phase0[45] <= input_pipeline_phase0[44];
          input_pipeline_phase0[46] <= input_pipeline_phase0[45];
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
        input_pipeline_phase1[17] <= 0;
        input_pipeline_phase1[18] <= 0;
        input_pipeline_phase1[19] <= 0;
        input_pipeline_phase1[20] <= 0;
        input_pipeline_phase1[21] <= 0;
        input_pipeline_phase1[22] <= 0;
        input_pipeline_phase1[23] <= 0;
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
          input_pipeline_phase1[17] <= input_pipeline_phase1[16];
          input_pipeline_phase1[18] <= input_pipeline_phase1[17];
          input_pipeline_phase1[19] <= input_pipeline_phase1[18];
          input_pipeline_phase1[20] <= input_pipeline_phase1[19];
          input_pipeline_phase1[21] <= input_pipeline_phase1[20];
          input_pipeline_phase1[22] <= input_pipeline_phase1[21];
          input_pipeline_phase1[23] <= input_pipeline_phase1[22];
        end
      end
    end // Delay_Pipeline_Phase1_process


  assign tapsum1 = input_register + input_pipeline_phase0[46];
  assign tapsum2  = input_pipeline_phase0[ 0] + input_pipeline_phase0[45];
  assign tapsum3  = input_pipeline_phase0[ 1] + input_pipeline_phase0[44];
  assign tapsum4  = input_pipeline_phase0[ 2] + input_pipeline_phase0[43];
  assign tapsum5  = input_pipeline_phase0[ 3] + input_pipeline_phase0[42];
  assign tapsum6  = input_pipeline_phase0[ 4] + input_pipeline_phase0[41];
  assign tapsum7  = input_pipeline_phase0[ 5] + input_pipeline_phase0[40];
  assign tapsum8  = input_pipeline_phase0[ 6] + input_pipeline_phase0[39];
  assign tapsum9  = input_pipeline_phase0[ 7] + input_pipeline_phase0[38];
  assign tapsum10 = input_pipeline_phase0[ 8] + input_pipeline_phase0[37];
  assign tapsum11 = input_pipeline_phase0[ 9] + input_pipeline_phase0[36];
  assign tapsum12 = input_pipeline_phase0[10] + input_pipeline_phase0[35];
  assign tapsum13 = input_pipeline_phase0[11] + input_pipeline_phase0[34];
  assign tapsum14 = input_pipeline_phase0[12] + input_pipeline_phase0[33];
  assign tapsum15 = input_pipeline_phase0[13] + input_pipeline_phase0[32];
  assign tapsum16 = input_pipeline_phase0[14] + input_pipeline_phase0[31];
  assign tapsum17 = input_pipeline_phase0[15] + input_pipeline_phase0[30];
  assign tapsum18 = input_pipeline_phase0[16] + input_pipeline_phase0[29];
  assign tapsum19 = input_pipeline_phase0[17] + input_pipeline_phase0[28];
  assign tapsum20 = input_pipeline_phase0[18] + input_pipeline_phase0[27];
  assign tapsum21 = input_pipeline_phase0[19] + input_pipeline_phase0[26];
  assign tapsum22 = input_pipeline_phase0[20] + input_pipeline_phase0[25];
  assign tapsum23 = input_pipeline_phase0[21] + input_pipeline_phase0[24];
  assign tapsum24 = input_pipeline_phase0[22] + input_pipeline_phase0[23];


  assign product_phase0_1 = tapsum1 * coeffphase1_1;
  assign product_phase0_2 = tapsum2 * coeffphase1_2;
  assign product_phase0_3 = tapsum3 * coeffphase1_3;
  assign product_phase0_4 = tapsum4 * coeffphase1_4;
  assign product_phase0_5 = tapsum5 * coeffphase1_5;
  assign product_phase0_6 = tapsum6 * coeffphase1_6;
  assign product_phase0_7 = tapsum7 * coeffphase1_7;
  assign product_phase0_8  = tapsum8  * coeffphase1_8;
  assign product_phase0_9  = tapsum9  * coeffphase1_9;
  assign product_phase0_10 = tapsum10 * coeffphase1_10;
  assign product_phase0_11 = tapsum11 * coeffphase1_11;
  assign product_phase0_12 = tapsum12 * coeffphase1_12;
  assign product_phase0_13 = tapsum13 * coeffphase1_13;
  assign product_phase0_14 = tapsum14 * coeffphase1_14;
  assign product_phase0_15 = tapsum15 * coeffphase1_15;
  assign product_phase0_16 = tapsum16 * coeffphase1_16;
  assign product_phase0_17 = tapsum17 * coeffphase1_17;
  assign product_phase0_18 = tapsum18 * coeffphase1_18;
  assign product_phase0_19 = tapsum19 * coeffphase1_19;
  assign product_phase0_20 = tapsum20 * coeffphase1_20;
  assign product_phase0_21 = tapsum21 * coeffphase1_21;
  assign product_phase0_22 = tapsum22 * coeffphase1_22;
  assign product_phase0_23 = tapsum23 * coeffphase1_23;
  assign product_phase0_24 = tapsum24 * coeffphase1_24;



  assign product_phase1_24 = $signed(input_pipeline_phase1[23]) * coeffphase2_24;
  
  always @(posedge clk or posedge reset) begin: product_pipeline_process1
  if (reset) begin
    product_pipeline_phase0_1  <= 0;
    product_pipeline_phase0_2  <= 0;
    product_pipeline_phase0_3  <= 0;
    product_pipeline_phase0_4  <= 0;
    product_pipeline_phase0_5  <= 0;
    product_pipeline_phase0_6  <= 0;
    product_pipeline_phase0_7  <= 0;
    product_pipeline_phase0_8  <= 0;
    product_pipeline_phase0_9  <= 0;
    product_pipeline_phase0_10 <= 0;
    product_pipeline_phase0_11 <= 0;
    product_pipeline_phase0_12 <= 0;
    product_pipeline_phase0_13 <= 0;
    product_pipeline_phase0_14 <= 0;
    product_pipeline_phase0_15 <= 0;
    product_pipeline_phase0_16 <= 0;
    product_pipeline_phase0_17 <= 0;
    product_pipeline_phase0_18 <= 0;
    product_pipeline_phase0_19 <= 0;
    product_pipeline_phase0_20 <= 0;
    product_pipeline_phase0_21 <= 0;
    product_pipeline_phase0_22 <= 0;
    product_pipeline_phase0_23 <= 0;
    product_pipeline_phase0_24 <= 0;
    product_pipeline_phase1_24 <= 0;
  end
  else if (phase_1) begin
    product_pipeline_phase0_1  <= product_phase0_1;
    product_pipeline_phase0_2  <= product_phase0_2;
    product_pipeline_phase0_3  <= product_phase0_3;
    product_pipeline_phase0_4  <= product_phase0_4;
    product_pipeline_phase0_5  <= product_phase0_5;
    product_pipeline_phase0_6  <= product_phase0_6;
    product_pipeline_phase0_7  <= product_phase0_7;
    product_pipeline_phase0_8  <= product_phase0_8;
    product_pipeline_phase0_9  <= product_phase0_9;
    product_pipeline_phase0_10 <= product_phase0_10;
    product_pipeline_phase0_11 <= product_phase0_11;
    product_pipeline_phase0_12 <= product_phase0_12;
    product_pipeline_phase0_13 <= product_phase0_13;
    product_pipeline_phase0_14 <= product_phase0_14;
    product_pipeline_phase0_15 <= product_phase0_15;
    product_pipeline_phase0_16 <= product_phase0_16;
    product_pipeline_phase0_17 <= product_phase0_17;
    product_pipeline_phase0_18 <= product_phase0_18;
    product_pipeline_phase0_19 <= product_phase0_19;
    product_pipeline_phase0_20 <= product_phase0_20;
    product_pipeline_phase0_21 <= product_phase0_21;
    product_pipeline_phase0_22 <= product_phase0_22;
    product_pipeline_phase0_23 <= product_phase0_23;
    product_pipeline_phase0_24 <= product_phase0_24;   
    product_pipeline_phase1_24 <= product_phase1_24;
  end
end // product_pipeline_process1

  assign add_signext_48 = product_pipeline_phase1_24;
  assign add_signext_49 = product_pipeline_phase0_1;
  assign add_temp = add_signext_48 + add_signext_49;
  assign sumvector1[0] = $signed({{4{add_temp[36]}}, add_temp});

  assign add_signext_50 = product_pipeline_phase0_2;
  assign add_signext_51 = product_pipeline_phase0_3;
  assign add_temp_1 = add_signext_50 + add_signext_51;
  assign sumvector1[1] = $signed({{4{add_temp_1[35]}}, add_temp_1});

  assign add_signext_52 = product_pipeline_phase0_4;
  assign add_signext_53 = product_pipeline_phase0_5;
  assign add_temp_2 = add_signext_52 + add_signext_53;
  assign sumvector1[2] = $signed({{4{add_temp_2[35]}}, add_temp_2});

  assign add_signext_54 = product_pipeline_phase0_6;
  assign add_signext_55 = product_pipeline_phase0_7;
  assign add_temp_3 = add_signext_54 + add_signext_55;
  assign sumvector1[3] = $signed({{4{add_temp_3[35]}}, add_temp_3});

  assign add_signext_56 = product_pipeline_phase0_8;
  assign add_signext_57 = product_pipeline_phase0_9;
  assign add_temp_4 = add_signext_56 + add_signext_57;
  assign sumvector1[4] = $signed({{4{add_temp_4[35]}}, add_temp_4});

  assign add_signext_58 = product_pipeline_phase0_10;
  assign add_signext_59 = product_pipeline_phase0_11;
  assign add_temp_5 = add_signext_58 + add_signext_59;
  assign sumvector1[5] = $signed({{4{add_temp_5[35]}}, add_temp_5});

  assign add_signext_60 = product_pipeline_phase0_12;
  assign add_signext_61 = product_pipeline_phase0_13;
  assign add_temp_6 = add_signext_60 + add_signext_61;
  assign sumvector1[6] = $signed({{4{add_temp_6[35]}}, add_temp_6});

  assign add_signext_62 = product_pipeline_phase0_14;
  assign add_signext_63 = product_pipeline_phase0_15;
  assign add_temp_7 = add_signext_62 + add_signext_63;
  assign sumvector1[7] = $signed({{4{add_temp_7[35]}}, add_temp_7});

  assign add_signext_64 = product_pipeline_phase0_16;
  assign add_signext_65 = product_pipeline_phase0_17;
  assign add_temp_8 = add_signext_64 + add_signext_65;
  assign sumvector1[8] = $signed({{4{add_temp_8[35]}}, add_temp_8});

  assign add_signext_66 = product_pipeline_phase0_18;
  assign add_signext_67 = product_pipeline_phase0_19;
  assign add_temp_9 = add_signext_66 + add_signext_67;
  assign sumvector1[9] = $signed({{4{add_temp_9[35]}}, add_temp_9});

  assign add_signext_68 = product_pipeline_phase0_20;
  assign add_signext_69 = product_pipeline_phase0_21;
  assign add_temp_10 = add_signext_68 + add_signext_69;
  assign sumvector1[10] = $signed({{4{add_temp_10[35]}}, add_temp_10});

  assign add_signext_70 = product_pipeline_phase0_22;
  assign add_signext_71 = product_pipeline_phase0_23;
  assign add_temp_11 = add_signext_70 + add_signext_71;
  assign sumvector1[11] = $signed({{4{add_temp_11[35]}}, add_temp_11});

 

  assign sumvector1[12] = $signed({{5{product_pipeline_phase0_24[35]}}, product_pipeline_phase0_24});

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
        end
      end
    end // sumdelay_pipeline_process1

  assign add_signext_96 = sumdelay_pipeline1[0];
  assign add_signext_97 = sumdelay_pipeline1[1];
  assign add_temp_24 = add_signext_96 + add_signext_97;
  assign sumvector2[0] = (add_temp_24[41] == 1'b0 & add_temp_24[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_24[41] == 1'b1 && add_temp_24[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_24[40:0];

  assign add_signext_98 = sumdelay_pipeline1[2];
  assign add_signext_99 = sumdelay_pipeline1[3];
  assign add_temp_25 = add_signext_98 + add_signext_99;
  assign sumvector2[1] = (add_temp_25[41] == 1'b0 & add_temp_25[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_25[41] == 1'b1 && add_temp_25[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_25[40:0];

  assign add_signext_100 = sumdelay_pipeline1[4];
  assign add_signext_101 = sumdelay_pipeline1[5];
  assign add_temp_26 = add_signext_100 + add_signext_101;
  assign sumvector2[2] = (add_temp_26[41] == 1'b0 & add_temp_26[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_26[41] == 1'b1 && add_temp_26[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_26[40:0];

  assign add_signext_102 = sumdelay_pipeline1[6];
  assign add_signext_103 = sumdelay_pipeline1[7];
  assign add_temp_27 = add_signext_102 + add_signext_103;
  assign sumvector2[3] = (add_temp_27[41] == 1'b0 & add_temp_27[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_27[41] == 1'b1 && add_temp_27[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_27[40:0];

  assign add_signext_104 = sumdelay_pipeline1[8];
  assign add_signext_105 = sumdelay_pipeline1[9];
  assign add_temp_28 = add_signext_104 + add_signext_105;
  assign sumvector2[4] = (add_temp_28[41] == 1'b0 & add_temp_28[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_28[41] == 1'b1 && add_temp_28[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_28[40:0];

  assign add_signext_106 = sumdelay_pipeline1[10];
  assign add_signext_107 = sumdelay_pipeline1[11];
  assign add_temp_29 = add_signext_106 + add_signext_107;
  assign sumvector2[5] = (add_temp_29[41] == 1'b0 & add_temp_29[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_29[41] == 1'b1 && add_temp_29[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_29[40:0];


  assign sumvector2[6] = sumdelay_pipeline1[12];

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
        end
      end
    end // sumdelay_pipeline_process2

  assign add_signext_120 = sumdelay_pipeline2[0];
  assign add_signext_121 = sumdelay_pipeline2[1];
  assign add_temp_36 = add_signext_120 + add_signext_121;
  assign sumvector3[0] = (add_temp_36[41] == 1'b0 & add_temp_36[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_36[41] == 1'b1 && add_temp_36[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_36[40:0];

  assign add_signext_122 = sumdelay_pipeline2[2];
  assign add_signext_123 = sumdelay_pipeline2[3];
  assign add_temp_37 = add_signext_122 + add_signext_123;
  assign sumvector3[1] = (add_temp_37[41] == 1'b0 & add_temp_37[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_37[41] == 1'b1 && add_temp_37[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_37[40:0];

  assign add_signext_124 = sumdelay_pipeline2[4];
  assign add_signext_125 = sumdelay_pipeline2[5];
  assign add_temp_38 = add_signext_124 + add_signext_125;
  assign sumvector3[2] = (add_temp_38[41] == 1'b0 & add_temp_38[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_38[41] == 1'b1 && add_temp_38[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_38[40:0];

  assign sumvector3[3] = sumdelay_pipeline2[6];

  always @ (posedge clk or posedge reset)
    begin: sumdelay_pipeline_process3
      if (reset == 1'b1) begin
        sumdelay_pipeline3[0] <= 0;
        sumdelay_pipeline3[1] <= 0;
        sumdelay_pipeline3[2] <= 0;
        sumdelay_pipeline3[3] <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          sumdelay_pipeline3[0] <= sumvector3[0];
          sumdelay_pipeline3[1] <= sumvector3[1];
          sumdelay_pipeline3[2] <= sumvector3[2];
          sumdelay_pipeline3[3] <= sumvector3[3];
        end
      end
    end // sumdelay_pipeline_process3

  assign add_signext_132 = sumdelay_pipeline3[0];
  assign add_signext_133 = sumdelay_pipeline3[1];
  assign add_temp_42 = add_signext_132 + add_signext_133;
  assign sumvector4[0] = (add_temp_42[41] == 1'b0 & add_temp_42[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_42[41] == 1'b1 && add_temp_42[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_42[40:0];

  assign add_signext_134 = sumdelay_pipeline3[2];
  assign add_signext_135 = sumdelay_pipeline3[3];
  assign add_temp_43 = add_signext_134 + add_signext_135;
  assign sumvector4[1] = (add_temp_43[41] == 1'b0 & add_temp_43[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_43[41] == 1'b1 && add_temp_43[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_43[40:0];


  always @ (posedge clk or posedge reset)
    begin: sumdelay_pipeline_process4
      if (reset == 1'b1) begin
        sumdelay_pipeline4[0] <= 0;
        sumdelay_pipeline4[1] <= 0;
      end
      else begin
        if (phase_1 == 1'b1) begin
          sumdelay_pipeline4[0] <= sumvector4[0];
          sumdelay_pipeline4[1] <= sumvector4[1];
        end
      end
    end // sumdelay_pipeline_process4

  assign add_signext_138 = sumdelay_pipeline4[0];
  assign add_signext_139 = sumdelay_pipeline4[1];
  assign add_temp_45 = add_signext_138 + add_signext_139;
  assign sumvector5 = (add_temp_45[41] == 1'b0 & add_temp_45[40] != 1'b0) ? 41'b01111111111111111111111111111111111111111 : 
      (add_temp_45[41] == 1'b1 && add_temp_45[40] != 1'b1) ? 41'b10000000000000000000000000000000000000000 : add_temp_45[40:0];


  assign output_typeconvert = (sumvector5[40] == 1'b0 & sumvector5[39:33] != 7'b0000000) ? 16'b0111111111111111 : 
      (sumvector5[40] == 1'b1 && sumvector5[39:33] != 7'b1111111) ? 16'b1000000000000000 : $signed({1'b0, sumvector5[33:18]} + (sumvector5[40] & |sumvector5[17:0]));

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


        end
      end
    end // ce_delay

  assign ce_gated =  ce_delayline12 & ce_out_reg;

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
endmodule  // hb_down
