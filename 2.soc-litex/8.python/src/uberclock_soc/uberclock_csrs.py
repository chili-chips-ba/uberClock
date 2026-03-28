# uberclock_csr_bank.py
#
# UberClock CSR Register Bank
# ==========================
# This module defines all memory-mapped control and status registers used to
# configure and observe the UberClock DSP block from the CPU (LiteX firmware,
# host software, scripts).
#

from migen import *
from litex.gen import *

from litex.soc.interconnect.csr import CSRStorage, CSRStatus


class UberClockCSRBank(LiteXModule):

    # ---------------------------------------------------------------------
    # Global width definitions
    # ---------------------------------------------------------------------
    PHASE_WIDTH        = 26  # Phase accumulator / phase increment width
    MAG_WIDTH          = 12  # Signed magnitude width (2's complement)
    GAIN_WIDTH         = 32  # Fixed-point gain coefficients
    SAMPLE_WIDTH       = 16  # Low-speed / CPU-injected sample width
    FINAL_SHIFT_WIDTH  = 3   # Output scaling shift

    def __init__(self):

        # =====================================================================
        #                Primary NCO and reference phase increments
        # =====================================================================

        self.phase_inc_nco = CSRStorage(
            self.PHASE_WIDTH,
            description="Main NCO phase increment controlling carrier frequency."
        )

        self.phase_inc_down_1 = CSRStorage(
            self.PHASE_WIDTH,
            description="Downsampler phase increment for channel 1."
        )
        self.phase_inc_down_2 = CSRStorage(
            self.PHASE_WIDTH,
            description="Downsampler phase increment for channel 2."
        )
        self.phase_inc_down_3 = CSRStorage(
            self.PHASE_WIDTH,
            description="Downsampler phase increment for channel 3."
        )
        self.phase_inc_down_4 = CSRStorage(
            self.PHASE_WIDTH,
            description="Downsampler phase increment for channel 4."
        )
        self.phase_inc_down_5 = CSRStorage(
            self.PHASE_WIDTH,
            description="Downsampler phase increment for channel 5."
        )

        self.phase_inc_down_ref = CSRStorage(
            self.PHASE_WIDTH,
            description="Reference downsampler phase increment."
        )

        # =====================================================================
        #                    CPU-driven NCO phase increments
        # =====================================================================

        self.phase_inc_cpu1 = CSRStorage(
            self.PHASE_WIDTH,
            description="CPU-controlled NCO phase increment for channel 1."
        )
        self.phase_inc_cpu2 = CSRStorage(
            self.PHASE_WIDTH,
            description="CPU-controlled NCO phase increment for channel 2."
        )
        self.phase_inc_cpu3 = CSRStorage(
            self.PHASE_WIDTH,
            description="CPU-controlled NCO phase increment for channel 3."
        )
        self.phase_inc_cpu4 = CSRStorage(
            self.PHASE_WIDTH,
            description="CPU-controlled NCO phase increment for channel 4."
        )
        self.phase_inc_cpu5 = CSRStorage(
            self.PHASE_WIDTH,
            description="CPU-controlled NCO phase increment for channel 5."
        )

        # =====================================================================
        #                             Magnitudes
        # =====================================================================

        self.nco_mag = CSRStorage(
            self.MAG_WIDTH,
            reset=0,
            description="Magnitude applied to main NCO output (signed)."
        )

        self.mag_cpu1 = CSRStorage(
            self.MAG_WIDTH,
            reset=0,
            description="Magnitude applied to CPU NCO channel 1 (signed)."
        )
        self.mag_cpu2 = CSRStorage(
            self.MAG_WIDTH,
            reset=0,
            description="Magnitude applied to CPU NCO channel 2 (signed)."
        )
        self.mag_cpu3 = CSRStorage(
            self.MAG_WIDTH,
            reset=0,
            description="Magnitude applied to CPU NCO channel 3 (signed)."
        )
        self.mag_cpu4 = CSRStorage(
            self.MAG_WIDTH,
            reset=0,
            description="Magnitude applied to CPU NCO channel 4 (signed)."
        )
        self.mag_cpu5 = CSRStorage(
            self.MAG_WIDTH,
            reset=0,
            description="Magnitude applied to CPU NCO channel 5 (signed)."
        )

        # =====================================================================
        #                 Input/output routing and debug selection
        # =====================================================================

        self.input_select = CSRStorage(
            2,
            description="Selects signal source feeding the DSP pipeline."
        )

        self.upsampler_input_mux = CSRStorage(
            2,
            description="Selects input source for the upsampler stage."
        )

        self.output_select_ch1 = CSRStorage(
            4,
            description="Output mux selection for DAC channel 1."
        )
        self.output_select_ch2 = CSRStorage(
            4,
            description="Output mux selection for DAC channel 2."
        )

        self.lowspeed_dbg_select = CSRStorage(
            3,
            description="Selects low-speed debug signal exported by the DSP."
        )

        self.highspeed_dbg_select = CSRStorage(
            3,
            description="Selects high-speed debug signal exported by the DSP."
        )

        # =====================================================================
        #                   Per-channel gain coefficients
        # =====================================================================

        self.gain1 = CSRStorage(
            self.GAIN_WIDTH,
            description="Gain coefficient applied to channel 1."
        )
        self.gain2 = CSRStorage(
            self.GAIN_WIDTH,
            description="Gain coefficient applied to channel 2."
        )
        self.gain3 = CSRStorage(
            self.GAIN_WIDTH,
            description="Gain coefficient applied to channel 3."
        )
        self.gain4 = CSRStorage(
            self.GAIN_WIDTH,
            description="Gain coefficient applied to channel 4."
        )
        self.gain5 = CSRStorage(
            self.GAIN_WIDTH,
            description="Gain coefficient applied to channel 5."
        )

        # =====================================================================
        #               CPU-injected samples and final output scaling
        # =====================================================================

        self.upsampler_input_x1 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided I (X) sample injected into upsampler."
        )
        self.upsampler_input_y1 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided Q (Y) sample injected into upsampler."
        )

        self.upsampler_input_x2 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided I (X) sample injected into upsampler."
        )
        self.upsampler_input_y2 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided Q (Y) sample injected into upsampler."
        )
        self.upsampler_input_x3 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided I (X) sample injected into upsampler."
        )
        self.upsampler_input_y3 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided Q (Y) sample injected into upsampler."
        )
        self.upsampler_input_x4 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided I (X) sample injected into upsampler."
        )
        self.upsampler_input_y4 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided Q (Y) sample injected into upsampler."
        )
        self.upsampler_input_x5 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided I (X) sample injected into upsampler."
        )
        self.upsampler_input_y5 = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided Q (Y) sample injected into upsampler."
        )
        # =====================================================================
        #        Downsampled data FIFO (UC->SYS, CPU readback)
        # =====================================================================
        # TODO: EXTEND TO 5 CHANNELS
        self.ds_fifo_pop = CSRStorage(
            1,
            description="Write (strobe) to pop one downsampled sample from the FIFO."
        )

        self.ds_fifo_x = CSRStatus(
            self.SAMPLE_WIDTH,
            description="Downsampled X sample from FIFO (latched on pop)."
        )

        self.ds_fifo_y = CSRStatus(
            self.SAMPLE_WIDTH,
            description="Downsampled Y sample from FIFO (latched on pop)."
        )

        self.ds_fifo_overflow = CSRStatus(
            1,
            description="Sticky: downsample FIFO overflow (UC wrote while full)."
        )

        self.ds_fifo_underflow = CSRStatus(
            1,
            description="Sticky: downsample FIFO underflow (CPU pop while empty)."
        )

        self.ds_fifo_clear = CSRStorage(
            1,
            description="Write (strobe) to clear downsample FIFO overflow/underflow flags."
        )

        self.ds_fifo_clear = CSRStorage(
            1,
            description="Write (strobe) to clear downsample FIFO overflow/underflow flags."
        )

        self.ds_fifo_flags = CSRStatus(
            8,
            description=(
                "FIFO flags packed into status bits: bit0=readable (SYS can pop). "
                "Remaining bits are reserved (0)."
            ),
        )

        # =====================================================================
        #        Upsampler input FIFO (SYS->UC, CPU injection)
        # =====================================================================

        self.ups_fifo_x = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided X sample to enqueue into upsampler FIFO."
        )

        self.ups_fifo_y = CSRStorage(
            self.SAMPLE_WIDTH,
            description="CPU-provided Y sample to enqueue into upsampler FIFO."
        )

        self.ups_fifo_push = CSRStorage(
            1,
            description="Write (strobe) to enqueue one sample into the upsampler FIFO."
        )

        self.ups_fifo_overflow = CSRStatus(
            1,
            description="Sticky: upsampler FIFO overflow (CPU push while full)."
        )

        self.ups_fifo_underflow = CSRStatus(
            1,
            description="Sticky: upsampler FIFO underflow (UC read while empty)."
        )

        self.ups_fifo_clear = CSRStorage(
            1,
            description="Write (strobe) to clear upsampler FIFO overflow/underflow flags."
        )

        self.ups_fifo_clear = CSRStorage(
            1,
            description="Write (strobe) to clear upsampler FIFO overflow/underflow flags."
        )

        self.ups_fifo_flags = CSRStatus(
            8,
            description=(
                "FIFO flags packed into status bits: bit1=writable (CPU can push). "
                "Remaining bits are reserved (0)."
            ),
        )

        self.final_shift = CSRStorage(
            self.FINAL_SHIFT_WIDTH,
            description="Final arithmetic right-shift applied to DSP output."
        )

        # =====================================================================
        # High-speed DDR capture control (UberDDR3 path)
        # =====================================================================

        self.cap_enable = CSRStorage(
            1,
            description="Enable high-speed capture from DSP into DDR memory."
        )

        self.cap_beats = CSRStorage(
            32,
            reset=256,
            description="Number of 256-bit beats captured into DDR."
        )

        # =====================================================================
        #                    Low-speed capture RAM
        # =====================================================================

        self.cap_arm = CSRStorage(
            1,
            description="Pulse to arm low-speed internal capture RAM."
        )

        self.cap_idx = CSRStorage(
            16,
            description="Read index for low-speed capture RAM."
        )

        self.cap_done = CSRStatus(
            1,
            description="Indicates low-speed capture RAM has completed."
        )

        self.cap_data = CSRStatus(
            16,
            description="Captured low-speed sample at cap_idx."
        )


        self.magnitude           = CSRStatus(16, description="Downsampled magnitude")
        self.phase               = CSRStatus(25, description="Downsampled phase")
