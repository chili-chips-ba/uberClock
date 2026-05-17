// SPDX-FileCopyrightText: 2026 Tarik Hamedovic
// SPDX-License-Identifier: BSD-2-Clause
//
// Minimal simulation models for Xilinx 7-series IO primitives used by the
// uberClock ADC/DAC wrappers. These are behavioral models for Verilator/LiteX
// simulation only; synthesis must use the vendor primitives.

`timescale 1ns/1ps

module IDDR #(
    parameter DDR_CLK_EDGE = "SAME_EDGE_PIPELINED",
    parameter INIT_Q1 = 1'b0,
    parameter INIT_Q2 = 1'b0,
    parameter SRTYPE = "SYNC"
) (
    output reg Q1,
    output reg Q2,
    input wire C,
    input wire CE,
    input wire D,
    input wire R,
    input wire S
);
    initial begin
        Q1 = INIT_Q1;
        Q2 = INIT_Q2;
    end

    always @(posedge C) begin
        if (R) begin
            Q1 <= 1'b0;
        end else if (S) begin
            Q1 <= 1'b1;
        end else if (CE) begin
            Q1 <= D;
        end
    end

    always @(negedge C) begin
        if (R) begin
            Q2 <= 1'b0;
        end else if (S) begin
            Q2 <= 1'b1;
        end else if (CE) begin
            Q2 <= D;
        end
    end
endmodule

module ODDR #(
    parameter DDR_CLK_EDGE = "SAME_EDGE",
    parameter INIT = 1'b0,
    parameter SRTYPE = "SYNC"
) (
    output reg Q,
    input wire C,
    input wire CE,
    input wire D1,
    input wire D2,
    input wire R,
    input wire S
);
    initial Q = INIT;

    always @(posedge C) begin
        if (R) begin
            Q <= 1'b0;
        end else if (S) begin
            Q <= 1'b1;
        end else if (CE) begin
            Q <= D1;
        end
    end

    always @(negedge C) begin
        if (R) begin
            Q <= 1'b0;
        end else if (S) begin
            Q <= 1'b1;
        end else if (CE) begin
            Q <= D2;
        end
    end
endmodule
