module mem_led_axi (
    input           clk,
    input           areset_n,        // active-low reset

    // — AW (write address) —
    input           s_axi_aw_valid,
    output reg      s_axi_aw_ready,
    input  [31:0]   s_axi_aw_addr,
    input  [2:0]    s_axi_aw_prot,

    // — W (write data) —
    input           s_axi_w_valid,
    output reg      s_axi_w_ready,
    input  [31:0]   s_axi_w_data,
    input  [3:0]    s_axi_w_strb,

    // — B (write response) —
    output reg      s_axi_b_valid,
    input           s_axi_b_ready,
    output     [1:0] s_axi_b_resp,

    // — AR (read address) —
    input           s_axi_ar_valid,
    output reg      s_axi_ar_ready,
    input  [31:0]   s_axi_ar_addr,
    input  [2:0]    s_axi_ar_prot,

    // — R (read data) —
    output reg      s_axi_r_valid,
    input           s_axi_r_ready,
    output reg [31:0] s_axi_r_data,
    output     [1:0] s_axi_r_resp,

    // LEDs
    output reg [3:0] leds
);

  // 16 words × 32-bit RAM
  reg [31:0] ram [0:15];

  // write-channel latches and flags
  reg        aw_pending;
  reg [3:0]  aw_addr_reg;
  reg [31:0] wdata_reg;
  reg [3:0]  wstrb_reg;

  // constant OKAY responses
  assign s_axi_b_resp = 2'b00;
  assign s_axi_r_resp = 2'b00;

  always @(posedge clk or negedge areset_n) begin
    if (!areset_n) begin
      // reset all control signals & LEDs
      s_axi_aw_ready <= 1'b1;
      s_axi_w_ready  <= 1'b1;
      s_axi_b_valid  <= 1'b0;
      s_axi_ar_ready <= 1'b1;
      s_axi_r_valid  <= 1'b0;
      leds           <= 4'b0000;

      aw_pending     <= 1'b0;
      aw_addr_reg    <= 4'b0;
      wdata_reg      <= 32'b0;
      wstrb_reg      <= 4'b0;
    end else begin
      // — AW channel: latch address if valid & ready
      if (s_axi_aw_ready && s_axi_aw_valid) begin
        aw_pending     <= 1'b1;
        aw_addr_reg    <= s_axi_aw_addr[5:2];
        s_axi_aw_ready <= 1'b0;
      end else if (s_axi_b_valid && s_axi_b_ready) begin
        // once the response is accepted, reopen AW
        s_axi_aw_ready <= 1'b1;
      end

      // — W channel: latch data if valid & ready
      if (s_axi_w_ready && s_axi_w_valid) begin
        wdata_reg     <= s_axi_w_data;
        wstrb_reg     <= s_axi_w_strb;
        s_axi_w_ready <= 1'b0;
      end else if (s_axi_b_valid && s_axi_b_ready) begin
        // once the response is accepted, reopen W
        s_axi_w_ready <= 1'b1;
      end

      // — Write Response: when both AW and W are latched, perform write & issue BVALID
      if (aw_pending && !s_axi_b_valid && !s_axi_aw_ready && !s_axi_w_ready) begin
        // apply write strobes to RAM
        if (wstrb_reg[0]) ram[aw_addr_reg][ 7: 0] <= wdata_reg[ 7: 0];
        if (wstrb_reg[1]) ram[aw_addr_reg][15: 8] <= wdata_reg[15: 8];
        if (wstrb_reg[2]) ram[aw_addr_reg][23:16] <= wdata_reg[23:16];
        if (wstrb_reg[3]) ram[aw_addr_reg][31:24] <= wdata_reg[31:24];

        // issue OKAY response
        s_axi_b_valid <= 1'b1;
        aw_pending    <= 1'b0;
      end

      // clear BVALID once master accepts it
      if (s_axi_b_valid && s_axi_b_ready) begin
        s_axi_b_valid <= 1'b0;
      end

      // — Read Address / Data channels —
      if (s_axi_ar_ready && s_axi_ar_valid) begin
        s_axi_ar_ready <= 1'b0;
        s_axi_r_valid  <= 1'b1;
        s_axi_r_data   <= ram[s_axi_ar_addr[5:2]];
      end else if (s_axi_r_valid && s_axi_r_ready) begin
        s_axi_ar_ready <= 1'b1;
        s_axi_r_valid  <= 1'b0;
      end

      // Drive LEDs from RAM[0]
      leds <= ram[0][3:0];
    end
  end

`ifdef FORMAL
  faxil_slave#(
  .C_AXI_ADDR_WIDTH(32),
  .C_AXI_DATA_WIDTH(32),
  .F_OPT_ASYNC_RESET(1'b1)
  )faxil(
    .i_clk(clk),
    .i_axi_reset_n(areset_n),
    .i_axi_awvalid(s_axi_aw_valid),
    .i_axi_awready(s_axi_aw_ready),
    .i_axi_awaddr (s_axi_aw_addr),
    .i_axi_awprot (s_axi_aw_prot),

    .i_axi_wvalid(s_axi_w_valid),
    .i_axi_wready(s_axi_w_ready),
    .i_axi_wdata (s_axi_w_data),
    .i_axi_wstrb (s_axi_w_strb),

    .i_axi_bvalid(s_axi_b_valid),
    .i_axi_bready(s_axi_b_ready),
    .i_axi_bresp (s_axi_b_resp),

    .i_axi_arvalid(s_axi_ar_valid),
    .i_axi_arready(s_axi_ar_ready),
    .i_axi_araddr (s_axi_ar_addr),
    .i_axi_arprot (s_axi_ar_prot),

    .i_axi_rvalid(s_axi_r_valid),
    .i_axi_rready(s_axi_r_ready),
    .i_axi_rdata (s_axi_r_data),
    .i_axi_rresp (s_axi_r_resp),

    .f_axi_rd_outstanding(),
    .f_axi_wr_outstanding(),
    .f_axi_awr_outstanding()
  );
`endif


endmodule
