//==========================================================================
// Copyright (C) 2023 Chili.CHIPS*ba
//--------------------------------------------------------------------------
//                     PROPRIETARY INFORMATION
//
// The information contained in this file is the property of CHILI CHIPS LLC.
// Except as specifically authorized in writing by CHILI CHIPS LLC, the holder
// of this file: (1) shall keep all information contained herein confidential;
// and (2) shall protect the same in whole or in part from disclosure and
// dissemination to all third parties; and (3) shall use the same for operation
// and maintenance purposes only.
//--------------------------------------------------------------------------
// Description:
// DPRAM Write Controller for ADC Data.
// Implements a 4096-word (4K) acquisition sequence, writing 32-bit ADC samples
// to the assigned memory space (Port 2). Initiated and reset via CPU CSR.
//==========================================================================

`timescale 1ns / 1ps

import signal_types_pkg::*;

module adc_mem_controller (
    // Clock & Reset
    input  logic	sys_clk,
    input  logic	sys_rst_n,

    // ADC Data Input
    input adc_sample_t	adc_sample_in,

    // CPU Control Register (CSR) Interface
    input  logic	csr_start_i, // Start trigger pulse
    output logic	csr_done_o,  // Status flag: High when buffer is full

    // DPRAM Interface (Write-only port)
    output logic	adc_we_o,    // Memory Write Enable
    output adc_sample_t	adc_data_o,  // Data to be stored
    output logic [12:0]	adc_addr_o   // Target memory address
);

    // Address Parameters
    localparam ADDR_BITS     = 13;      // 8192 words (address range 0 to 8191)
    localparam ADDR_START    = 13'h400; // Start offset (Word address 1024)
    localparam ADDR_SPAN     = 13'h1000;// Buffer size (4096 words)
    localparam ADDR_STOP_AT  = ADDR_START + ADDR_SPAN - 1; // End address (13'h13FF)

    adc_sample_t packed_sample_w;

    // State Machine Definition
    typedef enum logic [1:0] {
        IDLE,       // Wait for start trigger (default)
        RUNNING,    // Data acquisition in progress
        DONE        // Buffer full, wait for trigger release
    } acq_state_t;

    acq_state_t acq_state_r;

    // Internal Registers
    logic [ADDR_BITS-1:0] write_addr_r; // Current write address pointer
    logic                  adc_we_r;     // Internal Write Enable register
    logic                  csr_done_r;   // Internal Done flag status

    // Helper signal to detect the final write operation in the buffer
    logic end_of_buffer_write;
    assign end_of_buffer_write = (write_addr_r == ADDR_STOP_AT) && (acq_state_r == RUNNING);
    
    // ====================================================================
    // State Machine Transitions
    // ====================================================================
    always @(posedge sys_clk or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            acq_state_r <= IDLE;
        end else begin
            case (acq_state_r)
                IDLE: begin
                    // Transition to RUNNING when CPU issues the START pulse
                    if (csr_start_i) begin
                        acq_state_r <= RUNNING;
                    end
                end
                
                RUNNING: begin
                    // Move to DONE once the last sample is written to RAM
                    if (end_of_buffer_write) begin
                        acq_state_r <= DONE;
                    end
                end
                
                DONE: begin
                    // Wait for CPU to clear the START signal
                    // Transition back to IDLE to allow for a new acquisition cycle
                    if (csr_start_i == 0) begin
                        acq_state_r <= IDLE;
                    end
                end

                default: acq_state_r <= IDLE;
            endcase
        end
    end

    // ====================================================================
    // Data Path and Control Logic
    // ====================================================================
    always @(posedge sys_clk or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            write_addr_r <= ADDR_START;
            adc_we_r     <= 1'b0;
            csr_done_r   <= 1'b0;
        end else begin
            
            // Default signal states
            adc_we_r <= 1'b0;
            
            // Initialization Logic (IDLE state)
            if (acq_state_r == IDLE) begin
                csr_done_r <= 1'b0; // Clear the DONE flag
                if (csr_start_i) begin
                    write_addr_r <= ADDR_START; // Initialize address on START
                end
            end
            
            // Memory Write Management (RUNNING state)
            if (acq_state_r == RUNNING) begin
                adc_we_r <= 1'b1; // Activate Write Enable
                
                // Increment address pointer only after a valid write occurred
                if (adc_we_r) begin 
                    write_addr_r <= write_addr_r + 1'b1;
                end

                // Terminate write operations as soon as we reach the buffer limit
                if (end_of_buffer_write) begin
                    adc_we_r <= 1'b0; 
                end
                
            end else if (acq_state_r == DONE) begin
                // Assert the DONE status flag
                csr_done_r <= 1'b1;
                // Reset address pointer to START for the next cycle (non-writing)
                write_addr_r <= ADDR_START;
            end
        end
    end

    // --- Data Mapping to Structure ---
    assign packed_sample_w = adc_sample_in;

    // --- Output Assignments ---
    assign adc_we_o   = adc_we_r;
    assign adc_data_o = 32'(packed_sample_w);
    assign adc_addr_o = write_addr_r;
    assign csr_done_o = csr_done_r;

endmodule
