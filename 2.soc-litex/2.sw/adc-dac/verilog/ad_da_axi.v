//------------------------------------------------------------------------------
// ad_da_axi.v
//   AXI-Lite “passthrough” of ADC → CPU and CPU → DAC
//------------------------------------------------------------------------------
`timescale 1ns/1ps
module ad_da_axi #
(
  parameter integer DATA_WIDTH = 32,
  parameter integer ADDR_WIDTH = 4    // 2^4 = 16 words = 64 B
)
(
  // clock / reset
  input  wire                 clk,
  input  wire                 aresetn,

  //── AXI4-Lite slave interface ──────────────────────────────────────────────
  input  wire                 s_axi_aw_valid,
  output reg                  s_axi_aw_ready,
  input  wire [ADDR_WIDTH-1:0] s_axi_aw_addr,
  input  wire [2:0]           s_axi_aw_prot,

  input  wire                 s_axi_w_valid,
  output reg                  s_axi_w_ready,
  input  wire [DATA_WIDTH-1:0] s_axi_w_data,
  input  wire [DATA_WIDTH/8-1:0] s_axi_w_strb,

  output reg                  s_axi_b_valid,
  input  wire                 s_axi_b_ready,
  output wire [1:0]           s_axi_b_resp,

  input  wire                 s_axi_ar_valid,
  output reg                  s_axi_ar_ready,
  input  wire [ADDR_WIDTH-1:0] s_axi_ar_addr,
  input  wire [2:0]           s_axi_ar_prot,

  output reg                  s_axi_r_valid,
  input  wire                 s_axi_r_ready,
  output reg [DATA_WIDTH-1:0] s_axi_r_data,
  output wire [1:0]           s_axi_r_resp,

  //── ADC / DAC physical pins ─────────────────────────────────────────────────
  input  wire [11:0]          adc_ch1_data,  // raw 12-bit
  output wire                 adc_ch1_clk,

  output wire [13:0]          dac_ch1_data,  // 14-bit
  output wire                 dac_ch1_wrt,
  output wire                 dac_ch1_clk
);

  // simple OKAY
  assign s_axi_b_resp = 2'b00;
  assign s_axi_r_resp = 2'b00;

  // handshakes
  wire aw_hs = s_axi_aw_valid && s_axi_aw_ready;
  wire w_hs  = s_axi_w_valid  && s_axi_w_ready;
  wire ar_hs = s_axi_ar_valid && s_axi_ar_ready;

  //──── internal registers ─────────────────────────────────────────────────────
  reg [11:0] adc_reg;
  // DAC value to drive out
  reg [13:0] dac_reg;

  // latch ADC data each cycle
  always @(posedge clk) begin
    if (!aresetn)           adc_reg <= 12'd0;
    else                    adc_reg <= adc_ch1_data;
  end

  // AXI write-address ready / write-data ready
  always @(posedge clk or negedge aresetn) begin
    if (!aresetn) begin
      s_axi_aw_ready <= 1'b1;
      s_axi_w_ready  <= 1'b1;
    end else begin
      s_axi_aw_ready <= !s_axi_b_valid;
      s_axi_w_ready  <= !s_axi_b_valid;
    end
  end

  // AXI write-response logic
  always @(posedge clk or negedge aresetn) begin
    if (!aresetn)       s_axi_b_valid <= 1'b0;
    else if (aw_hs && w_hs)
                        s_axi_b_valid <= 1'b1;
    else if (s_axi_b_valid && s_axi_b_ready)
                        s_axi_b_valid <= 1'b0;
  end

  // AXI write → update dac_reg at address 0x04
  always @(posedge clk or negedge aresetn) begin
    if (!aresetn) begin
      dac_reg <= 14'd0;
    end else if (aw_hs && w_hs && (s_axi_aw_addr == 4'h1)) begin
      dac_reg <= s_axi_w_data[13:0];
    end
  end

  // AXI read-address ready
  always @(posedge clk or negedge aresetn) begin
    if (!aresetn)       s_axi_ar_ready <= 1'b1;
    else                s_axi_ar_ready <= !s_axi_r_valid;
  end

  // AXI read-data / valid
  always @(posedge clk or negedge aresetn) begin
    if (!aresetn) begin
      s_axi_r_valid <= 1'b0;
      s_axi_r_data  <= {DATA_WIDTH{1'b0}};
    end else if (ar_hs) begin
      s_axi_r_valid <= 1'b1;
      case(s_axi_ar_addr)
        4'h0:  // READ ADC
          s_axi_r_data <= {20'd0, adc_reg};
        4'h1:  // READ BACK DAC
          s_axi_r_data <= {18'd0, dac_reg};
        default:
          s_axi_r_data <= {DATA_WIDTH{1'b0}};
      endcase
    end else if (s_axi_r_valid && s_axi_r_ready) begin
      s_axi_r_valid <= 1'b0;
    end
  end

  assign adc_ch1_clk = clk;
  assign dac_ch1_clk = clk;
  assign dac_ch1_wrt = (aw_hs && w_hs && (s_axi_aw_addr == 4'h1));
  assign dac_ch1_data = dac_reg;

endmodule
