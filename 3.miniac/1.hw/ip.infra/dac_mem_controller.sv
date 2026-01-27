//==========================================================================
// Copyright (C) 2024-2025 Chili.CHIPS*ba
//--------------------------------------------------------------------------
//                         PROPRIETARY INFORMATION
//
// The information contained in this file is the property of CHILI CHIPS LLC.
// Except as specifically authorized in writing by CHILI CHIPS LLC, the holder
// of this file: (1) shall keep all information contained herein confidential;
// and (2) shall protect the same in whole or in part from disclosure and
// dissemination to all third parties; and (3) shall use the same for operation
// and maintenance purposes only.
//--------------------------------------------------------------------------
// Description:
// DAC Continuous Generation Controller.
// This module reads dual-channel 14-bit DAC samples from BRAM and streams 
// them to the DAC hardware. It operates in a continuous loop with a 
// programmable length (LEN) defined via CSR.
//==========================================================================

`timescale 1ns / 1ps

import signal_types_pkg::*;

module dac_mem_controller #(
    parameter ADDR_WIDTH = 11
)(
    input  logic                   clk,        // System clock (65MHz)
    input  logic                   rst_n,      // Active-low reset

    // CSR Control Interface
    input  logic                   dac_en0_i,  // Enable continuous generation on DAC channel 1
    input  logic                   dac_en1_i,  // Enable continuous generation on DAC channel 2
    input  logic                   dac_mode0_i, // 0: Continuous, 1: Snapshot (One-shot)
    input  logic                   dac_mode1_i, // 0: Continuous, 1: Snapshot (One-shot)
    input  logic [ADDR_WIDTH-1:0]  dac_len0_i, // Loop length (number of samples) for DAC channel 1
    input  logic [ADDR_WIDTH-1:0]  dac_len1_i, // Loop length (number of samples) for DAC channel 2


    // BRAM Interface (Port B)
    output logic [ADDR_WIDTH-1:0]  mem_addr0_o,	// Read address for channel 1
    output logic [ADDR_WIDTH-1:0]  mem_addr1_o,	// Read address for channel 2
    
    output logic                   mem_rd_en_o,	// Read enable
    
    input  dac_sample_t            mem_data_i,	// Data from memory (strukturni tip)

    // DAC Hardware Interface
    output logic [13:0]            dac_ch0_o,  // 14-bit Channel 0 Output
    output logic [13:0]            dac_ch1_o   // 14-bit Channel 1 Output
);

    // Internal Signal Definitions
    typedef enum logic {
        IDLE = 1'b0,
        RUN  = 1'b1
    } state_t;
    
    // Edge detection logic
    logic dac_en0_prev, dac_en1_prev;
    logic dac_en0_edge, dac_en1_edge;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            dac_en0_prev <= 1'b0;
            dac_en1_prev <= 1'b0;
        end else begin
            dac_en0_prev <= dac_en0_i;
            dac_en1_prev <= dac_en1_i;
        end
    end

    assign dac_en0_edge = dac_en0_i && !dac_en0_prev;
    assign dac_en1_edge = dac_en1_i && !dac_en1_prev;

    state_t state0_reg, state1_reg;
    logic [ADDR_WIDTH-1:0] addr0_cnt, addr1_cnt;

    //--------------------------------------------------------------------------
    // Channel 0 - State Machine & Counter Logic
    //--------------------------------------------------------------------------
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state0_reg <= IDLE;
            addr0_cnt  <= '0;
        end 
        else begin
            case (state0_reg)
                IDLE: begin
                    addr0_cnt <= '0;
                    if (dac_en0_edge && dac_len0_i > 0) state0_reg <= RUN;
                end
                RUN: begin
                    if (!dac_en0_i) begin
                        state0_reg <= IDLE;
                    end else begin
                        if (addr0_cnt >= (dac_len0_i - 1)) begin
                            addr0_cnt  <= '0;                    
                            if (dac_mode0_i) begin
                                state0_reg <= IDLE; // Snapshot: stop and wait for new edge
                            end
                        end else begin
                            addr0_cnt <= addr0_cnt + 1'b1;
                        end
                    	end
                end
            endcase
        end
    end
    
    //--------------------------------------------------------------------------
    // Channel 1 - State Machine & Counter Logic
    //--------------------------------------------------------------------------
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state1_reg <= IDLE;
            addr1_cnt  <= '0;
        end else begin
            case (state1_reg)
                IDLE: begin
                    addr1_cnt <= '0;
                    if (dac_en1_edge && dac_len1_i > 0) state1_reg <= RUN;
                end
                RUN: begin
                    if (!dac_en1_i) begin
                        state1_reg <= IDLE;
                    end else begin
                        if (addr1_cnt >= (dac_len1_i)) begin
                            addr1_cnt  <= '0;                    
                            if (dac_mode1_i) begin
                                state1_reg <= IDLE; // Snapshot: stop and wait for new edge
                            end 
                        end else begin
                            addr1_cnt <= addr1_cnt + 1'b1;
                        end
                    end
                end
            endcase
        end
    end

    //--------------------------------------------------------------------------
    // Output Assignments
    //--------------------------------------------------------------------------
    assign mem_addr0_o = addr0_cnt;
    assign mem_addr1_o = addr1_cnt;
    
    // Read enable is active if either channel is running
    assign mem_rd_en_o = (state0_reg == RUN) || (state1_reg == RUN);	

    // Mux output: direct access to structure fields
    assign dac_ch0_o = (state0_reg == RUN) ? mem_data_i.dac_ch0 : 14'h0;
    assign dac_ch1_o = (state1_reg == RUN) ? mem_data_i.dac_ch1 : 14'h0;
    
endmodule
