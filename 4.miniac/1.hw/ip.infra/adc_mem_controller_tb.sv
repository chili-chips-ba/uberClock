//==========================================================================
// Testbench for adc_mem_controller
// Focus: Monitoring Finite State Machine (FSM) transitions and RAM write signals.
//==========================================================================
`timescale 1ns / 1ps

import signal_types_pkg::*;

module adc_mem_controller_tb;

    // Testbench Parameters
    localparam CLK_PERIOD = 15.385; // ~65 MHz
    localparam NUM_SAMPLES = 4096;  // 4K samples buffer size
    localparam ADDR_START  = 13'h400; 

    // Signals for UUT (Unit Under Test)
    logic           sys_clk;
    logic           sys_rst_n;
    adc_sample_t    adc_sample_in; // Promijenjeno u strukturu
    logic           csr_start_i;
    wire            csr_done_o;
    wire            adc_we_o;
    adc_sample_t    adc_data_o;   // Promijenjeno u strukturu
    wire [12:0]     adc_addr_o;
    
    // Simulation counter
    integer        sample_count;
    
    // UUT Instance
    adc_mem_controller uut (
        .sys_clk       (sys_clk),
        .sys_rst_n     (sys_rst_n),
        .adc_sample_in (adc_sample_in),
        .csr_start_i   (csr_start_i),
        .csr_done_o    (csr_done_o),
        .adc_we_o      (adc_we_o),
        .adc_data_o    (adc_data_o),
        .adc_addr_o    (adc_addr_o)
    );

    // Clock Generation
    always begin
        sys_clk = 1'b0;
        #(CLK_PERIOD / 2) sys_clk = 1'b1;
        #(CLK_PERIOD / 2);
    end

    // --- Signal Monitoring ---
    // Direct FSM state monitoring via hierarchical path: uut.acq_state_r
    initial begin
        $monitor("[%0t] FSM State: %s | WE=%b | DONE=%b | ADDR=0x%h | CH1=%d | CH0=%d | IN=0x%h", 
            $time, 
            uut.acq_state_r.name(), 
            adc_we_o, 
            csr_done_o, 
            adc_addr_o, 
            uut.packed_sample_w.adc_ch1, 
            uut.packed_sample_w.adc_ch0, 
            adc_sample_in);
    end

    // --- Test Sequence ---
    initial begin
        $display("-------------------------------------------------------");
        $display("Starting ADC Memory Controller Simulation");
        $display("-------------------------------------------------------");
        
        $dumpfile("adc_mem_controller.vcd");
        $dumpvars(0, adc_mem_controller_tb);
        
        // Initial values
        sys_rst_n     = 1'b0; // Active-low reset
        csr_start_i   = 1'b0;
        adc_sample_in = 32'hFEED_FEED; 
        sample_count  = 0;
        
        // 1. Reset Phase
        @(posedge sys_clk);
        @(posedge sys_clk);
        sys_rst_n = 1'b1; // Release reset
        $display("[%0t] Reset released. Expected state: IDLE", $time);
        
        // 2. Start Acquisition (Pulse)
        @(posedge sys_clk);
        csr_start_i = 1'b1; // Issue START pulse
        $display("[%0t] START pulse issued: csr_start_i = 1", $time);

        @(posedge sys_clk);
        csr_start_i = 1'b0; // Release pulse
        $display("[%0t] START pulse released: csr_start_i = 0. Expected state: RUNNING", $time);

        // 3. Data Acquisition Phase - 4096 Samples
        while (sample_count < NUM_SAMPLES) begin
            
            // Pack data using structure fields to simulate real ADC input
            adc_sample_in.adc_unused1 = 4'b0;
            adc_sample_in.adc_ch1     = 12'(sample_count + 100);
            adc_sample_in.adc_unused0 = 4'b0;
            adc_sample_in.adc_ch0     = 12'(sample_count); 
            
            @(posedge sys_clk); // Wait for clock edge (Write occurs here)
            
            sample_count++;
        end

        // 4. Verification of DONE state
        
        // Set dummy value (should not be written to RAM since buffer is full)
        adc_sample_in = 32'(NUM_SAMPLES); 

        @(posedge sys_clk); // Cycle after the final write
        
        $display("[%0t] Acquisition Finished. Written %0d samples. Expected state: DONE", $time, NUM_SAMPLES);
        
        // Stay in DONE state for a few cycles
        repeat (3) @(posedge sys_clk);
        
        // 5. Restart Test
        
        // New START pulse
        @(posedge sys_clk);
        csr_start_i = 1'b1;
        
        @(posedge sys_clk);
        csr_start_i = 1'b0; 
        
        $display("[%0t] Second START issued. Expected state: RUNNING", $time);
        
        // Let the second run execute for a few cycles
        repeat (5) @(posedge sys_clk);


        // 6. Simulation Cleanup
        repeat (10) @(posedge sys_clk);
        
        $display("-------------------------------------------------------");
        $display("SIMULATION FINISHED: Check FSM log above for details.");
        $display("-------------------------------------------------------");
        
        $stop;
    end

endmodule
