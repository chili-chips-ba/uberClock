`timescale 1 ns / 1 ns
`default_nettype none
module hb_down_mac #(
    parameter DW_IN           = 16,
    parameter DW_ACC          = 41,
    parameter DW_OUT          = 16,
    parameter CW              = 19,
    parameter POLYPHASE_DEPTH = 48,
    parameter DEPTH           = 64,
    parameter COEFF_INIT_FILE = "hb_down_coeffs.mem",
    localparam ADDR_D = $clog2(DEPTH),                // bits to index DATA RAM
    localparam ADDR_C = $clog2(2*POLYPHASE_DEPTH),    // bits to index COEFF RAM
    localparam OFFSET = POLYPHASE_DEPTH - 1           // read-pointer offset
)(
    input  wire                   clk,
    input  wire                   clk_enable,  // 40 kHz sample tick
    input  wire                   reset,
    input  wire signed [DW_IN-1:0] filter_in,
    output wire  signed [DW_OUT-1:0] filter_out,
    output wire                    ce_out      // 20 kHz output strobe
);
//========================================================================
// 1) two-phase strobe
//========================================================================
reg phase_toggle;
wire phase_0, phase_1;

always @(posedge clk or posedge reset) begin
  if (reset) begin
    phase_toggle <= 1'b0;
  end else if (clk_enable) begin
        phase_toggle <= ~phase_toggle; // Toggle on each clk_enable
  end
end

assign phase_0 = ~phase_toggle && clk_enable;
assign phase_1 = phase_toggle && clk_enable;


//--------------------------------------------------------------------------
// 2) Coefficient memory 
//--------------------------------------------------------------------------
(* rom_style="block" *) 
reg signed [CW-1:0] coeffs [0:2*POLYPHASE_DEPTH-1];
initial $readmemb(COEFF_INIT_FILE, coeffs);

//--------------------------------------------------------------------------
// 3) Data RAM: circular buffer for last DEPTH samples
//---------------------------------------------------------------------------
reg signed [DW_IN-1:0] mem0 [0:DEPTH-1];
reg signed [DW_IN-1:0] mem1 [0:DEPTH-1];

// initial $readmemh("zeros64.mem", mem0);
// initial $readmemh("zeros64.mem", mem1);

//write pointer and read pointer control for phase_0 input samples
reg [ADDR_D-1:0] w_ptr0, r_ptr0;
always @(posedge clk or posedge reset) begin
  if (reset) begin
    w_ptr0 <= 0;
  end else if (phase_1) begin
    w_ptr0 <= w_ptr0 + 1; 
  end
end

always @(posedge clk or posedge reset) begin
  if (reset) begin
    r_ptr0 <= 0;
  end else if (phase_0) begin
    r_ptr0 <= w_ptr0 - OFFSET ; 
  end else if (phase_0_running) begin
    r_ptr0 <= r_ptr0 + 1; 
  end
end

always @(posedge clk) begin
if (phase_1) begin
    mem0[w_ptr0] <= filter_in; 
  end
end

reg signed [DW_IN-1:0] current_sample;



//write pointer and read pointer control for phase_1 input samples

reg [ADDR_D-1:0] w_ptr1, r_ptr1;
always @(posedge clk or posedge reset) begin
  if (reset) begin
    w_ptr1 <= 0;
  end else if (phase_0) begin
    w_ptr1 <= w_ptr1 + 1;
  end
end

always @(posedge clk or posedge reset) begin
  if (reset) begin
    r_ptr1 <= 0;
  end else if (phase_1) begin
    r_ptr1 <= w_ptr1 - OFFSET; 
  end else if (phase_1_running) begin
    r_ptr1 <= r_ptr1 + 1; 
  end
end

always @(posedge clk) begin
if (phase_0) begin
    mem1[w_ptr1] <= filter_in; 
  end
end

always @(posedge clk) begin
  if (phase_0_running) begin
    current_sample <= mem0[r_ptr0];
  end else if (phase_1_running) begin
    current_sample <= mem1[r_ptr1];
  end
end


// read pointer for coefficients
reg [ADDR_C-1:0] coeff_addr;
always @(posedge clk) begin
  if (reset) begin
    coeff_addr <= 0;
  end else if (phase_0_running) begin
    coeff_addr <= OFFSET - index;
  end else if (phase_1_running) begin
    coeff_addr <= 2 * OFFSET + 1 - index;
  end 
end


reg signed [CW-1:0] current_coeff;
always @(posedge clk) begin
    if (phase_1_running) begin
        current_coeff <= coeffs[OFFSET - index];
    end
    if (phase_0_running) begin
        current_coeff <= coeffs[2 * OFFSET + 1 - index];
    end
end

//multiply

reg signed [DW_IN+CW-1:0] product;
wire product0_valid, product1_valid;
assign product0_valid = state > PH0_0 && state < PH0_CLEANUP1;
assign product1_valid = state > PH1_0 && state < PH1_CLEANUP1;


