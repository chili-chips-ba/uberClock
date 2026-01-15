//==========================================================================
// Testbench for dac_mem_controller
// Focus: Continuous loop generation from memory and data field alignment.
//==========================================================================
`timescale 1ns / 1ps

import signal_types_pkg::*;

module dac_mem_controller_tb;

    // Parameters
    localparam ADDR_WIDTH = 11;
    localparam CLK_PERIOD = 15.38;

    // Signal definitions
    logic                    clk;
    logic                    rst_n;
    logic                    dac_en_i;
    logic [ADDR_WIDTH-1:0]   dac_len_i;
    logic [ADDR_WIDTH-1:0]   mem_addr_o;
    logic                    mem_rd_en_o;
    dac_sample_t             mem_data_i; // Strukturni tip
    logic [13:0]             dac_ch0_o;
    logic [13:0]             dac_ch1_o;

    // Unit Under Test (UUT) Instance
    dac_mem_controller #(
        .ADDR_WIDTH(ADDR_WIDTH)
    ) uut (
        .* );

    // Clock Generation
    initial begin
        clk = 0;
        forever #(CLK_PERIOD/2) clk = ~clk;
    end

    // --- Main Test Sequence ---
    initial begin
        // Initialization and Reset
        rst_n      = 0;
        dac_en_i   = 0;
        dac_len_i  = 0;
        mem_data_i = '0;
        
        #(CLK_PERIOD * 5);
        rst_n = 1;
        #(CLK_PERIOD * 2);

        // Test RUN State (Looping)
        dac_len_i = 11'd4;
        dac_en_i  = 1;

        // Memory Responder using structural assignments
        fork
            forever begin
                @(posedge clk);
                mem_data_i = '0; // Clear unused bits
                case (mem_addr_o)
                    11'd0: begin mem_data_i.dac_ch1 = 14'd100; mem_data_i.dac_ch0 = 14'd50;  end
                    11'd1: begin mem_data_i.dac_ch1 = 14'd200; mem_data_i.dac_ch0 = 14'd150; end
                    11'd2: begin mem_data_i.dac_ch1 = 14'd300; mem_data_i.dac_ch0 = 14'd250; end
                    11'd3: begin mem_data_i.dac_ch1 = 14'd400; mem_data_i.dac_ch0 = 14'd350; end
                    default: mem_data_i = '0;
                endcase
            end
        join_none 

        repeat(12) @(posedge clk); 

        // Shutdown Test
        dac_en_i = 0;
        #(CLK_PERIOD * 5);
        $finish;
    end

    // Console Monitoring
    initial begin
        $monitor("Time=%0t | State=%s | Addr=%0d | Ch1_Out=%0d | Ch0_Out=%0d", 
                 $time, uut.state_reg.name(), mem_addr_o, dac_ch1_o, dac_ch0_o);
    end

endmodule