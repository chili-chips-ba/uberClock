
module cic #(
    parameter  DATA_WIDTH       = 16,
    parameter  REGISTER_WIDTH   = 64,
    parameter  DECIMATION_RATIO = 8
) (
    input  wire                          clk,
    input  wire                          arst_n,
    input  wire                          en,
    input  wire signed [DATA_WIDTH-1:0]  data_in,
    output wire signed [DATA_WIDTH-1:0]  data_out,
    output wire                          data_clk
);

  localparam COUNT_WIDTH = $clog2(DECIMATION_RATIO);

  //=============================//
  //       Internal signals      //
  //=============================//
  reg signed [REGISTER_WIDTH-1:0] integrator_tmp, integrator_d_tmp;
  reg signed [REGISTER_WIDTH-1:0] integrator1, integrator2, integrator3, integrator4, integrator5;
  reg signed [REGISTER_WIDTH-1:0] comb6, comb_d6, comb7, comb_d7, comb8, comb_d8, comb9, comb_d9, comb10;
  reg                             valid_comb;
  reg                             decimation_clk;
  reg        [COUNT_WIDTH-1:0]    count;

    
  //=============================//
  //    Init section       //
  //=============================//


  //=============================//
  //    Integrator section       //
  //=============================//
   
 always @(posedge clk or negedge arst_n) begin
    if (!arst_n) begin
        count          <= {COUNT_WIDTH{1'b0}};
        integrator1    <= 0;
        integrator2    <= 0;
        integrator3    <= 0;
        integrator4    <= 0;
        integrator5    <= 0;
        integrator_tmp <= 0;
        integrator_d_tmp <= 0;
        decimation_clk <= 1'b0;
        valid_comb     <= 1'b0;
        
    end else if (en)  begin
        // Integrator logic
        integrator1 <= integrator1 + data_in;
        integrator2 <= integrator1 + integrator2;
        integrator3 <= integrator2 + integrator3;
        integrator4 <= integrator3 + integrator4;
        integrator5 <= integrator4 + integrator5;
        

        // Decimation logic
        if (count == DECIMATION_RATIO - 1) begin
            count          <= {COUNT_WIDTH{1'b0}};
            integrator_tmp <= integrator5;
            decimation_clk <= 1'b1;
            valid_comb     <= 1'b1;
        end else if (count == DECIMATION_RATIO >> 1) begin
            decimation_clk <= 1'b0;
            count          <= count + 1;
            valid_comb     <= 1'b0;
        end else begin
            count          <= count + 1;
            valid_comb     <= 1'b0;
        end
    end
end


  //=============================//
  //       Comb section          //
  //=============================//
  always @(posedge clk or negedge arst_n) begin
    if (!arst_n) begin
        comb6    <= 0;
        comb_d6    <= 0;
        comb7    <= 0;
        comb_d7    <= 0;
        comb8    <= 0;
        comb_d8 <= 0;
        comb9 <= 0;
        comb_d9 <= 0;
        comb10     <= 0;
        
        end
        else if (valid_comb == 1'b1) begin
      integrator_d_tmp  <= integrator_tmp;
      comb6             <= integrator_tmp - integrator_d_tmp;
      comb_d6           <= comb6;
      comb7             <= comb6 - comb_d6;
      comb_d7           <= comb7;
      comb8             <= comb7 - comb_d7;
      comb_d8           <= comb8;
      comb9             <= comb8 - comb_d8;
      comb_d9           <= comb9;
      comb10            <= comb9 - comb_d9;
    end
  end

 
  assign data_out = (comb10 >>> (REGISTER_WIDTH - DATA_WIDTH ));
  assign data_clk = decimation_clk;
  
  endmodule
