// ============================================================================
//  cordic_nco.v
//    A simple NCO that uses your existing `cordic` core to generate sine/cosine.
//    At each rising edge (when i_ce=1), the phase_accumulator increments by
//    PHASE_INC.  That accumulator is fed into cordic.i_phase, and the CORDIC
//    rotates a fixed “unit” vector by that phase each cycle.
// ============================================================================

module cordic_top #(
    //------------------------------------------------------------------------
    //  Match these to your `cordic` instance.  If you change IW/OW/NSTAGES/WW/PW
    //  in cordic.v, keep them in sync here.
    //------------------------------------------------------------------------
    parameter IW       = 12,   // input width to CORDIC (bits)
    parameter OW       = 12,   // output width from CORDIC (bits)
    parameter NSTAGES  = 15,   // how many pipeline stages your CORDIC uses
    parameter WW       = 15,   // internal working width for CORDIC
    parameter PW       = 19,   // phase‐accumulator width

    //------------------------------------------------------------------------
    //  PHASE_INC controls output frequency:
    //    f_out = f_clk * (PHASE_INC / 2^PW).
    //  Example: if f_clk=50 MHz, PW=19, and you want f_out ≈ 1 kHz:
    //      PHASE_INC = round(1e3 * 2^19 / 50e6) ≈ round(524288 / 50) = 10486.
    //------------------------------------------------------------------------
    parameter [PW-1:0] PHASE_INC = 19'd10486,

    //------------------------------------------------------------------------
    //  The CORDIC has a net gain of ≈ 1.164435.  To get a unity‐amplitude sine/cos
    //  out, you should preload i_xval with 1/GAIN in your CORDIC’s fixed‐point
    //  format.  If IW=12, and you treat i_xval as signed Q1.(IW−1) = Q1.11, then:
    //      1/G := 1 / 1.164435 ≈ 0.85934.
    //      In Q1.11, that value is round(0.85934 * 2^11) = 1760 decimal (0x6E0).
    //  If you change IW or WW, you’ll need to recompute this scale.
    //------------------------------------------------------------------------
    localparam signed [IW-1:0] I_XINIT = 12'sd1760,  // ≈ 0.85934 in Q1.11
    localparam signed [IW-1:0] I_YINIT = 12'sd0      // start vector on +x-axis

) (
    input  wire                clk,     // master clock
    input  wire                reset,   // synchronous reset (active high)
    input  wire                ce,      // enable; when ce=1, advance phase and CORDIC

    output wire signed [OW-1:0] o_cos,   // cosine output (scaled)
    output wire signed [OW-1:0] o_sin    // sine   output (scaled)
);

    //--------------------------------------------------------------------------
    //  Phase accumulator of width PW.  On each rising clk (when ce=1), advance
    //  by PHASE_INC; wraps around naturally at 2^PW.
    //--------------------------------------------------------------------------
    reg [PW-1:0] phase_acc;
    always @(posedge clk) begin
        if (reset) begin
            phase_acc <= {PW{1'b0}};
        end else if (ce) begin
            phase_acc <= phase_acc + PHASE_INC;
        end
    end

    //--------------------------------------------------------------------------
    //  Instantiate your existing CORDIC core.  We tie
    //    i_xval = I_XINIT, i_yval = I_YINIT, and feed i_phase = phase_acc.
    //  The CORDIC’s outputs (o_xval, o_yval) will be:
    //    o_xval ≈ cos(2π·phase_acc/2^PW)  scaled by (1/G),
    //    o_yval ≈ sin(2π·phase_acc/2^PW)  scaled by (1/G).
    //  We ignore the auxiliary pipeline output (o_aux); if you want a “valid”
    //  pulse after the pipeline, you can tap cordic.o_aux instead.
    //--------------------------------------------------------------------------
    wire aux_unused;
    cordic #(
        .IW      (IW),
        .OW      (OW),
        .NSTAGES (NSTAGES),
        .WW      (WW),
        .PW      (PW)
    ) u_cordic (
        .i_clk   (clk),
        .i_reset (reset),
        .i_ce    (ce),
        .i_xval  (I_XINIT),
        .i_yval  (I_YINIT),
        .i_phase (phase_acc),
        .i_aux   (1'b1),       // tie aux=1 so the “valid” pipeline shifts through
        .o_xval  (o_cos),
        .o_yval  (o_sin),
        .o_aux   (aux_unused)  // you can monitor this if you need “valid”
    );

endmodule
