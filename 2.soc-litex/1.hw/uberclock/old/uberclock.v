`timescale 1ns / 1ps
`default_nettype none

module uberclock #(
    parameter integer N_CHANNELS = 5,
    parameter integer IW         = 12,
    parameter integer OW         = 12,
    parameter integer NSTAGES    = 15,
    parameter integer WW         = 15,
    parameter integer PW         = 19
)(
    input  wire                          clk,
    input  wire                          rst,
    input  wire [N_CHANNELS-1:0][11:0]   adc_data,
    input  wire [PW-1:0]                 phase_inc_nco,
    input  wire [N_CHANNELS-1:0][PW-1:0] phase_inc_down,
    input  wire [N_CHANNELS-1:0][31:0]   gain,
    input  wire                          input_select,
    input  wire [1:0]                    output_select,

    output wire [13:0]                   dac_data,
    output wire                          dac_clk,
    output wire                          dac_wrt
);

    // Internal wire arrays
    wire signed [15:0] y_upconverted [N_CHANNELS-1:0];
    wire [13:0]        dac_data_ch   [N_CHANNELS-1:0];
    wire               dac_clk_ch    [N_CHANNELS-1:0];
    wire               dac_wrt_ch    [N_CHANNELS-1:0];

    genvar i;
    generate
        for (i = 0; i < N_CHANNELS; i = i + 1) begin : gen_channels
            dsp_channel #(
                .IW(IW), .OW(OW), .NSTAGES(NSTAGES),
                .WW(WW), .PW(PW)
            ) u_channel (
                .clk(clk),
                .rst(rst),
                .adc_data(adc_data[i]),
                .phase_inc_nco(phase_inc_nco),
                .phase_inc_down(phase_inc_down[i]),
                .gain(gain[i]),
                .input_select(input_select),
                .output_select(output_select),
                .dac_data(dac_data_ch[i]),
                .dac_clk(dac_clk_ch[i]),
                .dac_wrt(dac_wrt_ch[i])
            );
        end
    endgenerate

    // Sum y_upconverted outputs
    wire signed [18:0] y_sum =
        $signed(dac_data_ch[0]) +
        $signed(dac_data_ch[1]) +
        $signed(dac_data_ch[2]) +
        $signed(dac_data_ch[3]) +
        $signed(dac_data_ch[4]);

    reg [13:0] dac_data_reg;
    always @(posedge clk) begin
        dac_data_reg <= y_sum[18:5] + 14'd8192;
    end

    assign dac_data = dac_data_reg;
    assign dac_clk  = clk;
    assign dac_wrt  = 1'b1;

endmodule

`default_nettype wire
