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
    input  logic                   dac_en_i,   // Enable continuous generation
    input  logic [ADDR_WIDTH-1:0]  dac_len_i,  // Loop length (number of samples)

    // BRAM Interface (Port B)
    output logic [ADDR_WIDTH-1:0]  mem_addr_o, // Read address
    output logic                   mem_rd_en_o,// Read enable
    input  logic [31:0]            mem_data_i, // Data from memory

    // DAC Hardware Interface
    output logic [13:0]            dac_ch0_o,  // 14-bit Channel 0 Output
    output logic [13:0]            dac_ch1_o   // 14-bit Channel 1 Output
);

    // Internal Signal Definitions
    typedef enum logic {
        ST_IDLE = 1'b0,
        ST_RUN  = 1'b1
    } state_t;

    state_t state_reg, state_next;

    dac_sample_t dac_data_struct;
    logic [ADDR_WIDTH-1:0] addr_cnt;

    // Map memory input to the internal structure
    assign dac_data_struct = dac_sample_t'(mem_data_i);

    //--------------------------------------------------------------------------
    // State Machine & Counter Logic
    //--------------------------------------------------------------------------
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state_reg <= ST_IDLE;
            addr_cnt  <= '0;
        end 
        else begin
            state_reg <= state_next;
            
            if (state_reg == ST_RUN) begin
                // Reset address if length is reached, otherwise increment
                if (addr_cnt >= (dac_len_i - 1)) begin
                    addr_cnt <= '0;
                end 
                else begin
                    addr_cnt <= addr_cnt + 1'b1;
                end
            end 
            else begin
                addr_cnt <= '0;
            end
        end
    end

    // State Transition Logic
    always_comb begin
        state_next = state_reg;
        case (state_reg)
            ST_IDLE: if (dac_en_i && dac_len_i > 0) state_next = ST_RUN;
            ST_RUN:  if (!dac_en_i)                state_next = ST_IDLE;
            default:                               state_next = ST_IDLE;
        endcase
    end

    //--------------------------------------------------------------------------
    // Output Assignments
    //--------------------------------------------------------------------------
    assign mem_addr_o  = addr_cnt;
    assign mem_rd_en_o = (state_reg == ST_RUN);

    // Mux output: send sampled data when running, otherwise send zero
    assign dac_ch0_o = (state_reg == ST_RUN) ? dac_data_struct.dac_ch0 : 14'h0;
    assign dac_ch1_o = (state_reg == ST_RUN) ? dac_data_struct.dac_ch1 : 14'h0;

endmodule