always @(posedge clk) begin
  if (product0_valid || product1_valid) begin
    product <= current_sample * current_coeff;
  end
end

// accumulate the products

reg signed [DW_ACC-1:0] acc, acc_reg;
reg clear_acc, acc_en;
always @(posedge clk or posedge reset) begin
  if (reset || clear_acc) begin
    acc <= 0;
  end else if (acc_en) begin
    acc <= acc + product;
  end
end

always @(posedge clk) begin
  if (reset)
    acc_reg <= 0;
  else if (phase_1_done)  acc_reg <= acc; // register the accumulated value
end



function automatic [DW_OUT-1:0] round_sat (input signed [DW_ACC-1:0] x);
begin
   round_sat = (x[40] == 1'b0 & x[39:33] != 7'b0000000) ? 16'b0111111111111111 : 
      (x[40] == 1'b1 && x[39:33] != 7'b1111111) ? 16'b1000000000000000 : $signed({1'b0, x[33:18]} + (x[40] & |x[17:0]));
end
endfunction

//output register
reg signed [DW_OUT-1:0] filter_out_reg;
always @(posedge clk) begin
  if (reset)
    filter_out_reg <= {DW_OUT{1'b0}};      // synchronous reset
  else if (phase_1)
    filter_out_reg <= round_sat(acc_reg);
end

assign filter_out = filter_out_reg;
assign ce_out = phase_1;

//FSM and control signals

reg [4:0] state;
localparam PH0_IDLE      = 5'd0,
           PH0_0         = 5'd1,
           PH0_1         = 5'd2,
           PH0_RUNNING   = 5'd3,
           PH0_CLEANUP0  = 5'd4,
           PH0_CLEANUP1  = 5'd5,
           PH1_IDLE      = 5'd6,
           PH1_0         = 5'd7,
           PH1_1         = 5'd8,
           PH1_RUNNING   = 5'd9,
           PH1_CLEANUP0  = 5'd10,
           PH1_CLEANUP1  = 5'd11;

reg [5:0] index;
reg phase_0_running, phase_1_running;
reg phase_0_done, phase_1_done;

always @(posedge clk or posedge reset) begin
  if (reset) begin
    state <= PH0_IDLE;
    index <= 0;
    clear_acc <= 1'b1;
    acc_en <= 1'b0;
    phase_0_running <= 1'b0;
    phase_1_running <= 1'b0;
    phase_0_done <= 1'b0;
    phase_1_done <= 1'b0;
  end else begin
    phase_0_running <= 1'b0;
    phase_1_running <= 1'b0;
    phase_0_done <= 1'b0;
    phase_1_done <= 1'b0;
    clear_acc <= 1'b0;
    acc_en <= 1'b0;
    case (state)
      PH0_IDLE: begin
        if (phase_1) begin
          state <= PH0_0;
          clear_acc <= 1'b1;
          phase_0_running <= 1'b1; 
          index <= 0;
        end
      end
      
      PH0_0: begin
        state <= PH0_1;
        phase_0_running <= 1'b1; 
        index <= index + 1; 
      end

      PH0_1: begin
        state <= PH0_RUNNING;
        phase_0_running <= 1'b1;
        index <= index + 1; 
        acc_en <= 1'b1; 
      end
  
      PH0_RUNNING: begin
        acc_en <= 1'b1; 
        if (index < POLYPHASE_DEPTH - 1) begin
          index <= index + 1;
          phase_0_running <= 1'b1; 
        end else begin
          state <= PH0_CLEANUP0;
          phase_0_running <= 1'b0; 
          index <= 0; 
        end
      end
      
      PH0_CLEANUP0: begin
        acc_en <= 1'b1;
        state <= PH0_CLEANUP1;
      end

      PH0_CLEANUP1: begin
        acc_en <= 1'b0; 
        phase_0_done <= 1'b1; 
        state <= PH1_IDLE; 
      end
      
      PH1_IDLE: begin
        if (phase_0) begin
          state <= PH1_0;
          phase_1_running <= 1'b1; 
        end
      end

      PH1_0: begin
        state <= PH1_1;
        phase_1_running <= 1'b1; 
        index <= index + 1; 
      end

      PH1_1: begin
        state <= PH1_RUNNING;
        phase_1_running <= 1'b1; 
        index <= index + 1; 
        acc_en <= 1'b1; 
      end

      PH1_RUNNING: begin
        acc_en <= 1'b1; 
        if (index < POLYPHASE_DEPTH -1) begin
          index <= index + 1;
          phase_1_running <= 1'b1;
        end else begin
          state <= PH1_CLEANUP0;
          phase_1_running <= 1'b0; 
          index <= 0; 
        end
      end

      PH1_CLEANUP0: begin
        acc_en <= 1'b1;
        state <= PH1_CLEANUP1;
      end
      PH1_CLEANUP1: begin
        acc_en <= 1'b0; 
        phase_1_done <= 1'b1; 
        state <= PH0_IDLE;
      end

      default: state <= PH0_IDLE; 

    endcase
  end
end

endmodule
 
