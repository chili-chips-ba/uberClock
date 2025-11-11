`timescale 1ns / 1ps
module uberclock#(
    parameter IW       = 12,   // CORDIC input width
    parameter OW       = 12,   // CORDIC output width
    parameter NSTAGES  = 20,   // pipeline stages
    parameter WW       = 20,   // working width
    parameter PW       = 24    // phase accumulator width
)(
    input                     sys_clk,
    input                     rst,
    input  [2:0]              final_shift, 
    // ADC (12-bit inputs; AD9238 on J11)
    output                    adc_clk_ch0,  // AD channel 0 sampling clock
    output                    adc_clk_ch1,  // AD channel 1 sampling clock
    input  [11:0]             adc_data_ch0, // AD channel 0 data
    input  [11:0]             adc_data_ch1, // AD channel 1 data

    // DDR‐output DAC (14-bit output; AN9767 on J13)
    output                    da1_clk,         // DA1 clock (DDR‐output)
    output                    da1_wrt,         // DA1 write strobe (DDR‐output)
    output [13:0]             da1_data,        // DA1 14‐bit data bus (DDR‐output)
    output                    da2_clk,         // DA2 clock (DDR‐output)
    output                    da2_wrt,         // DA2 write strobe (DDR‐output)
    output [13:0]             da2_data,        // DA2 14‐bit data bus (DDR‐output)

    //Phase Increment
    input  [PW-1:0]           phase_inc_nco,
    input signed [11:0]       nco_mag, // NCO magnitude from CPU
    input  [PW-1:0]           phase_inc_down_1,
    input  [PW-1:0]           phase_inc_down_2,
    input  [PW-1:0]           phase_inc_down_3,
    input  [PW-1:0]           phase_inc_down_4,
    input  [PW-1:0]           phase_inc_down_5,
    input  [PW-1:0]           phase_inc_cpu,

    input  [1:0]              input_select,  // 0=use ADC, 1=use internal NCO
    input  [1:0]              upsampler_input_mux,
    input  [3:0]              output_select_ch1,
    input  [3:0]              output_select_ch2,
    input  [2:0]              lowspeed_dbg_select,
    input  [31:0]             gain1,
    input  [31:0]             gain2,
    input  [31:0]             gain3,
    input  [31:0]             gain4,
    input  [31:0]             gain5,

    // CPU signals
    output signed [15:0]      downsampled_data_x,
    output signed [15:0]      downsampled_data_y,
    output                    ce_down,
    input signed  [15:0]      upsampler_input_x,
    input signed  [15:0]      upsampler_input_y,

    output signed [15:0]      magnitude,
    output signed [24:0]      phase,
    // Capture control (CSR-driven)
    input              cap_arm,        // write 1 to arm/start a capture
    input      [15:0]  cap_idx,        // index to read back (0..511)
    output reg         cap_done,       // 1 when 512 samples captured
    output     [15:0]  cap_data        // sign-extended read data
    );
    //======================================================================
    // Instantiate the “adc” module
    //======================================================================
    wire [11:0] ad_data_ch0_12;
    wire [11:0] ad_data_ch1_12;
    adc u_adc (
        .sys_clk      (sys_clk),
        .rst_n        (rst),
        // Raw DDR-pinned inputs from the board
        .adc_data_ch0 (adc_data_ch0),
        .adc_data_ch1 (adc_data_ch1),
        // DDR clocks to drive each AD9238 chip
        .adc_clk_ch0  (adc_clk_ch0),
        .adc_clk_ch1  (adc_clk_ch1),
        // 12-bit, single-clock-domain outputs (rising-edge captures)
        .ad_data_ch0  (ad_data_ch0_12),
        .ad_data_ch1  (ad_data_ch1_12)
    );
    // ----------------------------------------------------------------------
    // Phase accumulator for NCO
    // ----------------------------------------------------------------------
    reg [PW-1:0] phase_acc_nco_reg = {PW{1'b0}};
    always @(posedge sys_clk or posedge rst) begin
       if (rst)
           phase_acc_nco_reg <= 0;
       else
           phase_acc_nco_reg <= phase_acc_nco_reg + phase_inc_nco;
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
        .i_xval  (nco_mag),
        .i_yval  (12'sd0),
        .i_phase (phase_acc_nco_reg),
        .i_aux   (1'b1),
        .o_xval  (nco_cos),
        .o_yval  (nco_sin),
        .o_aux   (nco_aux)
    );
    reg signed [11:0] filter_in, filter_in_1;
    always @(posedge sys_clk) begin
        filter_in <= {~ad_data_ch0_12[11], ad_data_ch0_12[10:0]};
        filter_in_1 <= {~ad_data_ch1_12[11], ad_data_ch1_12[10:0]};
    end

    wire signed [IW-1:0] selected_input =
                        (input_select == 2'b00) ? $signed(filter_in) :
                        (input_select == 2'b01) ? $signed(nco_cos)   :
                              $signed(sum[13:2]); // 14->12, sign keep
    //------------------------------------------------------------------------
    // rx1 channel
    //------------------------------------------------------------------------
    wire signed [15:0] downsampled_x1, downsampled_y1;
    wire [PW-1:0] phase_acc_down_reg1;
    wire signed [IW-1:0] x_downconverted1, y_downconverted1;
    wire signed [15:0] rx0_magnitude1;
    wire signed [24:0] rx0_phase1;
    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (24)
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
        .ce_down (ce_down),
        .rx_magnitude (rx0_magnitude1),
        .rx_phase (rx0_phase1)
    );

    // ----------------------------------------------------------------------
    // output to CPU
    // ----------------------------------------------------------------------
    assign  downsampled_data_x = downsampled_x1;
    assign  downsampled_data_y = downsampled_y1;
    assign  magnitude = rx0_magnitude1;
    assign  phase = rx0_phase1;
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

    assign upsampler_in_x1 = (upsampler_input_mux == 2'b00) ? upsampled_gain_x1 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_x : x_cpu_nco << 4;

    assign upsampler_in_y1 = (upsampler_input_mux == 2'b00) ? upsampled_gain_y1 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_y : y_cpu_nco << 4;
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
        .PW_I(24), 
        .PW(23)
    ) tx_1 (
       .sys_clk (sys_clk),
        .rst(rst),
        .phase_input(phase_acc_down_reg1),
        .tx_channel_input_x (upsampler_in_x1),
        .tx_channel_input_y (upsampler_in_y1),
        .tx_channel_output (tx_channel_output1),
        .tx_channel_upsampled_x (upsampled_x1),
        .tx_channel_upsampled_y (upsampled_y1),
        .ce_up(ce_up)
    );



    //------------------------------------------------------------------------
    // rx2 channel
    //------------------------------------------------------------------------
    wire signed [15:0] downsampled_x2, downsampled_y2;
    wire [PW-1:0] phase_acc_down_reg2;
    wire signed [IW-1:0] x_downconverted2, y_downconverted2;
    wire signed [15:0] rx0_magnitude2;
    wire signed [24:0] rx0_phase2;
    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (24)
    ) rx_2 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_2),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x2),
        .rx_channel_output_y (downsampled_y2),
        .downconversion_phase (phase_acc_down_reg2),
        .rx_downconverted_x (x_downconverted2),
        .rx_downconverted_y (y_downconverted2),
        // .ce_down (ce_down),
        .rx_magnitude (rx0_magnitude2),
        .rx_phase (rx0_phase2)
    );

    // ----------------------------------------------------------------------
    // output to CPU
    // ----------------------------------------------------------------------
    // assign  downsampled_data_x = downsampled_x1;
    // assign  downsampled_data_y = downsampled_y1;
    // assign  magnitude = rx0_magnitude1;
    // assign  phase = rx0_phase1;
    // ----------------------------------------------------------------------
    // Upsampling filters
    // ----------------------------------------------------------------------
    wire signed [48:0] y_gained2 , x_gained2 ;
    wire signed [32:0] gain_signed_2 = {1'b0, gain2};
    assign y_gained2 = downsampled_y2 * gain_signed_2;
    assign x_gained2 = downsampled_x2 * gain_signed_2;
    wire signed [15:0] upsampled_gain_x2, upsampled_gain_y2;
    assign upsampled_gain_y2 = y_gained2 >>> 30;
    assign upsampled_gain_x2 = x_gained2 >>> 30;

    wire signed [15:0] upsampler_in_x2, upsampler_in_y2;

    assign upsampler_in_x2 = (upsampler_input_mux == 2'b00) ? upsampled_gain_x2 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_x : x_cpu_nco << 4;

    assign upsampler_in_y2 = (upsampler_input_mux == 2'b00) ? upsampled_gain_y2 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_y : y_cpu_nco << 4;
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
        .PW_I(24), 
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
    wire signed [15:0] rx0_magnitude3;
    wire signed [24:0] rx0_phase3;
    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (24)
    ) rx_3 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_3),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x3),
        .rx_channel_output_y (downsampled_y3),
        .downconversion_phase (phase_acc_down_reg3),
        .rx_downconverted_x (x_downconverted3),
        .rx_downconverted_y (y_downconverted3),
        // .ce_down (ce_down),
        .rx_magnitude (rx0_magnitude3),
        .rx_phase (rx0_phase3)
    );

    // ----------------------------------------------------------------------
    // output to CPU
    // ----------------------------------------------------------------------
    // assign  downsampled_data_x = downsampled_x1;
    // assign  downsampled_data_y = downsampled_y1;
    // assign  magnitude = rx0_magnitude1;
    // assign  phase = rx0_phase1;
    // ----------------------------------------------------------------------
    // Upsampling filters
    // ----------------------------------------------------------------------
    wire signed [48:0] y_gained3 , x_gained3 ;
    wire signed [32:0] gain_signed_3 = {1'b0, gain3};
    assign y_gained3 = downsampled_y3 * gain_signed_3;
    assign x_gained3 = downsampled_x3 * gain_signed_3;
    wire signed [15:0] upsampled_gain_x3, upsampled_gain_y3;
    assign upsampled_gain_y3 = y_gained3 >>> 30;
    assign upsampled_gain_x3 = x_gained3 >>> 30;

    wire signed [15:0] upsampler_in_x3, upsampler_in_y3;

    assign upsampler_in_x3 = (upsampler_input_mux == 2'b00) ? upsampled_gain_x3 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_x : x_cpu_nco << 4;

    assign upsampler_in_y3 = (upsampler_input_mux == 2'b00) ? upsampled_gain_y3 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_y : y_cpu_nco << 4;
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
        .PW_I(24), 
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
    wire signed [15:0] rx0_magnitude4;
    wire signed [24:0] rx0_phase4;
    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (24)
    ) rx_4 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_4),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x4),
        .rx_channel_output_y (downsampled_y4),
        .downconversion_phase (phase_acc_down_reg4),
        .rx_downconverted_x (x_downconverted4),
        .rx_downconverted_y (y_downconverted4),
        // .ce_down (ce_down),
        .rx_magnitude (rx0_magnitude4),
        .rx_phase (rx0_phase4)
    );

    // ----------------------------------------------------------------------
    // output to CPU
    // ----------------------------------------------------------------------
    // assign  downsampled_data_x = downsampled_x1;
    // assign  downsampled_data_y = downsampled_y1;
    // assign  magnitude = rx0_magnitude1;
    // assign  phase = rx0_phase1;
    // ----------------------------------------------------------------------
    // Upsampling filters
    // ----------------------------------------------------------------------
    wire signed [48:0] y_gained4 , x_gained4 ;
    wire signed [32:0] gain_signed_4 = {1'b0, gain4};
    assign y_gained4 = downsampled_y4 * gain_signed_4;
    assign x_gained4 = downsampled_x4 * gain_signed_4;
    wire signed [15:0] upsampled_gain_x4, upsampled_gain_y4;
    assign upsampled_gain_y4 = y_gained4 >>> 30;
    assign upsampled_gain_x4 = x_gained4 >>> 30;

    wire signed [15:0] upsampler_in_x4, upsampler_in_y4;

    assign upsampler_in_x4 = (upsampler_input_mux == 2'b00) ? upsampled_gain_x4 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_x : x_cpu_nco << 4;

    assign upsampler_in_y4 = (upsampler_input_mux == 2'b00) ? upsampled_gain_y4 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_y : y_cpu_nco << 4;
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
        .PW_I(24), 
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
    wire signed [15:0] rx0_magnitude5;
    wire signed [24:0] rx0_phase5;
    rx_channel # (
        .IW (12), 
        .OW (12),
        .RX_OW (16),
        .NSTAGES (15), 
        .WW (15),
        .PW (24)
    ) rx_5 (
        .sys_clk (sys_clk),
        .rst(rst),
        .downconversion_phase_inc (phase_inc_down_5),
        .rx_channel_input (selected_input),
        .rx_channel_output_x (downsampled_x5),
        .rx_channel_output_y (downsampled_y5),
        .downconversion_phase (phase_acc_down_reg5),
        .rx_downconverted_x (x_downconverted5),
        .rx_downconverted_y (y_downconverted5),
        // .ce_down (ce_down),
        .rx_magnitude (rx0_magnitude5),
        .rx_phase (rx0_phase5)
    );

    // ----------------------------------------------------------------------
    // output to CPU
    // ----------------------------------------------------------------------
    // assign  downsampled_data_x = downsampled_x1;
    // assign  downsampled_data_y = downsampled_y1;
    // assign  magnitude = rx0_magnitude1;
    // assign  phase = rx0_phase1;
    // ----------------------------------------------------------------------
    // Upsampling filters
    // ----------------------------------------------------------------------
    wire signed [48:0] y_gained5 , x_gained5 ;
    wire signed [32:0] gain_signed_5 = {1'b0, gain5};
    assign y_gained5 = downsampled_y5 * gain_signed_5;
    assign x_gained5 = downsampled_x5 * gain_signed_5;
    wire signed [15:0] upsampled_gain_x5, upsampled_gain_y5;
    assign upsampled_gain_y5 = y_gained5 >>> 30;
    assign upsampled_gain_x5 = x_gained5 >>> 30;

    wire signed [15:0] upsampler_in_x5, upsampler_in_y5;

    assign upsampler_in_x5 = (upsampler_input_mux == 2'b00) ? upsampled_gain_x5 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_x : x_cpu_nco << 4;

    assign upsampler_in_y5 = (upsampler_input_mux == 2'b00) ? upsampled_gain_y5 :
                            (upsampler_input_mux == 2'b01) ? upsampler_input_y : y_cpu_nco << 4;
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
        .PW_I(24), 
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
    reg  signed [16:0] sum_1, sum_2;   // 16+16 = 17
    reg  signed [16:0] sum_3;          

    always @(posedge sys_clk) begin
        sum_1 <= $signed(tx_channel_output1) + $signed(tx_channel_output2);
        sum_2 <= $signed(tx_channel_output3) + $signed(tx_channel_output4);
        sum_3 <= $signed(tx_channel_output5);
    end

    reg  signed [17:0] sum_4, sum_5;   // 17+17 = 18

    always @(posedge sys_clk) begin
        sum_4 <= $signed(sum_1) + $signed(sum_2);
        sum_5 <= $signed(sum_3);       // to 18 ?
    end

    reg  signed [18:0] sum_final;      // 18+18 = 19
    always @(posedge sys_clk) begin
        sum_final <= $signed(sum_4) + $signed(sum_5);
    end

    // Scaling S (3 for /8)
    // wire [2:0] S = final_shift;  

    // // round-to-nearest before shift
    // wire signed [18:0] sum_rnd = sum_final + $signed(19'sd1 << (S - 1)); 
    // wire signed [18:0] sum_shf = (S == 3'd0) ? sum_final : (sum_rnd >>> S);

    // function automatic [13:0] sat14(input signed [18:0] x);
    //     if      (x >  19'sd8191)  sat14 = 14'sd8191;
    //     else if (x < -19'sd8192)  sat14 = -14'sd8192;
    //     else                      sat14 = x[13:0];
    // endfunction

    // wire signed [13:0] system_output_14 = sat14(sum_shf);
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
    wire [2:0] S = final_shift;  
    wire signed[13:0] sum = (S == 3'd0) ? system_output_14: (system_output_14 << S);

    // ----------------------------------------------------------------------
    // CPU CORDIC NCO
    // ----------------------------------------------------------------------
    wire signed [IW-1:0] x_cpu_nco, y_cpu_nco;
    wire                 down_aux_cpu;
   
   cordic #(
       .IW(IW),
       .OW(OW),
       .NSTAGES(NSTAGES),
       .WW(WW),
       .PW(PW)
   ) cordic_cpu(
       .i_clk  (sys_clk),
       .i_reset(rst),
       .i_ce   (1'b1),
       .i_xval (12'sd200),
       .i_yval (12'sd0),
       .i_phase(phase_acc_cpu_reg),
       .i_aux  (1'b1),
       .o_xval (x_cpu_nco),
       .o_yval (y_cpu_nco),
       .o_aux  (down_aux_cpu)
   );

    reg [PW-1:0] phase_acc_cpu_reg = {PW{1'b0}};
    always @(posedge sys_clk or posedge rst) begin
       if (rst)
           phase_acc_cpu_reg <= 0;
       else begin
            if (ce_down)
                phase_acc_cpu_reg <= phase_acc_cpu_reg + phase_inc_cpu; //52429 for 1kz at 10kHz rate
       end
        
    end


    // ----------------------------------------------------------------------
    // DAC data preparation
// ----------------------------------------------------------------------
    wire [13:0] dac1_data_in =  (output_select_ch1 == 4'b0000) ? upsampled_gain_y1[15:2]: // 19->14:
                                (output_select_ch1 == 4'b0001) ? upsampled_gain_y2[15:2]: // 19->14:
                                (output_select_ch1 == 4'b0010) ? upsampled_gain_y3[15:2]: // 19->14:
                                (output_select_ch1 == 4'b0011) ? upsampled_gain_y4[15:2]: // 19->14:
                                (output_select_ch1 == 4'b0100) ? upsampled_gain_y5[15:2]: // 19->14:
                                (output_select_ch1 == 4'b0101) ? tx_channel_output1[15:2]: // 19->14:
                                (output_select_ch1 == 4'b0110) ? tx_channel_output2[15:2]: // 19->14:
                                (output_select_ch1 == 4'b0111) ? tx_channel_output3[15:2]: // 19->14:
                                (output_select_ch1 == 4'b1000) ? tx_channel_output4[15:2]: // 19->14:
                                (output_select_ch1 == 4'b1001) ? tx_channel_output5[15:2]: // 19->14:
                                (output_select_ch1 == 4'b1010) ? nco_cos << 2:

                                (output_select_ch1 == 4'b1011) ? filter_in << 2:
                                (output_select_ch1 == 4'b1100) ? filter_in_1 << 2:
                                                                 sum_final[18:5]; // 19->14:

    wire [13:0] dac2_data_in =  (output_select_ch2 == 4'b0000) ? upsampled_gain_y1[15:2]: // 19->14:
                                (output_select_ch2 == 4'b0001) ? upsampled_gain_y2[15:2]: // 19->14:
                                (output_select_ch2 == 4'b0010) ? upsampled_gain_y3[15:2]: // 19->14:
                                (output_select_ch2 == 4'b0011) ? upsampled_gain_y4[15:2]: // 19->14:
                                (output_select_ch2 == 4'b0100) ? upsampled_gain_y5[15:2]: // 19->14:
                                (output_select_ch2 == 4'b0101) ? tx_channel_output1[15:2]: // 19->14:
                                (output_select_ch2 == 4'b0110) ? tx_channel_output2[15:2]: // 19->14:
                                (output_select_ch2 == 4'b0111) ? tx_channel_output3[15:2]: // 19->14:
                                (output_select_ch2 == 4'b1000) ? tx_channel_output4[15:2]: // 19->14:
                                (output_select_ch2 == 4'b1001) ? tx_channel_output5[15:2]: // 19->14:
                                (output_select_ch2 == 4'b1010) ? nco_cos << 2:

                                (output_select_ch2 == 4'b1011) ? filter_in << 2:
                                (output_select_ch2 == 4'b1100) ? filter_in_1 << 2:
                                                                 sum_final[18:5]; // 19->14:                             

    reg  [13:0] dac1_data_reg, dac2_data_reg;
    always @(posedge sys_clk) begin
        dac1_data_reg <= dac1_data_in + 14'd8192;
        dac2_data_reg <= dac2_data_in + 14'd8192;
    end
     // ----------------------------------------------------------------------
    // DDR-output DAC module
    // ----------------------------------------------------------------------
    dac u_dac (
        .sys_clk  (sys_clk),
        .rst_n    (rst),
        .data1    (dac1_data_reg),
        .data2    (dac2_data_reg),
        .da1_clk  (da1_clk),
        .da1_wrt  (da1_wrt),
        .da1_data (da1_data),
        .da2_clk  (da2_clk),
        .da2_wrt  (da2_wrt),
        .da2_data (da2_data)
    );
    // ----------------------------------------------------------------------
    // Capture 512 samples of full 16-bit upsampled_gain_y1 on ce_down
    // ----------------------------------------------------------------------
    wire signed [15:0] lowspeed_debug_signal;

    assign lowspeed_debug_signal =
        (lowspeed_dbg_select == 3'b000) ? upsampled_gain_y1 :
        (lowspeed_dbg_select == 3'b001) ? upsampled_gain_y2 :
        (lowspeed_dbg_select == 3'b010) ? upsampled_gain_y3 :
        (lowspeed_dbg_select == 3'b011) ? upsampled_gain_y4 :
        (lowspeed_dbg_select == 3'b100) ? upsampled_gain_y5 : 16'sd0;

    reg cap_arm_q;
    wire cap_arm_pulse = cap_arm & ~cap_arm_q;
    always @(posedge sys_clk) cap_arm_q <= cap_arm;

    reg         capturing;
    reg  [10:0]  wr_ptr;                 // 0..511
    reg  [15:0] cap_mem [0:2047];        // 512 x 16-bit

    always @(posedge sys_clk or posedge rst) begin
        if (rst) begin
            capturing <= 1'b0;
            cap_done  <= 1'b0;
            wr_ptr    <= 11'd0;
        end else begin
            // Arm/start on rising edge
            if (cap_arm_pulse) begin
                capturing <= 1'b1;
                cap_done  <= 1'b0;
                wr_ptr    <= 11'd0;
            end

            // Capture on ce_down
            if (capturing && ce_down) begin
                cap_mem[wr_ptr] <= lowspeed_debug_signal;  // FULL 16-bit sample
                if (wr_ptr == 11'd2047) begin
                    capturing <= 1'b0;
                    cap_done  <= 1'b1;
                end else begin
                    wr_ptr <= wr_ptr + 11'd1;
                end
            end
        end
    end

    wire [10:0] rd_idx = cap_idx[10:0];
    assign cap_data = cap_mem[rd_idx];

endmodule
