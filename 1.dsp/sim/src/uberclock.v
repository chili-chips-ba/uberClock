// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

module uberclock#(
    parameter IW       = 12,   // CORDIC input width
    parameter OW       = 12,   // CORDIC output width
    parameter NSTAGES  = 15,   // pipeline stages
    parameter WW       = 15,   // working width
    parameter PW       = 19    // phase accumulator width
)(
    input  wire        sys_clk_p,   // differential system clock
    input  wire        sys_clk_n,
//    input                     sys_clk,
    input              rst,
    input  [2:0]  final_shift, // 0..7 is plenty; use 2 or 3 typically

    
    input  [PW-1:0]           phase_inc_down_1,
    input  [PW-1:0]           phase_inc_down_2,
    input  [PW-1:0]           phase_inc_down_3,
    input  [PW-1:0]           phase_inc_down_4,
    input  [PW-1:0]           phase_inc_down_5,

    input  [31:0]             gain1,
    input  [31:0]             gain2,
    input  [31:0]             gain3,
    input  [31:0]             gain4,
    input  [31:0]             gain5
    );

    wire                            sys_clk_200, sys_clk_i,sys_clk ;                 //single end clock
   IBUFDS sys_clk_ibufgds
   (
      .O                              (sys_clk_200             ),
      .I                              (sys_clk_p               ),
      .IB                             (sys_clk_n               )
   );
  wire clk65i, locked, clk65;
  
  adc_pll adc_pll_m0  (.clk_out1(sys_clk_i), .reset(), .locked(locked), .clk_in1(sys_clk_200));
  BUFG   bufg_clk65   (.I(sys_clk_i), .O(sys_clk));

    // ----------------------------------------------------------------------
    // Phase accumulator for NCO
    // ----------------------------------------------------------------------
    reg [31:0] phase_acc_nco_reg = {31{1'b0}};
    always @(posedge sys_clk or posedge rst) begin
       if (rst)
           phase_acc_nco_reg <= 0;
       else
           phase_acc_nco_reg <= phase_acc_nco_reg + 660764199;
    end
    wire signed [IW-1:0] nco_cos, nco_sin;
    wire                 nco_aux;
    cordic #(
       .IW(IW),
       .OW(OW),
       .NSTAGES(NSTAGES),
       .WW(WW),
       .PW(PW)
    ) cordic_nco (
        .i_clk   (sys_clk),
        .i_reset (rst),
        .i_ce    (1'b1),
        .i_xval  (12'sd2047),
        .i_yval  (12'sd0),
        .i_phase (phase_acc_nco_reg[31:13]),
        .i_aux   (1'b1),
        .o_xval  (nco_cos),
        .o_yval  (nco_sin),
        .o_aux   (nco_aux)
    );


    wire signed [IW-1:0] selected_input = nco_aux ? $signed(nco_cos) : {IW{1'b0}};

    //------------------------------------------------------------------------
    // rx1 channel
    //------------------------------------------------------------------------
    wire signed [15:0] downsampled_x1, downsampled_y1;
    wire [PW-1:0] phase_acc_down_reg1;
    wire signed [IW-1:0] x_downconverted1, y_downconverted1;

    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (19)
    ) rx_1 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_1),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x1),
        .rx_channel_output_y (downsampled_y1),
        .downconversion_phase (phase_acc_down_reg1),
        .rx_downconverted_x (x_downconverted1),
        .rx_downconverted_y (y_downconverted1),
        .ce_down (ce_down)
    );



    // ----------------------------------------------------------------------
    // Upsampling filters
    // ----------------------------------------------------------------------
    wire signed [48:0] y_gained1 , x_gained1 ;
    wire signed [32:0] gain_signed_1 = {1'b0, gain1};
    assign y_gained1 = downsampled_y1 * gain_signed_1;
    assign x_gained1 = downsampled_x1 * gain_signed_1;
    wire signed [15:0] upsampled_gain_x1, upsampled_gain_y1;
    assign upsampled_gain_y1 = y_gained1 >>> 30;
    assign upsampled_gain_x1 = x_gained1 >>> 30;

    wire signed [15:0] upsampler_in_x1, upsampler_in_y1;

    assign upsampler_in_x1 =  upsampled_gain_x1 ;

    assign upsampler_in_y1 = upsampled_gain_y1 ;
    //--------------------------------------------------------
    // tx1 channel
    //--------------------------------------------------------
    wire ce_up;
    wire signed [15:0] tx_channel_output1;
    wire signed [15:0] upsampled_x1, upsampled_y1;
    tx_channel # (
        .IW(16), 
        .OW(16), 
        .TX_OW(16),
        .NSTAGES(19), 
        .WW(19),
        .PW_I(19), 
        .PW(23)
    ) tx_1 (
       .sys_clk (sys_clk),
        .rst(rst),
        .phase_input(phase_acc_down_reg1),
        .tx_channel_input_x (upsampler_in_x1),
        .tx_channel_input_y (upsampler_in_y1),
        .tx_channel_output (tx_channel_output1),
        .tx_channel_upsampled_x (upsampled_x1),
        .tx_channel_upsampled_y (upsampled_y1)
    );



    //------------------------------------------------------------------------
    // rx2 channel
    //------------------------------------------------------------------------
    wire signed [15:0] downsampled_x2, downsampled_y2;
    wire [PW-1:0] phase_acc_down_reg2;
    wire signed [IW-1:0] x_downconverted2, y_downconverted2;

    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (19)
    ) rx_2 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_2),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x2),
        .rx_channel_output_y (downsampled_y2),
        .downconversion_phase (phase_acc_down_reg2),
        .rx_downconverted_x (x_downconverted2),
        .rx_downconverted_y (y_downconverted2)
    );

    wire signed [48:0] y_gained2 , x_gained2 ;
    wire signed [32:0] gain_signed_2 = {1'b0, gain2};
    assign y_gained2 = downsampled_y2 * gain_signed_2;
    assign x_gained2 = downsampled_x2 * gain_signed_2;
    wire signed [15:0] upsampled_gain_x2, upsampled_gain_y2;
    assign upsampled_gain_y2 = y_gained2 >>> 30;
    assign upsampled_gain_x2 = x_gained2 >>> 30;

    wire signed [15:0] upsampler_in_x2, upsampler_in_y2;

    assign upsampler_in_x2 = upsampled_gain_x2 ;

    assign upsampler_in_y2 = upsampled_gain_y2 ;
    //--------------------------------------------------------
    // tx2 channel
    //--------------------------------------------------------
    wire signed [15:0] tx_channel_output2;
    wire signed [15:0] upsampled_x2, upsampled_y2;
    tx_channel # (
        .IW(16), 
        .OW(16), 
        .TX_OW(16),
        .NSTAGES(19), 
        .WW(19),
        .PW_I(19), 
        .PW(23)
    ) tx_2 (
        .sys_clk (sys_clk),
        .rst(rst),
        .phase_input(phase_acc_down_reg2),
        .tx_channel_input_x (upsampler_in_x2),
        .tx_channel_input_y (upsampler_in_y2),
        .tx_channel_output (tx_channel_output2),
        .tx_channel_upsampled_x (upsampled_x2),
        .tx_channel_upsampled_y (upsampled_y2)
    );



    //------------------------------------------------------------------------
    // rx3 channel
    //------------------------------------------------------------------------
    wire signed [15:0] downsampled_x3, downsampled_y3;
    wire [PW-1:0] phase_acc_down_reg3;
    wire signed [IW-1:0] x_downconverted3, y_downconverted3;

    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (19)
    ) rx_3 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_3),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x3),
        .rx_channel_output_y (downsampled_y3),
        .downconversion_phase (phase_acc_down_reg3),
        .rx_downconverted_x (x_downconverted3),
        .rx_downconverted_y (y_downconverted3)
    );

    wire signed [48:0] y_gained3 , x_gained3 ;
    wire signed [32:0] gain_signed_3 = {1'b0, gain3};
    assign y_gained3 = downsampled_y3 * gain_signed_3;
    assign x_gained3 = downsampled_x3 * gain_signed_3;
    wire signed [15:0] upsampled_gain_x3, upsampled_gain_y3;
    assign upsampled_gain_y3 = y_gained3 >>> 30;
    assign upsampled_gain_x3 = x_gained3 >>> 30;

    wire signed [15:0] upsampler_in_x3, upsampler_in_y3;

    assign upsampler_in_x3 = upsampled_gain_x3 ;

    assign upsampler_in_y3 = upsampled_gain_y3 ;
    //--------------------------------------------------------
    // tx3 channel
    //--------------------------------------------------------
    wire signed [15:0] tx_channel_output3;
    wire signed [15:0] upsampled_x3, upsampled_y3;
    tx_channel # (
        .IW(16), 
        .OW(16), 
        .TX_OW(16),
        .NSTAGES(19), 
        .WW(19),
        .PW_I(19), 
        .PW(23)
    ) tx_3 (
        .sys_clk (sys_clk),
        .rst(rst),
        .phase_input(phase_acc_down_reg3),
        .tx_channel_input_x (upsampler_in_x3),
        .tx_channel_input_y (upsampler_in_y3),
        .tx_channel_output (tx_channel_output3),
        .tx_channel_upsampled_x (upsampled_x3),
        .tx_channel_upsampled_y (upsampled_y3)
    );


    //------------------------------------------------------------------------
    // rx4 channel
    //------------------------------------------------------------------------
    wire signed [15:0] downsampled_x4, downsampled_y4;
    wire [PW-1:0] phase_acc_down_reg4;
    wire signed [IW-1:0] x_downconverted4, y_downconverted4;
    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (19)
    ) rx_4 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_4),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x4),
        .rx_channel_output_y (downsampled_y4),
        .downconversion_phase (phase_acc_down_reg4),
        .rx_downconverted_x (x_downconverted4),
        .rx_downconverted_y (y_downconverted4)
    );


    wire signed [48:0] y_gained4 , x_gained4 ;
    wire signed [32:0] gain_signed_4 = {1'b0, gain4};
    assign y_gained4 = downsampled_y4 * gain_signed_4;
    assign x_gained4 = downsampled_x4 * gain_signed_4;
    wire signed [15:0] upsampled_gain_x4, upsampled_gain_y4;
    assign upsampled_gain_y4 = y_gained4 >>> 30;
    assign upsampled_gain_x4 = x_gained4 >>> 30;

    wire signed [15:0] upsampler_in_x4, upsampler_in_y4;

    assign upsampler_in_x4 = upsampled_gain_x4 ;

    assign upsampler_in_y4 =  upsampled_gain_y4 ;
    //--------------------------------------------------------
    // tx4 channel
    //--------------------------------------------------------
    wire signed [15:0] tx_channel_output4;
    wire signed [15:0] upsampled_x4, upsampled_y4;
    tx_channel # (
        .IW(16), 
        .OW(16), 
        .TX_OW(16),
        .NSTAGES(19), 
        .WW(19),
        .PW_I(19), 
        .PW(23)
    ) tx_4 (
        .sys_clk (sys_clk),
        .rst(rst),
        .phase_input(phase_acc_down_reg4),
        .tx_channel_input_x (upsampler_in_x4),
        .tx_channel_input_y (upsampler_in_y4),
        .tx_channel_output (tx_channel_output4),
        .tx_channel_upsampled_x (upsampled_x4),
        .tx_channel_upsampled_y (upsampled_y4)
    );


    //------------------------------------------------------------------------
    // rx5 channel
    //------------------------------------------------------------------------
    wire signed [15:0] downsampled_x5, downsampled_y5;
    wire [PW-1:0] phase_acc_down_reg5;
    wire signed [IW-1:0] x_downconverted5, y_downconverted5;
    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (19)
    ) rx_5 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_5),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x5),
        .rx_channel_output_y (downsampled_y5),
        .downconversion_phase (phase_acc_down_reg5),
        .rx_downconverted_x (x_downconverted5),
        .rx_downconverted_y (y_downconverted5)
    );


    wire signed [48:0] y_gained5 , x_gained5 ;
    wire signed [32:0] gain_signed_5 = {1'b0, gain5};
    assign y_gained5 = downsampled_y5 * gain_signed_5;
    assign x_gained5 = downsampled_x5 * gain_signed_5;
    wire signed [15:0] upsampled_gain_x5, upsampled_gain_y5;
    assign upsampled_gain_y5 = y_gained5 >>> 30;
    assign upsampled_gain_x5 = x_gained5 >>> 30;

    wire signed [15:0] upsampler_in_x5, upsampler_in_y5;

    assign upsampler_in_x5 =  upsampled_gain_x5 ;
                            

    assign upsampler_in_y5 = upsampled_gain_y5 ;
    //--------------------------------------------------------
    // tx5 channel
    //--------------------------------------------------------
    wire signed [15:0] tx_channel_output5;
    wire signed [15:0] upsampled_x5, upsampled_y5;
    tx_channel # (
        .IW(16), 
        .OW(16), 
        .TX_OW(16),
        .NSTAGES(19), 
        .WW(19),
        .PW_I(19), 
        .PW(23)
    ) tx_5 (
        .sys_clk (sys_clk),
        .rst(rst),
        .phase_input(phase_acc_down_reg5),
        .tx_channel_input_x (upsampler_in_x5),
        .tx_channel_input_y (upsampler_in_y5),
        .tx_channel_output (tx_channel_output5),
        .tx_channel_upsampled_x (upsampled_x5),
        .tx_channel_upsampled_y (upsampled_y5)
    );


    // ----------------------------------------------------------------------
    // Sum tree
    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
