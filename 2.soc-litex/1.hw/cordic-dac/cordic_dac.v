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
    //  1) Phase accumulator (width = PW), driven by CPU‐supplied phase_inc
    //==========================================================================
    reg [PW-1:0] phase_acc;
    always @(posedge sys_clk or negedge rst_n) begin
        if (!rst_n) begin
            phase_acc <= {PW{1'b0}};
        end else begin
            phase_acc <= phase_acc + phase_inc;
        end
    end

    //==========================================================================
    //  2) Instantiate the pipeline CORDIC core. We tie:
    //       i_xval = I_XINIT, i_yval = I_YINIT, i_phase = phase_acc, i_aux = 1'b1.
    //     The CORDIC outputs:
    //       cordic_cos ≈ (1/G)·cos(2π·phase_acc/2^PW)
    //       cordic_sin ≈ (1/G)·sin(2π·phase_acc/2^PW)
    //==========================================================================

    //  To cancel the CORDIC gain (~1.164435), preload i_xval = (1/G) in Q1.(IW-1).
    //  For IW=12, Q1.11: 1/1.164435 ≈ 0.85934 → round(0.85934 * 2^11) = 1760 (0x6E0).
    localparam signed [IW-1:0] I_XINIT = 12'sd1760;
    localparam signed [IW-1:0] I_YINIT = 12'sd0;

    wire signed [OW-1:0] cordic_cos;
    wire signed [OW-1:0] cordic_sin;
    wire                 cordic_aux;  // “valid” flag (unused downstream)

    cordic #(
        .IW      (IW),
        .OW      (OW),
        .NSTAGES (NSTAGES),
        .WW      (WW),
        .PW      (PW)
    ) u_cordic (
        .i_clk   (sys_clk),
        .i_reset (~rst_n),        // invert active-low → active-high for CORDIC
        .i_ce    (1'b1),          // always enabled
        .i_xval  (I_XINIT),
        .i_yval  (I_YINIT),
        .i_phase (phase_acc),
        .i_aux   (1'b1),          // shift a “1” through pipeline
        .o_xval  (cordic_cos),
        .o_yval  (cordic_sin),
        .o_aux   (cordic_aux)
    );

    //==========================================================================
    //  3) Scale 12→14 bits and register
    //==========================================================================

    // Stage-1: shift left by 2 (multiply by 4) when cordic_aux is asserted
    reg signed [13:0] shift_sin, shift_cos;
    reg               aux_d1;

    always @(posedge sys_clk or negedge rst_n) begin
        if (!rst_n) begin
            shift_sin <= 0;
            shift_cos <= 0;
            aux_d1    <= 1'b0;
        end else begin
            // Delay the valid flag by one cycle
            aux_d1 <= cordic_aux;

            // When CORDIC indicates “valid”, capture the scaled values
            if (cordic_aux) begin
                shift_sin <= cordic_sin <<< 2;  // range: -8192 … +8191
                shift_cos <= cordic_cos <<< 2;
            end
        end
    end

    // Stage-2: add mid-scale offset (8192) when shift is ready
    reg [13:0] sin_reg, cos_reg;
    always @(posedge sys_clk or negedge rst_n) begin
        if (!rst_n) begin
            sin_reg <= 14'd8192;  // mid-scale on reset
            cos_reg <= 14'd8192;
        end else if (aux_d1) begin
            sin_reg <= shift_sin + 14'd8192;  // final 14-bit unsigned (0…16383)
            cos_reg <= shift_cos + 14'd8192;
        end
    end

    //==========================================================================
    //  4) Instantiate the separate DAC DDR-output module
    //==========================================================================
    dac u_dac_out (
        .sys_clk  (sys_clk),
        .rst_n    (rst_n),
        .data1    (sin_reg),    // 14-bit sine word
        .data2    (cos_reg),    // 14-bit cosine word
        .da1_clk  (da1_clk),
        .da1_wrt  (da1_wrt),
        .da1_data (da1_data),
        .da2_clk  (da2_clk),
        .da2_wrt  (da2_wrt),
        .da2_data (da2_data)
    );

endmodule
