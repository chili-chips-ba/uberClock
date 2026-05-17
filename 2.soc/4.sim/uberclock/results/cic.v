


module cic #(
    parameter DATA_WIDTH_I     = 12,
    parameter DATA_WIDTH_O     = 16,
    parameter REGISTER_WIDTH   = 64,
    parameter DECIMATION_RATIO = 8
) (
    input  wire                           clk,
    input  wire                           arst_n,   // async, active-low
    input  wire                           en,
    input  wire signed [DATA_WIDTH_I-1:0] data_in,
    output wire signed [DATA_WIDTH_O-1:0] data_out,
    output wire                           data_clk
);
  localparam COUNT_WIDTH = $clog2(DECIMATION_RATIO);

  // ============================= //
  //       Internal signals        //
  // ============================= //
  reg signed [REGISTER_WIDTH-1:0] integrator1, integrator2, integrator3, integrator4;
  reg signed [REGISTER_WIDTH-1:0] integrator_tmp, integrator_d_tmp;
  reg signed [REGISTER_WIDTH-1:0] comb5, comb_d5, comb6, comb_d6, comb7, comb_d7, comb8;

  reg                             valid_comb;    // raw pulse (same cycle as capture)
  reg                             valid_comb_q;  // 1-cycle delayed pulse for comb
  reg                             decimation_clk;
  reg        [COUNT_WIDTH-1:0]    count = 0;

  // ============================= //
  //    Integrator section         //
  // ============================= //
  always @(posedge clk or negedge arst_n) begin
    if (!arst_n) begin
      count          <= 0;
      integrator1    <= 0;
      integrator2    <= 0;
      integrator3    <= 0;
      integrator4    <= 0;
      integrator_tmp <= 0;
      decimation_clk <= 1'b0;
      valid_comb     <= 1'b0;
      valid_comb_q   <= 1'b0;
    end else if (en) begin
      // Integrators
      integrator1 <= integrator1 + data_in;
      integrator2 <= integrator2 + integrator1;
      integrator3 <= integrator3 + integrator2;
      integrator4 <= integrator4 + integrator3;

      // Defaults
      decimation_clk <= 1'b0;
      valid_comb     <= 1'b0;

      // Decimation pulse & capture
      if (count == DECIMATION_RATIO - 1) begin
        count          <= 0;
        integrator_tmp <= integrator4;  // stable sample for comb next cycle
        decimation_clk <= 1'b1;         // raw (same-cycle) pulse
        valid_comb     <= 1'b1;         // raw (same-cycle) pulse
      end else begin
        count <= count + 1'b1;
      end

      // Delay the CE so the comb sees stable data
      valid_comb_q <= valid_comb;
    end
  end

  // ============================= //
  //        Comb section           //
  // ============================= //
  always @(posedge clk or negedge arst_n) begin
    if (!arst_n) begin
      integrator_d_tmp <= 0;   // single procedural writer here
      comb5   <= 0; comb_d5 <= 0;
      comb6   <= 0; comb_d6 <= 0;
      comb7   <= 0; comb_d7 <= 0;
      comb8   <= 0;
    end else if (valid_comb_q) begin
      // classic 4-stage comb
      integrator_d_tmp <= integrator_tmp;
      comb5   <= integrator_tmp - integrator_d_tmp;
      comb_d5 <= comb5;
      comb6   <= comb5 - comb_d5;
      comb_d6 <= comb6;
      comb7   <= comb6 - comb_d6;
      comb_d7 <= comb7;
      comb8   <= comb7 - comb_d7;
    end
  end

  // ============================= //
  //        Output section         //
  // ============================= //
  assign data_out = comb8 >>> (REGISTER_WIDTH - DATA_WIDTH_O - 1);
  // IMPORTANT: CE aligned with data_out
  assign data_clk = valid_comb_q;

endmodule