// Sum tree (consistent widths + signed)
// ----------------------------------------------------------------------
reg  signed [16:0] sum_1, sum_2;   // 16+16 -> 17
reg  signed [16:0] sum_3;          // align to 17

always @(posedge sys_clk) begin
    sum_1 <= $signed(tx_channel_output1) + $signed(tx_channel_output2);
    sum_2 <= $signed(tx_channel_output3) + $signed(tx_channel_output4);
    sum_3 <= $signed(tx_channel_output5);
end

reg  signed [17:0] sum_4, sum_5;   // 17+17 -> 18

always @(posedge sys_clk) begin
    sum_4 <= $signed(sum_1) + $signed(sum_2);
    sum_5 <= $signed(sum_3);       // widen to 18
end

reg  signed [18:0] sum_final;      // 18+18 -> 19
always @(posedge sys_clk) begin
    sum_final <= $signed(sum_4) + $signed(sum_5);
end

//// Choose scaling S (3 for /8, 4 for /16, etc.)
//// -------- Final scale/round/saturate to 14-bit signed for DAC/feedback --------
//wire [2:0] S = final_shift;  // make it runtime-adjustable

//// round-to-nearest before shift
//wire signed [18:0] sum_rnd = sum_final + $signed(19'sd1 << (S - 1)); // safe when S!=0; see guard below
//wire signed [18:0] sum_shf = (S == 3'd0) ? sum_final : (sum_rnd >>> S);

//// 14-bit signed saturation: -8192..+8191
//function automatic [13:0] sat14(input signed [18:0] x);
//    if      (x >  19'sd8191)  sat14 = 14'sd8191;
//    else if (x < -19'sd8192)  sat14 = -14'sd8192;
//    else                      sat14 = x[13:0];
//endfunction

//wire signed [13:0] system_output_14 = sat14(sum_shf);

wire signed [18:0] pre_sum_val, sum_pre_round;

assign sum_pre_round = sum_final;

reg signed  [13:0] system_output_14;

assign pre_sum_val = sum_pre_round + $signed({ {(14){1'b0}},
                                     sum_pre_round[5],
                                     {(4){~sum_pre_round[5]}}});
                                     
always @(posedge sys_clk) begin
   if (rst) begin
     system_output_14 <= 0;

   end else begin
     system_output_14 <= sum_pre_round [18:5]; 
   end
 end                                



//assign	w_convergent = i_data[(IWID-1):0]
//			+ { {(OWID){1'b0}},
//				i_data[(IWID-OWID)],
//				{(IWID-OWID-1){!i_data[(IWID-OWID)]}}};
//always @(posedge i_clk)
//	o_convergent <= w_convergent[(IWID-1):(IWID-OWID)];

endmodule
