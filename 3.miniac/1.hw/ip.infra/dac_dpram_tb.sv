`timescale 1ns / 1ps

module dac_dpram_tb();
    //-------------------------------------------------------------------------
    // Signal Definitions
    //-------------------------------------------------------------------------
    logic clk = 0;
    logic rst_n = 0;
    
    // CPU-side interface signals (simulating CSR bus behavior)
    logic        req   = 0;     // Request strobe
    logic        wr    = 0;     // Write enable (1=Write, 0=Read)
    logic [12:0] addr  = 0;     // Byte address from CPU
    logic [31:0] wdata = 0;     // Data to be written to memory
    logic [31:0] rdata;         // Data read from memory

    // Controller configuration signals (from CSR registers)
    logic dac_en = 0;           // Enable continuous playback
    logic [10:0] dac_len = 0;   // Number of samples to loop

    // Final DAC outputs
    logic [13:0] dac_ch0, dac_ch1;

    // Clock Generation: ~65MHz (Period ~15.4ns)
    always #7.7 clk = ~clk;

    // Internal connections between Memory and Controller
    logic [10:0] ctrl_addr;     // Address driven by DAC controller
    logic [31:0] mem_to_ctrl;   // 32-bit word from memory to controller

    //-------------------------------------------------------------------------
    // Device Under Test (DUT) Instantiations
    //-------------------------------------------------------------------------
    
    // Dual-Port RAM for DAC Samples
    dac_dpram u_dac_dpram (
        // Port 1: CPU Interface (Simulated)
        .clk1  (clk), 
        .we1   (req & wr), 
        .addr1 (addr[12:2]),    // Convert byte address to word address
        .din1  (wdata), 
        .dout1 (rdata),
        
        // Port 2: DAC Controller Interface
        .clk2  (clk), 
        .we2   (1'b0),          // Port 2 is read-only for the controller
        .addr2 (ctrl_addr), 
        .din2  (32'h0), 
        .dout2 (mem_to_ctrl)
    );

    // DAC Memory Controller
    dac_mem_controller u_ctrl (
        .clk        (clk), 
        .rst_n      (rst_n),
        .dac_en_i   (dac_en), 
        .dac_len_i  (dac_len),
        .mem_addr_o (ctrl_addr), 
        .mem_data_i (mem_to_ctrl),
        .dac_ch0_o  (dac_ch0), 
        .dac_ch1_o  (dac_ch1)
    );

    //-------------------------------------------------------------------------
    // Test Procedure
    //-------------------------------------------------------------------------
    initial begin
        // System Reset
        #20 rst_n = 1;
        #20;

        // Step 1: Write first sample to DPRAM Address 0
        // Data format: [31:16] Channel 1, [15:0] Channel 0
        @(posedge clk);
        req <= 1; wr <= 1; addr <= 13'h0; wdata <= 32'h0001_2002; 
        @(posedge clk);
        req <= 0; wr <= 0;

        // Step 2: Write second sample to DPRAM Address 1 (Byte offset 0x4)
        @(posedge clk);
        req <= 1; wr <= 1; addr <= 13'h4; wdata <= 32'h0003_4004; 
        @(posedge clk);
        req <= 0; wr <= 0;

        // Step 3: Trigger DAC Controller
        // Set loop length to 2 and enable the output
        #50 dac_en <= 1; dac_len <= 2;

        // Observe the looping behavior
        #500;
        
        $display("Simulation finished. Check waveforms for dac_ch0 and dac_ch1.");
        $finish;
    end
endmodule