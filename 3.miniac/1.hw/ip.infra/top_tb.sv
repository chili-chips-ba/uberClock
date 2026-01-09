//==========================================================================
// Testbench: top_tb 
// Description: Verification environment for the ADC memory controller.
//==========================================================================
`timescale 1ns / 1ps

module top_tb;
    import soc_pkg::*; 

    // Parameters 
    localparam CLK_PERIOD = 15.385; // 65 MHz clock cycle
    localparam NUM_SAMPLES = 4096;
    localparam ADDR_START  = 13'h400;
    localparam NUM_WORDS_DMEM = 8192;

    // Signals 
    logic        sys_clk;
    logic        sys_rst_n;
    logic        csr_start_i;
    wire         csr_done_o;
    
    // 12-bit ADC channels (matching hardware specifications)
    logic [11:0] ad9238_data_ch0;
    logic [11:0] ad9238_data_ch1;
    
    // Packed 32-bit data word sent to the controller
    wire [31:0]  adc_sample_in;
    
    // Internal signals: Controller to RAM interface
    wire         adc_we;
    wire [31:0]  adc_data;
    wire [12:0]  adc_addr;

    // Verification Helper Variables
    integer sample_count = 0;
    integer i, errors = 0;
    logic [31:0] expected_mem [0:NUM_SAMPLES-1]; 

    // DATA PACKING: Top-level simulation logic
    // Format: [ 4'b0 | CH1(12b) | 4'b0 | CH0(12b) ]
    assign adc_sample_in = {4'h0, ad9238_data_ch1, 4'h0, ad9238_data_ch0};

    // SoC Interface Instance (for RAM Port 1)
    soc_if bus_dmem (
        .clk    (sys_clk),
        .arst_n (sys_rst_n)
    );
    // Disable CPU port for this specific test sequence
    assign bus_dmem.vld = 1'b0; 

    // RAM Instance
    soc_ram #(
        .NUM_WORDS (NUM_WORDS_DMEM)
    ) u_dmem (
        .bus      (bus_dmem.SLV), 
        .adc_clk  (sys_clk),      
        .adc_we   (adc_we),
        .adc_data (adc_data),
        .adc_addr (adc_addr)
    );

    // ADC Controller Instance (UUT - Unit Under Test)
    adc_mem_controller uut (
        .sys_clk        (sys_clk),
        .sys_rst_n      (sys_rst_n),
        .adc_sample_in (adc_sample_in),
        .csr_start_i   (csr_start_i),
        .csr_done_o    (csr_done_o),
        .adc_we_o      (adc_we),
        .adc_data_o    (adc_data),
        .adc_addr_o    (adc_addr)
    );

    // Clock Generation
    always begin
        sys_clk = 1'b0;
        #(CLK_PERIOD / 2) sys_clk = 1'b1;
        #(CLK_PERIOD / 2);
    end

    // MAIN TEST SEQUENCE
    initial begin
        // Initialization
        sys_rst_n = 1'b0;
        csr_start_i = 1'b0;
        ad9238_data_ch0 = 12'hAAA;
        ad9238_data_ch1 = 12'h555;
        sample_count = 0;

        // 1. Reset Phase
        repeat (10) @(posedge sys_clk);
        sys_rst_n = 1'b1;
        $display("[%0t] System reset released.", $time);

        // 2. Acquisition Start
        @(posedge sys_clk);
        csr_start_i = 1'b1;
        @(posedge sys_clk);
        csr_start_i = 1'b0;
        $display("[%0t] Start pulse issued to controller.", $time);

        // 3. Dynamic Data Stream Simulation
        // Wait for the controller to trigger the Write Enable signal
        wait(adc_we == 1'b1);
        
        while (sample_count < NUM_SAMPLES) begin
            // 1. Wait for clock edge for the controller to sample current data
            @(posedge sys_clk);
            
            if (adc_we) begin
                // 2. Capture the actual data present on lines for verification
                expected_mem[sample_count] = adc_sample_in;
                sample_count++;
                
                // 3. Prepare data for the next clock cycle
                ad9238_data_ch0 <= ad9238_data_ch0 + 1;
                ad9238_data_ch1 <= ad9238_data_ch1 + 1; 
            end
        end

        // 4. End of Acquisition
        wait(csr_done_o == 1'b1);
        $display("[%0t] Acquisition completed. Waiting for stabilization...", $time);
        repeat(10) @(posedge sys_clk);

        // 5. Automated Self-Checking Verification
        $display("-------------------------------------------------------");
        $display("STARTING RAM VERIFICATION (Range: 0x%h - 0x%h)", ADDR_START, ADDR_START + NUM_SAMPLES - 1);
        
        for (i = 0; i < NUM_SAMPLES; i = i + 1) begin
            if (u_dmem.mem[ADDR_START + i] !== expected_mem[i]) begin
                $display("ERROR: Addr 0x%h | Expected 0x%h | Found 0x%h", 
                         ADDR_START + i, expected_mem[i], u_dmem.mem[ADDR_START + i]);
                errors++;
            end
        end

        if (errors == 0) 
            $display("SUCCESS: VERIFICATION PASSED! All %0d samples match perfectly.", NUM_SAMPLES);
        else 
            $display("FAILURE: VERIFICATION FAILED! Total errors found: %0d", errors);
        $display("-------------------------------------------------------");

        repeat (20) @(posedge sys_clk);
        $finish;
    end

    // Monitoring
    initial begin
        $monitor("[%0t] State: %s | Addr: 0x%h | WE: %b | CH0: 0x%h | CH1: 0x%h", 
                 $time, uut.acq_state_r.name(), adc_addr, adc_we, ad9238_data_ch0, ad9238_data_ch1);
    end

endmodule