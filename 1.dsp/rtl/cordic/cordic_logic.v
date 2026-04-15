// cordic_logic.v
// Encapsulates phase accumulator, CORDIC pipeline, and scaling (no mux)
`timescale 1 ns / 1 ns
`default_nettype none
module cordic_logic #(
    parameter IW       = 12,   // CORDIC input width
    parameter OW       = 12,   // CORDIC output width
    parameter NSTAGES  = 15,   // pipeline stages
    parameter WW       = 15,   // working width
    parameter PW       = 19    // phase accumulator width
)(
    input  wire                sys_clk,
    input  wire                rst_n,
    input  wire [PW-1:0]       phase_inc,
    output wire [13:0]         sin_out,    // final 14-bit unsigned sine
    output wire [13:0]         cos_out,     // final 14-bit unsigned cosine
    output wire [PW-1:0]       phase_acc_out,
    output wire                cordic_aux_out

);
    // Phase accumulator
    reg [PW-1:0] phase_acc;
    always @(posedge sys_clk or negedge rst_n) begin
        if (!rst_n)
            phase_acc <= {PW{1'b0}};
        else
            phase_acc <= phase_acc + phase_inc;
    end

    assign phase_acc_out = phase_acc;

    // CORDIC instance
    localparam signed [IW-1:0] I_XINIT = 12'sd1000;
    localparam signed [IW-1:0] I_YINIT = 12'sd0;
    wire signed [OW-1:0] cordic_cos, cordic_sin;
    wire                 cordic_aux;
    cordic #(
        .IW      (IW),
        .OW      (OW),
        .NSTAGES (NSTAGES),
        .WW      (WW),
        .PW      (PW)
    ) u_cordic (
        .i_clk   (sys_clk),
        .i_reset (~rst_n),
        .i_ce    (1'b1),
        .i_xval  (I_XINIT),
        .i_yval  (I_YINIT),
        .i_phase (phase_acc),
        .i_aux   (1'b1),
        .o_xval  (cordic_cos),
        .o_yval  (cordic_sin),
        .o_aux   (cordic_aux)
    );

    assign cordic_aux_out = cordic_aux;

    // Scale and register CORDIC outputs
    reg signed [13:0] shift_sin, shift_cos;
    reg               aux_d1;
    always @(posedge sys_clk or negedge rst_n) begin
        if (!rst_n) begin
            shift_sin <= 0;
            shift_cos <= 0;
            aux_d1    <= 1'b0;
        end else begin
            aux_d1 <= cordic_aux;
            if (cordic_aux) begin
                shift_sin <= cordic_sin <<< 2;
                shift_cos <= cordic_cos <<< 2;
            end
        end
    end

    reg [13:0] sin_reg, cos_reg;
    always @(posedge sys_clk or negedge rst_n) begin
        if (!rst_n) begin
            sin_reg <= 14'd0;
            cos_reg <= 14'd0;
        end else if (aux_d1) begin
            sin_reg <= shift_sin + 14'd8192;
            cos_reg <= shift_cos + 14'd8192;
        end
    end

    assign sin_out = sin_reg;
    assign cos_out = cos_reg;

endmodule
