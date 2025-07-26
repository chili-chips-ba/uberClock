
module tx_channel # (
    parameter IW = 16, 
    parameter OW = 16, 
    parameter TX_OW = 16,
    parameter NSTAGES = 19, 
    parameter WW = 19,
    parameter PW_I = 19, 
    parameter PW = 23
) (
    input  wire                   sys_clk,
    input  wire                   rst,
    input  wire        [PW_I-1:0] phase_input,
    input  wire signed [IW-1:0]   tx_channel_input_x,
    input  wire signed [IW-1:0]   tx_channel_input_y,
    output wire signed [TX_OW-1:0]  tx_channel_upsampled_x,
    output wire signed [TX_OW-1:0]  tx_channel_upsampled_y,
    output wire signed [TX_OW-1:0]  tx_channel_output,
    output wire                   ce_up
);

    wire signed [TX_OW-1:0] upsampled_x, upsampled_y;
    wire ce_out_up_x, ce_out_up_y; 
    upsamplerFilter up_x (
        .clk        (sys_clk),
        .clk_enable (1'b1),
        .reset      (rst),
        .filter_in  (tx_channel_input_x),
        .filter_out (upsampled_x),
        .ce_out     (ce_out_up_x)
    );
    upsamplerFilter up_y (
        .clk        (sys_clk),
        .clk_enable (1'b1),
        .reset      (rst),
        .filter_in  (tx_channel_input_y),
        .filter_out (upsampled_y),
        .ce_out     (ce_out_up_y)
    );
    // ----------------------------------------------------------------------
    // Upconversion CORDIC
    // ----------------------------------------------------------------------
    wire [PW-1:0] phase = (1 << PW) - (phase_input << (PW - PW_I));
    wire signed [TX_OW:0] x_upconverted, y_upconverted;
    wire up_aux;
    cordic16 #(
        .IW(IW),
        .OW(OW),
        .NSTAGES(NSTAGES),
        .WW(WW),
        .PW(PW)
    ) cordic_up (
        .i_clk   (sys_clk),
        .i_reset (rst),
        .i_ce    (1'b1),
        .i_xval  (upsampled_y),
        .i_yval  (upsampled_x),
        .i_phase (phase),
        .i_aux   (ce_out_up_x),
        .o_xval  (x_upconverted),
        .o_yval  (y_upconverted),
        .o_aux   (up_aux)
    );
    assign ce_up = ce_out_up_y;
    assign tx_channel_output = y_upconverted;
    assign tx_channel_upsampled_x = upsampled_x;
    assign tx_channel_upsampled_y = upsampled_y;

endmodule