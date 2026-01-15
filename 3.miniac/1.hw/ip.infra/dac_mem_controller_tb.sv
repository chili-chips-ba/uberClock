//==========================================================================
// Testbench for dac_mem_controller
// Focus: Continuous loop generation from memory and data field alignment.
//==========================================================================
`timescale 1ns / 1ps

import signal_types_pkg::*; // Import package for dac_sample_t structure

module dac_mem_controller_tb;

    // Parameters
    localparam ADDR_WIDTH = 11;
    localparam CLK_PERIOD = 15.38; // ~65MHz in nanoseconds

    // Signal definitions for UUT connection
    logic                    clk;
    logic                    rst_n;
    logic                    dac_en_i;
    logic [ADDR_WIDTH-1:0]   dac_len_i;
    logic [ADDR_WIDTH-1:0]   mem_addr_o;
    logic                    mem_rd_en_o;
    logic [31:0]             mem_data_i;
    logic [13:0]             dac_ch0_o;
    logic [13:0]             dac_ch1_o;

    // Unit Under Test (UUT) Instance
    dac_mem_controller #(
        .ADDR_WIDTH(ADDR_WIDTH)
    ) uut (
        .* // Automatic port connection
    );

    // Clock Generation
    initial begin
        clk = 0;
        forever #(CLK_PERIOD/2) clk = ~clk;
    end

    // --- Main Test Sequence ---
    initial begin
        // --- 1. Initialization and Reset ---
        $display("-------------------------------------------------------");
        $display("Starting DAC Memory Controller Simulation");
        $display("-------------------------------------------------------");
        
        rst_n      = 0;
        dac_en_i   = 0;
        dac_len_i  = 0;
        mem_data_i = 32'h0;
        
        #(CLK_PERIOD * 5);
        rst_n = 1; // Release reset
        #(CLK_PERIOD * 2);

        // --- 2. Test RUN State (Looping) ---
        $display("[%0t] Starting continuous loop test...", $time);
        dac_len_i = 11'd4; // Loop through addresses 0, 1, 2, 3
        dac_en_i  = 1;

        // Memory Responder: Simulates BRAM behavior in the background
        fork
            forever begin
                @(posedge clk);
                // Data is packed according to dac_sample_t:
                // {unused1[2], ch1[14], unused0[2], ch0[14]}
                case (mem_addr_o)
                    11'd0: mem_data_i <= {2'b0, 14'd100, 2'b0, 14'd50};
                    11'd1: mem_data_i <= {2'b0, 14'd200, 2'b0, 14'd150};
                    11'd2: mem_data_i <= {2'b0, 14'd300, 2'b0, 14'd250};
                    11'd3: mem_data_i <= {2'b0, 14'd400, 2'b0, 14'd350};
                    default: mem_data_i <= 32'h0;
                endcase
            end
        join_none 

        // Let the controller loop for 3 full cycles (3 * 4 = 12 clock cycles)
        repeat(12) @(posedge clk); 

        // --- 3. Shutdown Test ---
        $display("[%0t] Disabling DAC controller...", $time);
        dac_en_i = 0;
        #(CLK_PERIOD * 5);
        
        $display("-------------------------------------------------------");
        $display("Simulation finished successfully.");
        $display("-------------------------------------------------------");
        $finish;
    end

    // Console Monitoring
    initial begin
        $monitor("Time=%0t | State=%s | Addr=%0d | Ch1_Out=%0d | Ch0_Out=%0d", 
                 $time, uut.state_reg.name(), mem_addr_o, dac_ch1_o, dac_ch0_o);
    end

endmodule
