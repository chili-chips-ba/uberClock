`timescale 1ns / 1ps
`default_nettype none
module uberclock #(
    parameter IW      = 12,
    parameter OW      = 12,
    parameter NSTAGES = 15,
    parameter WW      = 15,
    parameter PW      = 19
)(
    // Clocks/Reset
    input  wire                    sys_clk,
    input  wire                    rst,

    // Controls
    input  wire        [2:0]       final_shift,

    // ADC (12-bit)
    output wire                    adc_clk_ch0,
    output wire                    adc_clk_ch1,
    input  wire        [11:0]      adc_data_ch0,
    input  wire        [11:0]      adc_data_ch1,

    // DAC (14-bit DDR)
    output wire                    da1_clk,
    output wire                    da1_wrt,
    output wire        [13:0]      da1_data,
    output wire                    da2_clk,
    output wire                    da2_wrt,
    output wire        [13:0]      da2_data,

    // Phase increments
    input  wire        [PW-1:0]    phase_inc_nco,
    input  wire        [PW-1:0]    phase_inc_down_1,
    input  wire        [PW-1:0]    phase_inc_down_2,
    input  wire        [PW-1:0]    phase_inc_down_3,
    input  wire        [PW-1:0]    phase_inc_down_4,
    input  wire        [PW-1:0]    phase_inc_down_5,
    input  wire        [PW-1:0]    phase_inc_cpu,

    // Muxes / gains
    input  wire        [1:0]       input_select,
    input  wire        [1:0]       upsampler_input_mux,
    input  wire        [1:0]       output_select_ch1,
    input  wire        [1:0]       output_select_ch2,
    input  wire        [31:0]      gain1,
    input  wire        [31:0]      gain2,
    input  wire        [31:0]      gain3,
    input  wire        [31:0]      gain4,
    input  wire        [31:0]      gain5,

    // CPU-visible signals
    output wire signed [15:0]      downsampled_data_x,
    output wire signed [15:0]      downsampled_data_y,
    output wire                    ce_down,
    input  wire signed [15:0]      upsampler_input_x,
    input  wire signed [15:0]      upsampler_input_y,
    output wire signed [15:0]      magnitude,
    output wire signed [24:0]      phase,

    // Debug
    output wire        [IW-1:0]    dbg_nco_cos,
    output wire        [IW-1:0]    dbg_nco_sin,
    output wire        [PW-1:0]    dbg_phase_acc_down,
    output wire        [11:0]      dbg_x_downconverted,
    output wire        [11:0]      dbg_y_downconverted,
    output wire        [15:0]      dbg_downsampled_x,
    output wire        [15:0]      dbg_downsampled_y,
    output wire        [15:0]      dbg_upsampled_x,
    output wire        [15:0]      dbg_upsampled_y,
    output wire        [IW-1:0]    dbg_selected_input,
    output wire        [IW-1:0]    cap_selected_input
    // output wire  [22:0]         dbg_phase_inv,
    // output wire [15:0]          dbg_x_upconverted,
    // output wire [15:0]          dbg_y_upconverted,
    // output wire                 dbg_ce_down_x,
    // output wire                 dbg_ce_down_y,
    // output wire                 dbg_ce_up_x,
    // output wire                 dbg_cic_ce_x,
    // output wire                 dbg_comp_ce_x,
    // output wire                 dbg_hb_ce_x,
    // output wire  signed [11:0]  dbg_cic_out_x,
    // output wire  signed [15:0]  dbg_comp_out_x
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
        .i_xval  (12'sd1000),
        .i_yval  (12'sd0),
        .i_phase (phase_acc_nco_reg),
        .i_aux   (1'b1),
        .o_xval  (nco_cos),
        .o_yval  (nco_sin),
        .o_aux   (nco_aux)
    );
    reg signed [11:0] filter_in;
    always @(posedge sys_clk) begin
        filter_in <= {~ad_data_ch0_12[11], ad_data_ch0_12[10:0]};
    end

    // ----------------------------------------------------------------------
    // Test ramp generator (uses phase_inc_cpu as step)
    // - 12-bit bipolar saw: [-2048, +2047]
    // ----------------------------------------------------------------------
    reg  [15:0] ramp_q;
    always @(posedge sys_clk or posedge rst) begin
        if (rst) ramp_q <= 16'd0;
        else     ramp_q <= ramp_q + {4'd0, phase_inc_cpu[11:0]}; // step = phase_inc_cpu[11:0]
    end

    // Make a 12-bit signed saw by centering the unsigned ramp around 0
    wire signed [IW-1:0] test_ramp12 = $signed({1'b0, ramp_q[15:4]}) - 12'sd2048;


wire signed [IW-1:0] selected_input =
                        (input_select == 2'b00) ? $signed(filter_in) :
                        (input_select == 2'b01) ? $signed(nco_cos)   :
                        (input_select == 2'b10) ? $signed(system_output_14[13:2]) :
                                                   test_ramp12;  // 3 = TEST
    reg signed [IW-1:0] selected_input_q;
    always @(posedge sys_clk) begin
        selected_input_q <= selected_input;
    end

    assign cap_selected_input = selected_input_q;


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
    wire signed [15:0] rx0_magnitude3;
    wire signed [24:0] rx0_phase3;
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
    wire signed [15:0] rx0_magnitude4;
    wire signed [24:0] rx0_phase4;
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
    wire signed [15:0] rx0_magnitude5;
    wire signed [24:0] rx0_phase5;
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

// Choose scaling S (3 for /8, 4 for /16, etc.)
// -------- Final scale/round/saturate to 14-bit signed for DAC/feedback --------
wire [2:0] S = final_shift;  // make it runtime-adjustable

// round-to-nearest before shift
wire signed [18:0] sum_rnd = sum_final + $signed(19'sd1 << (S - 1)); // safe when S!=0; see guard below
wire signed [18:0] sum_shf = (S == 3'd0) ? sum_final : (sum_rnd >>> S);

// 14-bit signed saturation: -8192..+8191
function automatic [13:0] sat14(input signed [18:0] x);
    if      (x >  19'sd8191)  sat14 = 14'sd8191;
    else if (x < -19'sd8192)  sat14 = -14'sd8192;
    else                      sat14 = x[13:0];
endfunction

wire signed [13:0] system_output_14 = sat14(sum_shf);



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
       .i_xval (12'sd1000),
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
    wire [13:0] dac1_data_in =   (output_select_ch1 == 2'b00) ? downsampled_y1[15:2]:
                                 (output_select_ch1 == 2'b01) ? downsampled_y5[15:2]:
                                 (output_select_ch1 == 2'b10) ? nco_cos << 2:
                                                                system_output_14; 

    wire [13:0] dac2_data_in =   (output_select_ch2 == 2'b00) ? downsampled_y2[15:2]:
                                 (output_select_ch2 == 2'b01) ? downsampled_y3[15:2]:
                                 (output_select_ch2 == 2'b10) ? downsampled_y4[15:2]:
                                                                tx_channel_output1[15:2];
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
    // Debug signal assignments
    // ----------------------------------------------------------------------
    assign dbg_nco_cos         = nco_cos;
    assign dbg_nco_sin         = nco_sin;
    assign dbg_phase_acc_down  = phase_acc_down_reg1;
    assign dbg_x_downconverted = x_downconverted1;
    assign dbg_y_downconverted = y_downconverted1;
    assign dbg_downsampled_x   = downsampled_x1;
    assign dbg_downsampled_y   = downsampled_y1;
    assign dbg_upsampled_x     = upsampled_x1;
    assign dbg_upsampled_y     = upsampled_y1;
    assign dbg_selected_input  = selected_input;
    // assign dbg_phase_inv       = phase_inv;
    // assign dbg_x_upconverted   = x_upconverted;
    // assign dbg_y_upconverted   = tx_channel_output ;
    // assign dbg_ce_down_x       = ce_out_down_x;
    // assign dbg_ce_down_y       = ce_out_down_y;
    // assign dbg_ce_up_x         = ce_out_up_x;
    // assign dbg_cic_ce_x        = cic_ce_x;
    // assign dbg_comp_ce_x       = comp_ce_x;
    // assign dbg_hb_ce_x         = hb_ce_x;
    // assign dbg_cic_out_x       = cic_out_x;
    // assign dbg_comp_out_x      = comp_out_x;
endmodule

`default_nettype wire
