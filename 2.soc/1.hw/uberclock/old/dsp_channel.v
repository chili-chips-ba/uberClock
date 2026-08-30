`timescale 1ns / 1ps
`default_nettype none

module dsp_channel #(
    parameter integer IW       = 12,
    parameter integer OW       = 12,
    parameter integer NSTAGES  = 15,
    parameter integer WW       = 15,
    parameter integer PW       = 19
)(
    input  wire                     clk,
    input  wire                     rst,
    input  wire [11:0]              input_data,
    input  wire [31:0]              gain,

    output wire [13:0]              dac_data,
    output wire                     dac_clk,
    output wire                     dac_wrt
);



    // -----------------------------
    // Downconversion CORDIC
    // -----------------------------
    reg [PW-1:0] phase_acc_down;
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            phase_acc_down <= 0;;
        end else begin
            phase_acc_down <= phase_acc_down + phase_inc_down;
        end

    end

    wire signed [IW-1:0] x_downconverted, y_downconverted;
    wire                 aux_downconverted;

   cordic #(
       .IW(IW),
       .OW(OW),
       .NSTAGES(NSTAGES),
       .WW(WW),
       .PW(PW)
   ) cordic_down (
       .i_clk  (clk),
       .i_reset(rst),
       .i_ce   (1'b1),
       .i_xval (0),
       .i_yval (input_data),
       .i_phase(phase_acc_down),
       .i_aux  (1'b1),
       .o_xval (x_downconverted),
       .o_yval (y_downconverted),
       .o_aux  (aux_downconverted)
   );

    // -----------------------------
    // Downsampler Filters
    // -----------------------------
    wire signed [15:0] x_downsampled, y_downsampled;
    wire ce_downsampled_x, ce_downsampled_y;

    downsamplerFilter down_x_filt (
        .clk(clk),
        .clk_enable(1'b1),
        .reset(rst),
        .filter_in(x_downconverted),
        .filter_out(x_downsampled),
        .ce_out(ce_downsampled_x)
    );

    downsamplerFilter down_y_filt (
        .clk(clk),
        .clk_enable(1'b1),
        .reset(rst),
        .filter_in(y_downconverted),
        .filter_out(y_downsampled),
        .ce_out(ce_downsampled_y)
    );


    // ----------------------------------------------------------------------
    // Gain
    // ----------------------------------------------------------------------
    wire signed [48:0] gain_x_full = x_downsampled * {1'b0, gain};
    wire signed [48:0] gain_y_full = y_downsampled * {1'b0, gain};
    wire signed [15:0] gain_x = gain_x_full >>> 30;
    wire signed [15:0] gain_y = gain_y_full >>> 30;


    // ----------------------------------------------------------------------
    // Upsampling filters
    // ----------------------------------------------------------------------

    wire signed [15:0] x_upsampled, y_upsampled;
    wire ce_upsampled_x, ce_upsampled_y;

    upsamplerFilter up_x_filt (
        .clk(clk), .clk_enable(1'b1), .reset(rst),
        .filter_in(gain_x), .filter_out(x_upsampled), .ce_out(ce_upsampled_x)
    );

    upsamplerFilter up_y_filt (
        .clk(clk), .clk_enable(1'b1), .reset(rst),
        .filter_in(gain_y), .filter_out(y_upsampled), .ce_out(ce_upsampled_y)
    );


    // ----------------------------------------------------------------------
    // Upconversion CORDIC
    // ----------------------------------------------------------------------
    wire [PW+4:0] phase_inv = (1 << (PW + 4)) - (phase_acc_down << 4);
    wire signed [15:0] x_upconverted, y_upconverted;
    wire aux_upconverted;

    cordic16 #(
        .IW(16), .OW(16), .NSTAGES(19), .WW(19), .PW(PW + 5)
    ) u_up (
        .i_clk(clk), .i_reset(rst), .i_ce(1'b1),
        .i_xval(y_upsampled), .i_yval(x_upsampled),
        .i_phase(phase_inv[PW+4:0]), .i_aux(ce_upsampled_x),
        .o_xval(x_upconverted), .o_yval(y_upconverted), .o_aux(aux_upconverted)
    );

    // ----------------------------------------------------------------------
    // DAC data preparation
    // ----------------------------------------------------------------------

    wire [13:0] dac_input_data =
        (output_select == 2'b00) ? y_downsampled[15:2] :
        (output_select == 2'b01) ? gain_y[15:2] :
        (output_select == 2'b10) ? y_downconverted << 2 :
                                   y_upconverted[15:2];
    reg [13:0] dac_data_reg;
    always @(posedge clk)
        dac_data_reg <= dac_input_data + 14'd8192;

    assign dac_data = dac_data_reg;
    assign dac_clk  = clk;
    assign dac_wrt  = 1'b1;

endmodule
`default_nettype wire
