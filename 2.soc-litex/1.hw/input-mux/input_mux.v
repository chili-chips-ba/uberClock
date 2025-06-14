module input_mux #(
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
    //  Input select bit from CPU
    //--------------------------------------------------------------------------
    input                      input_sw_reg,
    //--------------------------------------------------------------------------
    //  ADC (12-bit inputs; AD9238 on J11)
    //--------------------------------------------------------------------------
    output                     adc_clk_ch0,
    output                     adc_clk_ch1,
    input      [11:0]          adc_data_ch0,
    input      [11:0]          adc_data_ch1,
    //--------------------------------------------------------------------------
    //  DDR-DAC #1 interface
    //--------------------------------------------------------------------------
    output wire                da1_clk,
    output wire                da1_wrt,
    output wire [13:0]         da1_data,
    //--------------------------------------------------------------------------
    //  DDR-DAC #2 interface
    //--------------------------------------------------------------------------
    output wire                da2_clk,
    output wire                da2_wrt,
    output wire [13:0]         da2_data,

    output wire [13:0]         debug_sin,
    output wire [13:0]         debug_cos,
    output wire [PW-1:0]       phase_acc_out,
    output wire                cordic_aux_out
);

    //-------------------------------------------------------------------------
    //  1) Grab ADC samples into our local domain
    //-------------------------------------------------------------------------
    wire [11:0] ad_data_ch0_12;
    wire [11:0] ad_data_ch1_12;
    adc u_adc (
        .sys_clk      (sys_clk),
        .rst_n        (rst_n),
        // raw DDR-pinned inputs
        .adc_data_ch0 (adc_data_ch0),
        .adc_data_ch1 (adc_data_ch1),
        // DDR clocks
        .adc_clk_ch0  (adc_clk_ch0),
        .adc_clk_ch1  (adc_clk_ch1),
        // 12-bit outputs
        .ad_data_ch0  (ad_data_ch0_12),
        .ad_data_ch1  (ad_data_ch1_12)
    );

    //-------------------------------------------------------------------------
    //  2) All the CORDIC + scaling logic in one module
    //-------------------------------------------------------------------------
    wire [13:0] sin_out;
    wire [13:0] cos_out;
    cordic_logic #(
        .IW      (IW),
        .OW      (OW),
        .NSTAGES (NSTAGES),
        .WW      (WW),
        .PW      (PW)
    ) u_cordic_logic (
        .sys_clk   (sys_clk),
        .rst_n     (~rst_n),
        .phase_inc (phase_inc),
        .sin_out   (sin_out),
        .cos_out   (cos_out),
        .phase_acc_out(phase_acc_out),
        .cordic_aux_out(cordic_aux_out)
    );

    //-------------------------------------------------------------------------
    //  3) Mux: if input_sw_reg==1, use raw ADC<<2; else use cordic-derived
    //-------------------------------------------------------------------------
    wire [13:0] dac_in_1 = input_sw_reg
                          ? (ad_data_ch0_12 <<< 2)
                          : sin_out;
    wire [13:0] dac_in_2 = input_sw_reg
                          ? (ad_data_ch1_12 <<< 2)
                          : cos_out;

    assign debug_sin = dac_in_1;
    assign debug_cos = dac_in_2;


    //-------------------------------------------------------------------------
    //  4) DDR-DAC output
    //-------------------------------------------------------------------------
    dac u_dac_out (
        .sys_clk  (sys_clk),
        .rst_n    (rst_n),
        .data1    (dac_in_1),
        .data2    (dac_in_2),
        .da1_clk  (da1_clk),
        .da1_wrt  (da1_wrt),
        .da1_data (da1_data),
        .da2_clk  (da2_clk),
        .da2_wrt  (da2_wrt),
        .da2_data (da2_data)
    );

endmodule
