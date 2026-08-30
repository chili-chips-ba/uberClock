// ============================================================================
//  cordic_dac.v  (core logic, with CPU‐controlled phase increment)
// ============================================================================

module cordic_dac #(
    parameter IW       = 12,   // CORDIC input width (bits)
    parameter OW       = 12,   // CORDIC output width (bits)
    parameter NSTAGES  = 15,   // number of pipeline stages in CORDIC
    parameter WW       = 15,   // internal working width (bits)
    parameter PW       = 19    // phase-accumulator width (bits)
) (
    //--------------------------------------------------------------------------
    //  Clock / Reset / Phase interface
    //--------------------------------------------------------------------------
    input  wire                sys_clk,
    input  wire                rst_n,
    input  wire [PW-1:0]       phase_inc,

    //--------------------------------------------------------------------------
    //  DAC #1 interface
    //--------------------------------------------------------------------------
    output wire                da1_clk,
    output wire                da1_wrt,
    output wire [13:0]         da1_data,

    //--------------------------------------------------------------------------
    //  DAC #2 interface
    //--------------------------------------------------------------------------
    output wire                da2_clk,
    output wire                da2_wrt,
    output wire [13:0]         da2_data
);

    //==========================================================================
    //  1) CORDIC + scaling logic all wrapped up in cordic_logic
    //==========================================================================
    wire [13:0] sin_out;
    wire [13:0] cos_out;

    cordic_logic #(
        .IW      (IW),
        .OW      (OW),
        .NSTAGES (NSTAGES),
        .WW      (WW),
        .PW      (PW)
    ) u_cordic_logic (
        .sys_clk    (sys_clk),
        .rst_n      (rst_n),
        .phase_inc  (phase_inc),
        .sin_out    (sin_out),
        .cos_out    (cos_out)
    );

    //==========================================================================
    //  2) Instantiate the separate DDR-output DAC module
    //==========================================================================
    dac u_dac_out (
        .sys_clk  (sys_clk),
        .rst_n    (rst_n),
        .data1    (sin_out),    // 14-bit sine word from cordic_logic
        .data2    (cos_out),    // 14-bit cosine word from cordic_logic
        .da1_clk  (da1_clk),
        .da1_wrt  (da1_wrt),
        .da1_data (da1_data),
        .da2_clk  (da2_clk),
        .da2_wrt  (da2_wrt),
        .da2_data (da2_data)
    );

endmodule
