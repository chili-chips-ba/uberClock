module ub_stream2wb_dma #(
    parameter integer BUS_WIDTH         = 256,
    parameter integer ADDRESS_WIDTH     = 32,
    parameter         OPT_LITTLE_ENDIAN = 1'b1,
    parameter integer LGPIPE            = 6
)(
    input  wire                            i_clk,
    input  wire                            i_reset,

    input  wire                            i_request,
    input  wire [ADDRESS_WIDTH-1:0]        i_addr,
    output wire                            o_busy,
    output wire                            o_err,

    // Incoming stream
    input  wire                            S_VALID,
    output wire                            S_READY,
    input  wire [BUS_WIDTH-1:0]            S_DATA,
    input  wire [$clog2(BUS_WIDTH/8):0]    S_BYTES,
    input  wire                            S_LAST,

    // Pipelined Wishbone master
    output wire                            o_mcyc,
    output wire                            o_mstb,
    output wire                            o_mwe,
    output wire [ADDRESS_WIDTH-$clog2(BUS_WIDTH/8)-1:0] o_maddr,
    output wire [BUS_WIDTH-1:0]            o_mdata,
    output wire [BUS_WIDTH/8-1:0]          o_msel,
    input  wire                            i_mstall,
    input  wire                            i_mack,
    input  wire [BUS_WIDTH-1:0]            i_mdata,
    input  wire                            i_merr
);
    localparam WBLSB = $clog2(BUS_WIDTH/8);

    // ---------------- rxgears ----------------
    wire                    G_VALID, G_READY, G_LAST;
    wire [BUS_WIDTH-1:0]    G_DATA;
    wire [WBLSB:0]          G_BYTES;

    zipdma_rxgears #(
        .BUS_WIDTH(BUS_WIDTH),
        .OPT_LITTLE_ENDIAN(OPT_LITTLE_ENDIAN)
    ) u_rxgears (
        .i_clk(i_clk),
        .i_reset(i_reset),
        .i_soft_reset(1'b0),
        .S_VALID(S_VALID),
        .S_READY(S_READY),
        .S_DATA (S_DATA),
        .S_BYTES(S_BYTES),
        .S_LAST (S_LAST),
        .M_VALID(G_VALID),
        .M_READY(G_READY),
        .M_DATA (G_DATA),
        .M_BYTES(G_BYTES),
        .M_LAST (G_LAST)
    );

    // ---------------- sfifo ----------------
    localparam FIFO_W = BUS_WIDTH + (WBLSB+1) + 1;
    wire               F_FULL, F_EMPTY;
    wire [FIFO_W-1:0]  F_IN, F_OUT;
    wire               F_WR, F_RD;

    assign F_IN   = {G_DATA, G_BYTES, G_LAST};
    assign G_READY = ~F_FULL;
    assign F_WR    = G_VALID & G_READY;

    sfifo #(
        .BW(FIFO_W),
        .LGFLEN(6),
        .OPT_ASYNC_READ(1'b1),
        .OPT_WRITE_ON_FULL(1'b0),
        .OPT_READ_ON_EMPTY(1'b0)
    ) u_sfifo (
        .i_clk(i_clk),
        .i_reset(i_reset),
        .i_wr  (F_WR),
        .i_data(F_IN),
        .o_full(F_FULL),
        .o_fill(),
        .i_rd  (F_RD),
        .o_data(F_OUT),
        .o_empty(F_EMPTY)
    );

    wire [BUS_WIDTH-1:0] S2_DATA;
    wire [WBLSB:0]       S2_BYTES;
    wire                 S2_LAST, S2_VALID, S2_READY;

    assign {S2_DATA, S2_BYTES, S2_LAST} = F_OUT;
    assign S2_VALID = !F_EMPTY;
    assign F_RD     =  S2_READY;

    // ---------------- s2mm ----------------
    wire [ADDRESS_WIDTH-$clog2(BUS_WIDTH/8)-1:0] wb_addr_w;

    zipdma_s2mm #(
        .ADDRESS_WIDTH(ADDRESS_WIDTH),
        .BUS_WIDTH(BUS_WIDTH),
        .OPT_LITTLE_ENDIAN(OPT_LITTLE_ENDIAN),
        .LGPIPE(LGPIPE)
    ) u_s2mm (
        .i_clk(i_clk),
        .i_reset(i_reset),
        .i_request(i_request),
        .o_busy(o_busy),
        .o_err(o_err),
        .i_inc(1'b1),
        .i_size(2'b00),
        .i_addr(i_addr),

        .S_VALID(S2_VALID),
        .S_READY(S2_READY),
        .S_DATA (S2_DATA),
        .S_BYTES(S2_BYTES),
        .S_LAST (S2_LAST),

        .o_wr_cyc (o_mcyc),
        .o_wr_stb (o_mstb),
        .o_wr_we  (o_mwe),
        .o_wr_addr(wb_addr_w),
        .o_wr_data(o_mdata),
        .o_wr_sel (o_msel),
        .i_wr_stall(i_mstall),
        .i_wr_ack (i_mack),
        .i_wr_data(i_mdata),
        .i_wr_err (i_merr)
    );

    assign o_maddr = wb_addr_w;
endmodule
`default_nettype wire
