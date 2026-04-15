`timescale 1ns/1ps

module tb_cordic;

  // Parameters (must match those of the top module)
  parameter IW       = 13;
  parameter OW       = 13;
  parameter NSTAGES  = 16;
  parameter WW       = 16;
  parameter PW       = 20;

  // For a 65 MHz clock, period = ~15.3846 ns.
  // (Your simulator may allow real delays with the given timescale.)
  real CLOCK_PERIOD = 15.3846;

  // Testbench signals
  reg                     i_clk;
  reg                     i_reset;
  reg                     i_ce;
  reg signed [IW-1:0]     i_xval;
  reg signed [IW-1:0]     i_yval;
  reg       [PW-1:0]      i_phase;
  reg                     i_aux;

  wire signed [OW-1:0]    o_xval;
  wire signed [OW-1:0]    o_yval;
  wire                    o_aux;

  // Instantiate the top-level CORDIC module.
  cordic #(
    .IW(IW),
    .OW(OW),
    .NSTAGES(NSTAGES),
    .WW(WW),
    .PW(PW)
  ) dut (
    .i_clk(i_clk),
    .i_reset(i_reset),
    .i_ce(i_ce),
    .i_xval(i_xval),
    .i_yval(i_yval),
    .i_phase(i_phase),
    .i_aux(i_aux),
    .o_xval(o_xval),
    .o_yval(o_yval),
    .o_aux(o_aux)
  );

  // Clock generation: 65 MHz clock.
  initial begin
    i_clk = 0;
    forever #(CLOCK_PERIOD/2) i_clk = ~i_clk;
  end

  // Stimulus process
  initial begin
    // Initialize inputs
    i_reset = 1;
    i_ce    = 0;
    i_xval  = 2000; // A moderate amplitude value; adjust as needed.
    i_yval  = 0;
    i_phase = 0;
    i_aux   = 0;

    // Hold reset for 3 clock cycles.
    #(CLOCK_PERIOD*3);
    i_reset = 0;
    i_ce    = 1;

    // Now, drive the phase input.
    // Using a phase increment of 161319 per cycle gives:
    //   f_out = (PHASE_INC * f_clk) / 2^20 ≈ 10 MHz.
    // We run for 50 clock cycles (this, plus pipeline latency, shows ~3 periods).
    repeat (1000) begin
      #(CLOCK_PERIOD);
//      i_phase = i_phase + 161319;
      i_phase = i_phase + 16131; //1MHz
    end

    // Wait a few more cycles to capture the outputs, then finish.
    #(CLOCK_PERIOD*10);
    $finish;
  end

endmodule
