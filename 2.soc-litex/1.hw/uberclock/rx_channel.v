`default_nettype none
module rx_channel # (
    parameter IW = 12, 
    parameter OW = 12,
    parameter RX_OW = 16,
    parameter NSTAGES = 15, 
    parameter WW = 15,
    parameter PW = 19
) (
    input  wire                   sys_clk,
    input  wire                   rst,
    input  wire        [PW-1:0]   downconversion_phase_inc,
    input  wire signed [IW-1:0]   rx_channel_input,
    output wire signed [RX_OW-1:0]  rx_channel_output_x,
    output wire signed [RX_OW-1:0]  rx_channel_output_y,
    output wire        [PW-1:0]   downconversion_phase,
    output wire signed [IW -1:0]  rx_downconverted_x,
    output wire signed [IW -1:0]  rx_downconverted_y,
    output wire                   ce_down,
    output wire signed [RX_OW-1:0]  rx_magnitude,
    output wire signed [24:0]  rx_phase
);
   // ----------------------------------------------------------------------
   // Phase accumulator for downconversion
   // ----------------------------------------------------------------------
   reg [PW-1:0] phase_acc_down_reg = {PW{1'b0}};
   always @(posedge sys_clk or posedge rst) begin
       if (rst)
           phase_acc_down_reg <= 0;
       else
           phase_acc_down_reg <= phase_acc_down_reg + downconversion_phase_inc;
   end
   wire signed [IW-1:0] x_downconverted, y_downconverted;
   wire                 down_aux;

   cordic #(
       .IW(IW),
       .OW(OW),
       .NSTAGES(NSTAGES),
       .WW(WW),
       .PW(PW)
   ) cordic_down (
       .i_clk  (sys_clk),
       .i_reset(rst),
       .i_ce   (1'b1),
       .i_xval (0),
       .i_yval (rx_channel_input),
       .i_phase(phase_acc_down_reg),
       .i_aux  (1'b1),
       .o_xval (x_downconverted),
       .o_yval (y_downconverted),
       .o_aux  (down_aux)
   );

    // ----------------------------------------------------------------------
    // Downsampling filters
    // ----------------------------------------------------------------------
    wire signed [RX_OW-1:0]  downsampled_x, downsampled_y;

    wire ce_out_down_x;
    wire ce_out_down_y;

    downsamplerFilter down_x (
        .clk           (sys_clk),
        .clk_enable    (1'b1),
        .reset         (rst),
        .filter_in     (x_downconverted),
        .filter_out    (downsampled_x),
        .ce_out        (ce_out_down_x)
   );
    downsamplerFilter down_y (
        .clk        (sys_clk),
        .clk_enable (1'b1),
        .reset      (rst),
        .filter_in  (y_downconverted),
        .filter_out (downsampled_y),
        .ce_out     (ce_out_down_y)
    );

    wire signed [15:0] magnitude;
    wire [PW-1:0] phase;
    wire aux;
    to_polar #(
       .IW      (16),  // input width
       .OW      (16),  // output magnitude width
       .WW      (26),  // internal working width
       .PW      (25),  // phase accumulator width
       .NSTAGES (22)   // number of CORDIC iterations
    ) arctan (
        .clk(sys_clk),
        .rst(rst),
        .i_ce(ce_out_down_y),      // clock-enable for pipeline
        .i_xval(downsampled_x),
        .i_yval(downsampled_y),
        .i_aux(1'b1),
        .o_mag(magnitude),
        .o_phase(phase),
        .o_aux(aux)
    );


    assign rx_channel_output_x = downsampled_x;
    assign rx_channel_output_y = downsampled_y;
    assign ce_down  = ce_out_down_y;
    assign downconversion_phase = phase_acc_down_reg;
    assign rx_downconverted_x = x_downconverted;
    assign rx_downconverted_y = y_downconverted;
    assign rx_magnitude = magnitude;
    assign rx_phase = phase; 
endmodule
