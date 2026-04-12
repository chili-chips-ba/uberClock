`default_nettype none
module mem_led_axi #(
    // Widths: AXI-Lite is fixed at 32 bits, four config words
    parameter C_AXI_ADDR_WIDTH = 4,
    localparam C_AXI_DATA_WIDTH = 32,
    parameter [0:0] OPT_SKIDBUFFER   = 1'b0,
    parameter [0:0] OPT_LOWPOWER     = 0
) (
    // AXI-Lite interface
    input  wire                        s_axi_aclk,
    input  wire                        s_axi_aresetn,

    input  wire                        s_axi_awvalid,
    output wire                        s_axi_awready,
    input  wire [C_AXI_ADDR_WIDTH-1:0] s_axi_awaddr,
    input  wire [2:0]                  s_axi_awprot,

    input  wire                        s_axi_wvalid,
    output wire                        s_axi_wready,
    input  wire [C_AXI_DATA_WIDTH-1:0] s_axi_wdata,
    input  wire [C_AXI_DATA_WIDTH/8-1:0] s_axi_wstrb,

    output wire                        s_axi_bvalid,
    input  wire                        s_axi_bready,
    output wire [1:0]                  s_axi_bresp,

    input  wire                        s_axi_arvalid,
    output wire                        s_axi_arready,
    input  wire [C_AXI_ADDR_WIDTH-1:0] s_axi_araddr,
    input  wire [2:0]                  s_axi_arprot,

    output wire                        s_axi_rvalid,
    input  wire                        s_axi_rready,
    output wire [C_AXI_DATA_WIDTH-1:0] s_axi_rdata,
    output wire [1:0]                  s_axi_rresp,

    // LED outputs: one per RAM word (bit 0)
    output wire [3:0]                  leds
);

//////////////////////////////////////////////////////////////////////////
// Signal declarations
//////////////////////////////////////////////////////////////////////////
localparam ADDR_LSB = $clog2(C_AXI_DATA_WIDTH) - 3;

wire reset_sync = !s_axi_aresetn;

// Write channel
wire                       axi_write_ready;
wire [C_AXI_ADDR_WIDTH-ADDR_LSB-1:0] awskd_addr;
wire [C_AXI_DATA_WIDTH-1:0] wskd_data;
wire [C_AXI_DATA_WIDTH/8-1:0] wskd_strb;
reg                        axi_bvalid;

// Read channel
wire                       axi_read_ready;
wire [C_AXI_ADDR_WIDTH-ADDR_LSB-1:0] arskd_addr;
reg  [C_AXI_DATA_WIDTH-1:0] axi_read_data;
reg                        axi_read_valid;

// --- replace reg0..reg3 with a 4-word RAM ---
reg [C_AXI_DATA_WIDTH-1:0] ram [0:3];

//////////////////////////////////////////////////////////////////////////
// AXI-Lite write logic (unchanged)
//////////////////////////////////////////////////////////////////////////
generate if (OPT_SKIDBUFFER) begin : skidbuffer_write
    wire awskd_valid, wskd_valid;
    // … same as before …
    assign axi_write_ready = awskd_valid && wskd_valid
                           && (!axi_bvalid || s_axi_bready);
end else begin : simple_writes
    reg aw_ready;
    initial aw_ready = 1'b0;
    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn)
            aw_ready <= 1'b0;
        else
            aw_ready <= !aw_ready
                       && (s_axi_awvalid && s_axi_wvalid)
                       && (!axi_bvalid || s_axi_bready);
    end

    assign s_axi_awready    = aw_ready;
    assign s_axi_wready     = aw_ready;
    assign awskd_addr       = s_axi_awaddr[C_AXI_ADDR_WIDTH-1:ADDR_LSB];
    assign wskd_data        = s_axi_wdata;
    assign wskd_strb        = s_axi_wstrb;
    assign axi_write_ready  = aw_ready;
end endgenerate

initial axi_bvalid = 1'b0;
always @(posedge s_axi_aclk) begin
    if (reset_sync)
        axi_bvalid <= 1'b0;
    else if (axi_write_ready)
        axi_bvalid <= 1'b1;
    else if (s_axi_bready)
        axi_bvalid <= 1'b0;
end

assign s_axi_bvalid = axi_bvalid;
assign s_axi_bresp  = 2'b00;

//////////////////////////////////////////////////////////////////////////
// AXI-Lite read logic (unchanged)
//////////////////////////////////////////////////////////////////////////
generate if (OPT_SKIDBUFFER) begin : skidbuffer_read
    wire arskd_valid;
    // … same as before …
    assign axi_read_ready = arskd_valid
                          && (!axi_read_valid || s_axi_rready);
end else begin : simple_reads
    reg ar_ready;
    always @(*) ar_ready = !s_axi_rvalid;
    assign s_axi_arready   = ar_ready;
    assign arskd_addr      = s_axi_araddr[C_AXI_ADDR_WIDTH-1:ADDR_LSB];
    assign axi_read_ready  = (s_axi_arvalid && ar_ready);
end endgenerate

initial axi_read_valid = 1'b0;
always @(posedge s_axi_aclk) begin
    if (reset_sync)
        axi_read_valid <= 1'b0;
    else if (axi_read_ready)
        axi_read_valid <= 1'b1;
    else if (s_axi_rready)
        axi_read_valid <= 1'b0;
end

assign s_axi_rvalid = axi_read_valid;
assign s_axi_rresp  = 2'b00;

//////////////////////////////////////////////////////////////////////////
// RAM write & read
//////////////////////////////////////////////////////////////////////////
always @(posedge s_axi_aclk) begin
    if (reset_sync) begin
        ram[0] <= 0;
        ram[1] <= 0;
        ram[2] <= 0;
        ram[3] <= 0;
    end else if (axi_write_ready) begin
        ram[awskd_addr] <= apply_wstrb(ram[awskd_addr], wskd_data, wskd_strb);
    end
end

always @(posedge s_axi_aclk) begin
    if (!axi_read_valid || s_axi_rready) begin
        axi_read_data <= ram[arskd_addr];
    end
end

assign s_axi_rdata = axi_read_data;

//////////////////////////////////////////////////////////////////////////
// LED drive: use bit 0 of each RAM word
//////////////////////////////////////////////////////////////////////////
assign leds = { ram[3][0], ram[2][0], ram[1][0], ram[0][0] };

//////////////////////////////////////////////////////////////////////////
// write-strobe function (unchanged)
//////////////////////////////////////////////////////////////////////////
function [C_AXI_DATA_WIDTH-1:0] apply_wstrb;
    input [C_AXI_DATA_WIDTH-1:0] prior_data;
    input [C_AXI_DATA_WIDTH-1:0] new_data;
    input [C_AXI_DATA_WIDTH/8-1:0] wstrb;
    integer i;
    for (i = 0; i < C_AXI_DATA_WIDTH/8; i = i+1)
        apply_wstrb[i*8 +: 8] = wstrb[i]
            ? new_data[i*8 +: 8]
            : prior_data[i*8 +: 8];
endfunction

//=============================================================================
// Unused signals (for Verilator)
wire unused_signals = &{
    1'b0,
    s_axi_awprot,
    s_axi_arprot,
    s_axi_araddr[ADDR_LSB-1:0],
    s_axi_awaddr[ADDR_LSB-1:0]
};

`ifdef FORMAL
    //=============================================================================
    // Formal properties
    //=============================================================================
    localparam F_AXIL_LG_DEPTH      = 4;
    wire [F_AXIL_LG_DEPTH-1:0] f_axil_rd_outstanding;
    wire [F_AXIL_LG_DEPTH-1:0] f_axil_wr_outstanding;
    wire [F_AXIL_LG_DEPTH-1:0] f_axil_awr_outstanding;

    faxil_slave #(
        .C_AXI_DATA_WIDTH (C_AXI_DATA_WIDTH),
        .C_AXI_ADDR_WIDTH (C_AXI_ADDR_WIDTH),
        .F_LGDEPTH        (F_AXIL_LG_DEPTH),
        .F_AXI_MAXWAIT    (3),
        .F_AXI_MAXDELAY   (3),
        .F_AXI_MAXRSTALL  (5),
        .F_OPT_COVER_BURST(4)
    ) u_formal (
        .i_clk             (s_axi_aclk),
        .i_axi_reset_n     (s_axi_aresetn),
        .i_axi_awvalid     (s_axi_awvalid),
        .i_axi_awready     (s_axi_awready),
        .i_axi_awaddr      (s_axi_awaddr),
        .i_axi_awprot      (s_axi_awprot),
        .i_axi_wvalid      (s_axi_wvalid),
        .i_axi_wready      (s_axi_wready),
        .i_axi_wdata       (s_axi_wdata),
        .i_axi_wstrb       (s_axi_wstrb),
        .i_axi_bvalid      (s_axi_bvalid),
        .i_axi_bready      (s_axi_bready),
        .i_axi_bresp       (s_axi_bresp),
        .i_axi_arvalid     (s_axi_arvalid),
        .i_axi_arready     (s_axi_arready),
        .i_axi_araddr      (s_axi_araddr),
        .i_axi_arprot      (s_axi_arprot),
        .i_axi_rvalid      (s_axi_rvalid),
        .i_axi_rready      (s_axi_rready),
        .i_axi_rdata       (s_axi_rdata),
        .i_axi_rresp       (s_axi_rresp),
        .f_axi_rd_outstanding(f_axil_rd_outstanding),
        .f_axi_wr_outstanding(f_axil_wr_outstanding),
        .f_axi_awr_outstanding(f_axil_awr_outstanding)
    );

    always @(*) begin
        if (OPT_SKIDBUFFER) begin
            assert(f_axil_awr_outstanding == (s_axi_bvalid ? 1 : 0)
                + (s_axi_awready ? 0 : 1));
            assert(f_axil_wr_outstanding == (s_axi_bvalid ? 1 : 0)
                + (s_axi_wready ? 0 : 1));
            assert(f_axil_rd_outstanding == (s_axi_rvalid ? 1 : 0)
                + (s_axi_arready ? 0 : 1));
        end else begin
            assert(f_axil_wr_outstanding == (s_axi_bvalid ? 1 : 0));
            assert(f_axil_awr_outstanding == f_axil_wr_outstanding);
            assert(f_axil_rd_outstanding == (s_axi_rvalid ? 1 : 0));
        end
    end

    always @(*)
        if (OPT_LOWPOWER && !s_axi_rvalid)
            assert(s_axi_rdata == 0);
`endif

endmodule
