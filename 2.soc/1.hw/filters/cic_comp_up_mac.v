module cic_comp_up_mac #(
  parameter DW              = 16,                  // input/output data width
  parameter CW              = 19,                  // coefficient width
  parameter POLYPHASE_DEPTH = 60,                  // taps per phase
  parameter DEPTH           = 64,                  // circular buffer depth
  parameter COEFF_INIT_FILE = "coeffs.mem",        // coefficient memory init
  localparam ADDR_D = $clog2(DEPTH),                // bits to index DATA RAM
  localparam ADDR_C = $clog2(2*POLYPHASE_DEPTH),    // bits to index COEFF RAM
  localparam OFFSET = POLYPHASE_DEPTH - 1           // read-pointer offset
)(
  input  wire               clk,          // 65 MHz system clock
  input  wire               clk_enable,   // 20 kHz tick
  input  wire               reset,        // synchronous reset
  input  wire signed [DW-1:0] filter_in,  // new sample in
  output wire signed [DW-1:0] filter_out, // interpolated sample out
  output wire               ce_out        // 10 kHz strobe
);


  // ce_out strobe generation
  reg [1:0] cur_count;
  always @(posedge clk or posedge reset) begin
    if (reset)           cur_count <= 2'd0;
    else if (clk_enable) cur_count <= (cur_count == 2'd1) ? 2'd0 : cur_count + 1;
  end
  wire phase_1 = (cur_count == 2'd1) && clk_enable;
  assign ce_out = phase_1;


  // coefficient RAM
  reg signed [CW-1:0] coeffs [0:2*POLYPHASE_DEPTH-1];
  initial $readmemb(COEFF_INIT_FILE, coeffs);

  // data RAM
  reg signed [DW-1:0] mem [0:DEPTH-1];
  integer i;
  always @(posedge clk or posedge reset) begin
    if (reset) begin
      for (i = 0; i < DEPTH; i = i + 1)
        mem[i] <= {DW{1'b0}};
    end else if (phase_1) begin
      mem[w_ptr] <= filter_in;
    end
  end

  reg [ADDR_D-1:0] w_ptr, r_ptr;
  // write pointer
  always @(posedge clk or posedge reset) begin
    if (reset)        w_ptr <= 0;
    else if (phase_1) w_ptr <= w_ptr + 1;
  end
  // read pointer
  always @(posedge clk or posedge reset) begin
    if (reset)                    r_ptr <= 0;
    else if (phase_1)             r_ptr <= w_ptr - OFFSET;
    else if (clk_enable && ~phase_1) r_ptr <= w_ptr - OFFSET - 1;
    else if (state == PH1_RUN ||
             state == PH2_RUN)    r_ptr <= r_ptr + 1;
  end

  reg signed [DW-1:0] current_sample;
  always @(posedge clk) begin
  if (reset) current_sample <= 0;
    if (state == PH1_RUN || state == PH2_RUN)
      current_sample <= mem[r_ptr];
  end

  // read coefficient
//  reg [ADDR_C-1:0] coeff_addr;
//  always @(posedge clk) begin
//  if (reset) coeff_addr <= 0;
//    if      (state == PH2_RUN) coeff_addr <= OFFSET - mac_idx;
//    else if (state == PH1_RUN) coeff_addr <= 2*OFFSET + 1 - mac_idx;
//  end

  reg signed [CW-1:0] current_coeff;
  always @(posedge clk) begin
  if (reset) current_coeff <= 0;
    if (state == PH1_RUN) current_coeff <= coeffs[OFFSET - mac_idx];
    else if (state == PH2_RUN) current_coeff <= coeffs[2*OFFSET + 1 - mac_idx];

  end

  // multiply
//  reg signed [DW+CW-1:0] product;
//  always @(posedge clk) begin
//    if (state == PH1_RUN || state == PH2_RUN)
//      product <= current_sample * current_coeff;
//  end

  wire signed [DW+CW-1:0] product;
  assign product = (state == PH1_RUN || state == PH2_RUN) ? current_sample * current_coeff: 0;

  // accumulator
  reg signed [36:0] acc;
  reg               clear_acc, acc_en;
  always @(posedge clk or posedge reset) begin
    if      (reset)     acc <= 0;
    else if (clear_acc) acc <= 0;
    else if (acc_en)    acc <= acc + product;
  end

  // FSM and flags
  localparam IDLE      = 2'd0,
             PH1_RUN   = 2'd1,
             PH1_OUT = 2'd2,
             PH2_RUN   = 2'd3;

  reg [1:0] state;
  reg [5:0] mac_idx;
  reg       phase1_done, phase2_done;

  always @(posedge clk or posedge reset) begin
    if (reset) begin
      state         <= IDLE;
      mac_idx       <= 0;
      clear_acc     <= 0;
      acc_en        <= 0;
      phase1_done   <= 0;
      phase2_done   <= 0;
    end else begin
      clear_acc   <= 0;
      acc_en      <= 0;
      phase1_done <= 0;
      phase2_done <= 0;

      case (state)
        IDLE: if (phase_1) begin
          clear_acc <= 1;
          mac_idx   <= 0;
          state     <= PH1_RUN;
        end

        PH1_RUN: begin
          acc_en <= 1;
          if (mac_idx == POLYPHASE_DEPTH-1) begin
            phase1_done <= 1;
            clear_acc   <= 1;
            mac_idx     <= 0;
            state       <= PH1_OUT;
          end else
            mac_idx <= mac_idx + 1;
        end

        PH1_OUT: if (clk_enable) begin
          clear_acc <= 1;
          mac_idx   <= 0;
          state     <= PH2_RUN;
        end

        PH2_RUN: begin
          acc_en <= 1;
          if (mac_idx == POLYPHASE_DEPTH-1) begin
            phase2_done <= 1;
            state       <= IDLE;
          end else
            mac_idx <= mac_idx + 1;
        end
      endcase
    end
  end

  // delayed output register
  reg signed [36:0] acc_registered;
  reg               snapshot_taken;
  reg signed [DW-1:0] filter_out_reg;

  function automatic [DW-1:0] round_sat(input signed [36:0] x);
    begin
      round_sat = (x[36] == 1'b0 & x[35:30] != 6'b000000) ? 16'b0111111111111111 :
      (x[36] == 1'b1 && x[35:30] != 6'b111111) ? 16'b1000000000000000 : $signed({1'b0, x[30:15]} + (x[36] & |x[14:0]));
    end
  endfunction


  always @(posedge clk or posedge reset) begin
    if (reset) begin
      filter_out_reg       <= 0;
      snapshot_taken <= 0;
    end else begin
      // capture whenever a phase finishes
      if (phase1_done || phase2_done) begin
        acc_registered <= acc;
        snapshot_taken <= 1;
      end
      // following clk_enable drives output
      if (clk_enable && snapshot_taken) begin
        filter_out_reg       <= round_sat(acc_registered);
        snapshot_taken <= 0;
      end
    end
  end

  assign filter_out = filter_out_reg;

endmodule
