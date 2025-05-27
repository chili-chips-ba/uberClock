# -------------------------------------------------------------------------
#  Custom AXI-Lite RAM + LED driver wrapper
# -------------------------------------------------------------------------
class LEDMemAXI(LiteXModule):
    def __init__(self, pads):
        self.axi = AXILiteInterface(data_width=32, address_width=4)
        self.specials += Instance("mem_led_axi",
            i_s_axi_aclk    = ClockSignal(),
            i_s_axi_aresetn = ~ResetSignal(),

            i_s_axi_awvalid = self.axi.aw.valid,
            o_s_axi_awready = self.axi.aw.ready,
            i_s_axi_awaddr  = self.axi.aw.addr,
            i_s_axi_awprot  = 0,

            i_s_axi_wvalid  = self.axi.w.valid,
            o_s_axi_wready  = self.axi.w.ready,
            i_s_axi_wdata   = self.axi.w.data,
            i_s_axi_wstrb   = self.axi.w.strb,

            o_s_axi_bvalid  = self.axi.b.valid,
            i_s_axi_bready  = self.axi.b.ready,
            o_s_axi_bresp   = self.axi.b.resp,

            i_s_axi_arvalid = self.axi.ar.valid,
            o_s_axi_arready = self.axi.ar.ready,
            i_s_axi_araddr  = self.axi.ar.addr,
            i_s_axi_arprot  = 0,

            o_s_axi_rvalid  = self.axi.r.valid,
            i_s_axi_rready  = self.axi.r.ready,
            o_s_axi_rdata   = self.axi.r.data,
            o_s_axi_rresp   = self.axi.r.resp,

            o_leds          = pads
        )
