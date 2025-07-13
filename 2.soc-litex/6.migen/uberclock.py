# uberclock_block.py

from migen import Signal, ClockSignal, ResetSignal, If
from migen import Module, Instance
from litex.gen import LiteXModule
from litex.soc.interconnect.csr import CSRStorage, CSRStatus
from litex.soc.interconnect.csr_eventmanager import EventManager, EventSourcePulse
from litescope import LiteScopeAnalyzer

class UberclockBlock(LiteXModule, Module):
    """Encapsulates the Verilog instantiation & CSR wiring for Uberclock."""
    def __init__(self, platform, verilog_dir, sys_clk_freq):
        # --- 1) bring in all your .v sources ---
        for filename in [
                "adc/adc.v",
                "dac/dac.v",

                "filters/cic.v",
                "filters/cic_comp_down_mac.v",
                "filters/comp_down_coeffs.mem",
                "filters/hb_down_mac.v",
                "filters/hb_down_coeffs.mem",
                "filters/downsamplerFilter.v",

                "filters/upsamplerFilter.v",
                "filters/hb_up_mac.v",
                "filters/coeffs.mem",
                "filters/cic_comp_up_mac.v",
                "filters/coeffs_comp.mem",
                "filters/cic_int.v",

                "cordic_dsp_dac/cordic_dsp_dac.v",

                "cordic/cordic_pre_rotate.v",
                "cordic/cordic_pipeline_stage.v",
                "cordic/cordic_round.v",
                "cordic/cordic.v",
                "cordic/cordic_logic.v",
                "cordic/gain_and_saturate.v",

                "cordic16/cordic16.v",
                # "cordic16/gain_and_saturate.v",
                # "cordic16/cordic_round.v",
                "cordic16/cordic_pre_rotate_16.v",
                # "cordic16/cordic_pipeline_stage.v",
            ]:
                platform.add_source(f"{verilog_dir}/{filename}")

        # --- 2) Declare CSRs ---
        self.input_select    = CSRStorage(1,  description="0=ADC,1=NCO")
        self.output_select   = CSRStorage(2,  description="DAC output selector")
        self.phase_inc_nco   = CSRStorage(19, description="NCO phase increment")
        self.phase_inc_down  = CSRStorage(19, description="Downconv phase inc")
        self.gain1           = CSRStorage(32, description="Gain1 (Q)")
        self.gain2           = CSRStorage(32, description="Gain2 (Q)")
        self.downsampled     = CSRStatus(16, description="Post-downsample")
        self.upsampler_input = CSRStorage(16, description="Upsampler input")

        # --- 3) EventManager for interrupt on downsample ready ---
        ce_down = Signal()
        evm     = EventManager()
        evm.ce_down = EventSourcePulse(description="Downsample ready pulse")
        self.submodules += evm
        evm.finalize()
        self.irq.add("ce_down")

        # --- 4) Debug probes (aggregate in a dict for brevity) ---
        dbg = {
            "nco_cos":       Signal(12),
            "nco_sin":       Signal(12),
            "phase_acc_down":Signal(19),
            "x_downconverted":Signal(12),
            "y_downconverted":Signal(12),
            "downsampled_x": Signal(16),
            "downsampled_y": Signal(16),
            "upsampled_x":Signal(16),
            "upsampled_y":Signal(16),
            "phase_inv":Signal(23),
            "x_upconverted":Signal(16),
            "y_upconverted":Signal(16),
            "ce_down_x":Signal(),
            "ce_up_x":Signal(),
            "cic_ce_x":Signal(),
            "comp_ce_x":Signal(),
            "hb_ce_x":Signal(),
            "cic_out_x":Signal(12),
            "comp_out_x":Signal(16),
        }

        # --- 5) Tie CSRs to local signals for the Instance… ---
        in_sel  = self.input_select.storage
        out_sel = self.output_select.storage
        pin_nco = self.phase_inc_nco.storage
        pin_dn  = self.phase_inc_down.storage
        g1, g2  = self.gain1.storage, self.gain2.storage
        ds_in   = self.upsampler_input.storage

        # --- 6) Instantiate the top-level Verilog module ---
        self.specials += Instance(
            "cordic_dsp_dac",
            i_sys_clk        = ClockSignal("sys"),
            i_rst            = ResetSignal("sys"),

            # ADC interfaces
            o_adc_clk_ch0    = platform.request("adc_clk_ch0"),
            o_adc_clk_ch1    = platform.request("adc_clk_ch1"),
            i_adc_data_ch0   = platform.request("adc_data_ch0"),
            i_adc_data_ch1   = platform.request("adc_data_ch1"),

            # DAC interfaces
            o_da1_clk        = platform.request("da1_clk",  0),
            o_da1_wrt        = platform.request("da1_wrt",  0),
            o_da1_data       = platform.request("da1_data", 0),
            o_da2_clk        = platform.request("da2_clk",  0),
            o_da2_wrt        = platform.request("da2_wrt",  0),
            o_da2_data       = platform.request("da2_data", 0),

            # CSR inputs
            i_input_select   = in_sel,
            i_output_select  = out_sel,
            i_phase_inc_nco  = pin_nco,
            i_phase_inc_down = pin_dn,
            i_gain1          = g1,
            i_gain2          = g2,
            i_upsampler_input= ds_in,

            # CSR outputs
            o_downsampled_data = self.downsampled.status,
            o_ce_down          = ce_down,

            # debug outputs (unpack dict)
            **{f"o_dbg_{k}": v for k, v in dbg.items()}
        )

        # --- 7) Hook event pulse & CSRStatus mirror ---
        self.sync += If(ce_down, evm.ce_down.trigger.eq(1))
        self.comb += self.downsampled.status.eq(dbg["downsampled_y"])

        # --- 8) LiteScope on all debug signals + key CSRs ---
        probes = list(dbg.values()) + [pin_nco, pin_dn, in_sel, out_sel, g1, g2]
        self.submodules.analyzer = LiteScopeAnalyzer(
            probes, depth=16384, clock_domain="sys", samplerate=sys_clk_freq
        )
        self.add_csr("analyzer")
