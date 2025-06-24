module downsamplerFilter (
    input  wire        clk,
    input  wire        clk_enable,
    input  wire        reset,
    input  wire signed [11:0] filter_in,    // sfix12_En11
    output wire signed [15:0] filter_out,   // Final output from hb_down
    output wire        ce_out               // Clock enable from final stage
);

    // Intermediate wires
    wire signed [11:0] cic_out;
    wire               cic_ce_out;

    wire signed [15:0] comp_out;
    wire               comp_ce_out;


	cic #(
	 .DATA_WIDTH_I(12),         // Input data width
	 .DATA_WIDTH_O(12),
	 .REGISTER_WIDTH(56),     // Internal accumulator width
	 .DECIMATION_RATIO(1625)     // Decimation factor
	) cic_inst (
	 .clk        (clk),        // Input clock
	 .arst_n     (!reset),     // Asynchronous active-low reset
	 .en         (clk_enable), // Enable signal
	 .data_in    (filter_in),    // Input sample (signed 16-bit)
	 .data_out   (cic_out),   // Output sample (signed 16-bit, decimated)
	 .data_clk   (cic_ce_out)    // Output clock (valid when output sample ready)
	);



    // Stage 2: CIC Compensation Filter
    // cic_comp_down_opt comp_inst (
    //     .clk(clk),
    //     .clk_enable(cic_ce_out),   // Enable only when CIC produces output
    //     .reset(reset),
    //     .filter_in(cic_out),
    //     .filter_out(comp_out),
    //     .ce_out(comp_ce_out)
    // );
    cic_comp_down_mac #(
        .DW_IN           (12),
        .DW_OUT          (16),
        .CW              (15),
        .POLYPHASE_DEPTH (17),
        .COEFF_INIT_FILE ("comp_down_coeffs.mem")
    ) cic_comp_down_inst (
        .clk        (clk),
        .clk_enable (cic_ce_out),
        .reset      (reset),
        .filter_in  (cic_out),
        .filter_out (comp_out),
        .ce_out     (comp_ce_out)
    );

    // Stage 3: Half-Band Filter
    // hb_down_opt hb_inst (
    //     .clk(clk),
    //     .clk_enable(comp_ce_out),  // Enable only when compensation stage produces output
    //     .reset(reset),
    //     .filter_in(comp_out),
    //     .filter_out(filter_out),
    //     .ce_out(ce_out)
    // );
    hb_down_mac #(
        .DW_IN           (16),
        .DW_OUT          (16),
        .CW              (19),
        .POLYPHASE_DEPTH (48),
        .DEPTH           (64),
        .COEFF_INIT_FILE ("hb_down_coeffs.mem")
    ) hb_down_inst (
        .clk        (clk),
        .clk_enable (comp_ce_out),
        .reset      (reset),
        .filter_in  (comp_out),
        .filter_out (filter_out),
        .ce_out     (ce_out)
    );
 

endmodule
