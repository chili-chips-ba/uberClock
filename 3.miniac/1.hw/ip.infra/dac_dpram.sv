//==========================================================================
// Copyright (C) 2023 Chili.CHIPS*ba
//--------------------------------------------------------------------------
//                      PROPRIETARY INFORMATION
//
// The information contained in this file is the property of CHILI CHIPS LLC.
// Except as specifically authorized in writing by CHILI CHIPS LLC, the holder
// of this file: (1) shall keep all information contained herein confidential;
// and (2) shall protect the same in whole or in part from disclosure and
// dissemination to all third parties; and (3) shall use the same for operation
// and maintenance purposes only.
//--------------------------------------------------------------------------
// Description:
//   True Dual-Port RAM (TDPRAM) for DAC waveform storage.
//   Port 1: Connected to the CPU via CSR External interface for waveform loading.
//   Port 2: Connected to the DAC Controller for continuous signal generation.
//   This memory is inferred as FPGA Block RAM (BRAM).
//==========================================================================
`timescale 1ns / 1ps

module dac_dpram #(
    parameter DATA_WIDTH = 16,
    parameter ADDR_WIDTH = 11 // 2048 entries
)(
    // Port 1: CPU Interface (Povezuješ na CSR signale u top.sv)
    input  logic                   clk1,
    input  logic                   we1,
    input  logic [ADDR_WIDTH-1:0]  addr1,
    input  logic [DATA_WIDTH-1:0]  din1,
    output logic [DATA_WIDTH-1:0]  dout1,

    // Port 2: DAC Controller Interface
    input  logic                   clk2,
    input  logic                   we2,    // Za DAC fiksno na 0
    input  logic [ADDR_WIDTH-1:0]  addr2,
    input  logic [DATA_WIDTH-1:0]  din2,   
    output logic [DATA_WIDTH-1:0]  dout2
);

    // Ova linija osigurava da Vivado koristi Block RAM resurse
    (* ram_style = "block" *) logic [DATA_WIDTH-1:0] ram [2**ADDR_WIDTH-1:0];

    // Port 1: CPU pristup
    always_ff @(posedge clk1) begin
        if (we1) begin
            ram[addr1] <= din1;
        end
        dout1 <= ram[addr1];
    end

    // Port 2: DAC Controller pristup
    always_ff @(posedge clk2) begin
        if (we2) begin
            ram[addr2] <= din2;
        end
        dout2 <= ram[addr2];
    end

endmodule
