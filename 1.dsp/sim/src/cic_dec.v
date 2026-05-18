/*
 * Cascaded integrator-comb (CIC) Decimator
 * clk_enable-driven, ce_out strobe output
 */

module cic_dec #(
    parameter I_WIDTH    = 16,
    parameter O_WIDTH    = 14,
    parameter RMAX       = 1625,
    parameter M          = 1,
    parameter N          = 5
    // REG_WIDTH is enough to prevent overflow
    //parameter REG_WIDTH  = I_WIDTH + $clog2((RMAX*M)**N)
)(
    input  wire                   clk,
    input  wire                   rst,
    input  wire                   clk_enable,

    input  wire [I_WIDTH-1:0]     input_tdata,

    output wire [O_WIDTH-1:0]     output_tdata,
    output wire                   clk_out,
    output reg                    ce_out
);
    localparam REG_WIDTH = I_WIDTH + N * $clog2(RMAX * M);

    // =========================================================
    // Internal state
    // =========================================================
    reg [$clog2(RMAX+1)-1:0] cycle_reg = 0;

    // N integrator + comb stage registers
    reg [REG_WIDTH-1:0] int_reg  [0:N-1];
    reg [REG_WIDTH-1:0] comb_reg [0:N-1];

    genvar k;
    integer i;

    // =========================================================
    // Integrator stages (always run at input rate)
    // =========================================================
    generate
    for (k = 0; k < N; k = k + 1) begin : integrator
        always @(posedge clk) begin
            if (rst) begin
                int_reg[k] <= 0;
            end else if (clk_enable) begin
                if (k == 0) begin
                    int_reg[k] <= $signed(int_reg[k]) + $signed(input_tdata);
                end else begin
                    int_reg[k] <= $signed(int_reg[k]) + $signed(int_reg[k-1]);
                end
            end
        end
    end
    endgenerate

    // =========================================================
    // Comb stages (only update on output strobe)
    // =========================================================
    generate
    for (k = 0; k < N; k = k + 1) begin : comb
        reg [REG_WIDTH-1:0] delay_reg [0:M-1];

        initial begin
            for (i = 0; i < M; i = i + 1)
                delay_reg[i] = 0;
            comb_reg[k] = 0;
        end

        always @(posedge clk) begin
            if (rst) begin
                for (i = 0; i < M; i = i + 1)
                    delay_reg[i] <= 0;
                comb_reg[k] <= 0;
            end else if (ce_out) begin
                if (k == 0) begin
                    delay_reg[0] <= $signed(int_reg[N-1]);
                    comb_reg[k]  <= $signed(int_reg[N-1]) - $signed(delay_reg[M-1]);
                end else begin
                    delay_reg[0] <= $signed(comb_reg[k-1]);
                    comb_reg[k]  <= $signed(comb_reg[k-1]) - $signed(delay_reg[M-1]);
                end

                for (i = 0; i < M-1; i = i + 1)
                    delay_reg[i+1] <= delay_reg[i];
            end
        end
    end
    endgenerate

    // =========================================================
    // Cycle counter & ce_out strobe generation
    // =========================================================
    always @(posedge clk) begin
        if (rst) begin
            cycle_reg <= 0;
            ce_out    <= 1'b0;
        end else if (clk_enable) begin
            if (cycle_reg == RMAX - 1) begin
                cycle_reg <= 0;
                ce_out    <= 1'b1;   // one-cycle pulse
            end else begin
                cycle_reg <= cycle_reg + 1;
                ce_out    <= 1'b0;
            end
        end else begin
            ce_out <= 1'b0;
        end
    end

    // =========================================================
    // Outputs
    // =========================================================
    
    
   
//    assign output_tdata = gain_full_reg >>> 31;
    assign output_tdata = (comb_reg[N-1] >>> (REG_WIDTH - O_WIDTH - 1));
    assign clk_out      = clk & ce_out;   // gated clock pulse

endmodule