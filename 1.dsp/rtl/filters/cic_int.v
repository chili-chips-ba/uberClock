// Language: Verilog 2001
//`timescale 1ns / 1ps
/*
 * Cascaded integrator-comb (CIC) Interpolator
 */
`timescale 1 ns / 1 ns
`default_nettype none

module cic_int #(
    parameter I_WIDTH    = 16,
    parameter O_WIDTH    = 14,
    parameter RMAX       = 1625,
    parameter M          = 1,
    parameter N          = 5,
    parameter REG_WIDTH  = 71
)(
    input  wire                      clk,
    input  wire                      rst,
    input  wire [I_WIDTH-1:0]        input_tdata,
    input  wire                      clk_enable,
    output wire [O_WIDTH-1:0]        output_tdata,
    output wire                      clk_out,
    output reg                       ce_out
);
    // counter for interpolation cycle
    reg [$clog2(RMAX+1)-1:0] cycle_reg = 0;
    // comb and integrator registers
    reg [REG_WIDTH-1:0] comb_reg   [0:N-1];
    reg [REG_WIDTH-1:0] int_reg    [0:N-1];
    genvar k;
    integer i;
    //================================================================
    // Generate comb stages
    //================================================================
    generate
    for (k = 0; k < N; k = k + 1) begin : comb
        reg [REG_WIDTH-1:0] delay_reg [0:M-1];
        initial begin
            for (i = 0; i < M; i = i + 1) begin
                delay_reg[i] = 0;
            end
            comb_reg[k] = 0;
        end
        always @(posedge clk) begin
            if (rst) begin
                for (i = 0; i < M; i = i + 1) begin
                    delay_reg[i] <= 0;
                end
                comb_reg[k] <= 0;
            end else if (cycle_reg == 0) begin
                if (k == 0) begin
                    delay_reg[0]   <= $signed(input_tdata);
                    comb_reg[k]    <= $signed(input_tdata) - $signed(delay_reg[M-1]);
                end else begin
                    delay_reg[0]   <= $signed(comb_reg[k-1]);
                    comb_reg[k]    <= $signed(comb_reg[k-1]) - $signed(delay_reg[M-1]);
                end
                for (i = 0; i < M-1; i = i + 1) begin
                    delay_reg[i+1] <= delay_reg[i];
                end
            end
        end
    end
    endgenerate
    //================================================================
    // Generate integrator stages
    //================================================================
    generate
    for (k = 0; k < N; k = k + 1) begin : integrator
        always @(posedge clk) begin
            if (rst) begin
                int_reg[k] <= 0;
            end else begin
                if (k == 0) begin
                    int_reg[k] <= $signed(int_reg[k]) + $signed(comb_reg[N-1]);
                end else begin
                    int_reg[k] <= $signed(int_reg[k]) + $signed(int_reg[k-1]);
                end
            end
        end
    end
    endgenerate
    //================================================================
    // Cycle counter & ce_out generation (one-cycle pulse at RMAX)
    //================================================================
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            cycle_reg <= 0;
            ce_out    <= 1'b0;
        end else if (clk_enable) begin
            if (cycle_reg == RMAX-1) begin
                cycle_reg <= 0;
                ce_out    <= 1'b1;
            end else begin
                cycle_reg <= cycle_reg + 1;
                ce_out    <= 1'b0;
            end
        end else begin
            // hold count, no pulse
            ce_out <= 1'b0;
        end
    end
    //================================================================
    // Outputs
    //================================================================
    // truncate / round down from REG_WIDTH to O_WIDTH
    assign output_tdata = int_reg[N-1] >>> (REG_WIDTH - O_WIDTH -1);
    // clk_out is gated version of clk
    assign clk_out      = clk & ce_out;
endmodule
