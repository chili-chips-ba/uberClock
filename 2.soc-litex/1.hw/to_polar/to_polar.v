
module to_polar #(
    parameter IW      = 16,   // input width
    parameter OW      = 16,   // output magnitude width
    parameter WW      = 26,   // internal working width
    parameter PW      = 25,   // phase accumulator width
    parameter NSTAGES = 22    // number of CORDIC iterations
) (
    input  wire                 clk,
    input  wire                 rst,
    input  wire                 i_ce,      // clock-enable for pipeline
    input  wire signed [IW-1:0] i_xval,
    input  wire signed [IW-1:0] i_yval,
    input  wire                 i_aux,
    output reg  signed [OW-1:0] o_mag,
    output reg        [PW-1:0]  o_phase,
    output reg                 o_aux
);

    // 1) expand inputs to working width
    wire signed [WW-1:0] ext_x = { {2{i_xval[IW-1]}}, i_xval, {(WW-IW-2){1'b0}} };
    wire signed [WW-1:0] ext_y = { {2{i_yval[IW-1]}}, i_yval, {(WW-IW-2){1'b0}} };

    // 2) CORDIC angle lookup
    wire [PW-1:0] cordic_angle [0:NSTAGES-1];
    assign cordic_angle[ 0] = 25'h025_c80a;
    assign cordic_angle[ 1] = 25'h013_f670;
    assign cordic_angle[ 2] = 25'h00a_2223;
    assign cordic_angle[ 3] = 25'h005_161a;
    assign cordic_angle[ 4] = 25'h002_8baf;
    assign cordic_angle[ 5] = 25'h001_45ec;
    assign cordic_angle[ 6] = 25'h000_a2f8;
    assign cordic_angle[ 7] = 25'h000_517c;
    assign cordic_angle[ 8] = 25'h000_28be;
    assign cordic_angle[ 9] = 25'h000_145f;
    assign cordic_angle[10] = 25'h000_0a2f;
    assign cordic_angle[11] = 25'h000_0517;
    assign cordic_angle[12] = 25'h000_028b;
    assign cordic_angle[13] = 25'h000_0145;
    assign cordic_angle[14] = 25'h000_00a2;
    assign cordic_angle[15] = 25'h000_0051;
    assign cordic_angle[16] = 25'h000_0028;
    assign cordic_angle[17] = 25'h000_0014;
    assign cordic_angle[18] = 25'h000_000a;
    assign cordic_angle[19] = 25'h000_0005;
    assign cordic_angle[20] = 25'h000_0002;
    assign cordic_angle[21] = 25'h000_0001;

    // 3) pipeline registers
    reg signed [WW-1:0] stage_x   [0:NSTAGES];
    reg signed [WW-1:0] stage_y   [0:NSTAGES];
    reg        [PW-1:0] stage_phi [0:NSTAGES];
    reg                 stage_aux [0:NSTAGES];


    // stage 0: pre-rotation into ±45°
    always @(posedge clk) begin
        if (rst) begin
            stage_x[0]   <= 0;
            stage_y[0]   <= 0;
            stage_phi[0] <= 0;
            stage_aux[0] <= 0;
        end else if (i_ce) begin
            stage_aux[0] <= i_aux;
            case ({i_xval[IW-1], i_yval[IW-1]})
                2'b01: begin
                    stage_x[0]   <=  ext_x - ext_y;
                    stage_y[0]   <=  ext_x + ext_y;
                    stage_phi[0] <= 25'h1c00000;  // -315°
                end
                2'b10: begin
                    stage_x[0]   <= -ext_x + ext_y;
                    stage_y[0]   <= -ext_x - ext_y;
                    stage_phi[0] <= 25'hc00000;   // -135°
                end
                2'b11: begin
                    stage_x[0]   <= -ext_x - ext_y;
                    stage_y[0]   <=  ext_x - ext_y;
                    stage_phi[0] <= 25'h1400000;  // -225°
                end
                default: begin
                    stage_x[0]   <=  ext_x + ext_y;
                    stage_y[0]   <= -ext_x + ext_y;
                    stage_phi[0] <= 25'h400000;   // -45°
                end
            endcase
        end
    end

    // stages 1..NSTAGES: one rotation per cycle
    genvar i;
    generate
    for (i = 0; i < NSTAGES; i = i+1) begin : CORDIC_LOOP
        always @(posedge clk) begin
            if (rst) begin
                stage_x[i+1]   <= 0;
                stage_y[i+1]   <= 0;
                stage_phi[i+1] <= 0;
                stage_aux[i+1] <= 0;
            end else if (i_ce) begin
                stage_aux[i+1] <= stage_aux[i];
                if (stage_y[i][WW-1]) begin
                    // below x-axis: rotate positive
                    stage_x[i+1]   <= stage_x[i] - (stage_y[i] >>> (i+1));
                    stage_y[i+1]   <= stage_y[i] + (stage_x[i] >>> (i+1));
                    stage_phi[i+1] <= stage_phi[i] - cordic_angle[i];
                end else begin
                    // above x-axis: rotate negative
                    stage_x[i+1]   <= stage_x[i] + (stage_y[i] >>> (i+1));
                    stage_y[i+1]   <= stage_y[i] - (stage_x[i] >>> (i+1));
                    stage_phi[i+1] <= stage_phi[i] + cordic_angle[i];
                end
            end
        end
    end
    endgenerate

    // 4) final rounding and output regs
    wire [WW-1:0] pre_mag = stage_x[NSTAGES]
        + $signed({ {(OW){1'b0}},
                    stage_x[NSTAGES][WW-OW],
                    {(WW-OW-1){!stage_x[NSTAGES][WW-OW]}} });

    always @(posedge clk) begin
        if (rst) begin
            o_mag   <= 0;
            o_phase <= 0;
            o_aux   <= 0;
        end else if (i_ce) begin
            o_mag   <= pre_mag[WW-1 -: OW];
            o_phase <= stage_phi[NSTAGES];
            o_aux   <= stage_aux[NSTAGES];
        end
    end

endmodule
