# streams.py
#
# UberClock high-speed streaming primitives (UC domain)
# =====================================================
# UC-domain streaming building blocks used to feed the UberDDR3 S2MM DMA writer:
#
#   1) RampSource:
#        Deterministic 16-bit-lane ramp packed into BUS_DATA_WIDTH-bit beats.
#
#   2) SamplePackerStream:
#        Samples one value per UC cycle, packs 16-bit samples into BUS_DATA_WIDTH
#        beats (LANES = BUS_DATA_WIDTH/16), and outputs a valid/ready beat stream.
#
#   3) UCStreamMux:
#        Select between RampSource and an external beat-stream.
#
# Notes:
#   - These blocks are UC-domain only. CDC to sys must be done outside.
#   - Migen Signal() does NOT accept `description=...` (that's for LiteX CSRs).
#

from __future__ import annotations

from migen import *
from litex.gen import *
from migen.genlib.fifo import SyncFIFO


# -----------------------------------------------------------------------------
# RampSource
# -----------------------------------------------------------------------------
class RampSource(LiteXModule):
    """
    Generates a 16-bit-lane ramp packed into bus_data_width-bit beats.

    Packing:
      - Beat width: bus_data_width bits
      - Lanes:      LANES = bus_data_width / 16
      - Lane i contains: (base_step + i) as a 16-bit value
      - base_step increments by LANES per accepted beat
    """

    def __init__(self, bus_data_width: int = 256, max_beats: int = (1 << 23)):
        assert bus_data_width % 16 == 0
        LANES = bus_data_width // 16

        # Control / stream interface
        self.start = Signal()  # pulse to start ramp streaming (UC domain)

        self.valid = Signal()
        self.ready = Signal()
        self.data  = Signal(bus_data_width)
        self.bytes = Signal(max=(bus_data_width // 8) + 1)
        self.last  = Signal()

        self.length_beats = Signal(32, reset=256)

        # Internal state
        running   = Signal()
        base_step = Signal(16)              # lane0 value (increments by LANES per beat)
        beat_idx  = Signal(max=max_beats)   # beat index counter

        # Clamp length into [1..MAX_BEATS]
        length_clamped = Signal.like(beat_idx)
        self.comb += [
            If(self.length_beats == 0,
                length_clamped.eq(1)
            ).Elif(self.length_beats >= max_beats,
                length_clamped.eq(max_beats)
            ).Else(
                length_clamped.eq(self.length_beats[:len(beat_idx)])
            )
        ]

        # Beat counter / ramp progression
        self.sync += [
            If(self.start,
                running.eq(1),
                beat_idx.eq(0),
                base_step.eq(0),
            ).Elif(running & self.ready,
                base_step.eq(base_step + LANES),
                If(beat_idx == (length_clamped - 1),
                    running.eq(0)
                ).Else(
                    beat_idx.eq(beat_idx + 1)
                )
            )
        ]

        # Outputs
        self.comb += [
            self.valid.eq(running),
            self.bytes.eq(bus_data_width // 8),
            self.last.eq(running & (beat_idx == (length_clamped - 1))),
        ]

        # Ramp packing: lane i = base_step + i
        for i in range(LANES):
            self.comb += self.data[16*i:16*(i+1)].eq(base_step + i)


# -----------------------------------------------------------------------------
# SamplePackerStream
# -----------------------------------------------------------------------------
class SamplePackerStream(LiteXModule):
    """
    Samples one value per UC cycle, packs 16-bit samples into bus_data_width beats.

    Packing:
      - Each UC cycle produces one 16-bit sample (sign-extended from sample_width)
      - LANES = bus_data_width / 16 samples are packed into one beat
      - The first sample goes to lane 0 (bits [15:0]), then lane 1, ..., lane LANES-1
      - After LANES cycles, one beat is enqueued into the beat FIFO

    Output:
      - Beat FIFO drains using valid/ready
      - `frames` is the number of beats to capture/emit (one beat = LANES samples)
      - overflow=1 means the beat FIFO could not accept completed beats in time
    """

    def __init__(self, sample_width: int = 12, bus_data_width: int = 256, beat_fifo_depth: int = 512):
        assert bus_data_width % 16 == 0
        LANES = bus_data_width // 16

        # Inputs
        self.sample_in = Signal(sample_width)
        self.start     = Signal()     # pulse
        self.frames    = Signal(32)   # number of beats (frames) to capture/emit

        # Stream outputs
        self.valid = Signal()
        self.ready = Signal()
        self.data  = Signal(bus_data_width)
        self.bytes = Signal(max=(bus_data_width // 8) + 1)
        self.last  = Signal()

        self.overflow = Signal()

        self.comb += self.bytes.eq(bus_data_width // 8)

        # Sign-extend to 16-bit
        sample16 = Signal(16)
        self.comb += sample16.eq(
            Cat(self.sample_in,
                Replicate(self.sample_in[sample_width - 1], 16 - sample_width))
        )

        # Beat FIFO (UC domain)
        bf = SyncFIFO(width=bus_data_width, depth=beat_fifo_depth)
        self.submodules.bf = bf

        # Ping-pong packing buffers
        buf0 = Array(Signal(16) for _ in range(LANES))
        buf1 = Array(Signal(16) for _ in range(LANES))

        active   = Signal()              # 0 -> fill buf0, 1 -> fill buf1
        fill_idx = Signal(max=LANES)     # lane being written

        pend0 = Signal()                 # buf0 complete, waiting enqueue
        pend1 = Signal()                 # buf1 complete, waiting enqueue

        running       = Signal()
        frames_packed = Signal(32)       # complete beats formed (not necessarily enqueued yet)
        frames_sent   = Signal(32)       # beats popped downstream

        # Assemble words from buffers
        word0 = Signal(bus_data_width)
        word1 = Signal(bus_data_width)
        for i in range(LANES):
            self.comb += [
                word0[16*i:16*(i+1)].eq(buf0[i]),
                word1[16*i:16*(i+1)].eq(buf1[i]),
            ]

        # Sampling / packing (every UC cycle)
        self.sync += [
            If(self.start,
                running.eq(1),
                active.eq(0),
                fill_idx.eq(0),
                pend0.eq(0),
                pend1.eq(0),
                frames_packed.eq(0),
                frames_sent.eq(0),
                self.overflow.eq(0),
            ).Elif(running & ~self.overflow,
                If(active == 0,
                    buf0[fill_idx].eq(sample16)
                ).Else(
                    buf1[fill_idx].eq(sample16)
                ),

                If(fill_idx == (LANES - 1),
                    If(active == 0,
                        If(pend0 | pend1,
                            self.overflow.eq(1),
                            running.eq(0),
                        ).Else(
                            pend0.eq(1),
                        )
                    ).Else(
                        If(pend0 | pend1,
                            self.overflow.eq(1),
                            running.eq(0),
                        ).Else(
                            pend1.eq(1),
                        )
                    ),

                    active.eq(~active),
                    fill_idx.eq(0),

                    If(frames_packed == (self.frames - 1),
                        running.eq(0),
                    ).Else(
                        frames_packed.eq(frames_packed + 1),
                    )
                ).Else(
                    fill_idx.eq(fill_idx + 1)
                )
            )
        ]

        do_enq0 = Signal()
        do_enq1 = Signal()

        self.comb += [
            do_enq0.eq(pend0 & bf.writable),
            do_enq1.eq(~do_enq0 & pend1 & bf.writable),
        ]

        self.comb += [
            bf.we.eq(do_enq0 | do_enq1),
            bf.din.eq(Mux(do_enq0, word0, word1)),
        ]

        self.sync += [
            If(do_enq0, pend0.eq(0)),
            If(do_enq1, pend1.eq(0)),
        ]

        # Output stream from beat FIFO
        self.comb += [
            self.valid.eq(bf.readable),
            self.data.eq(bf.dout),
            bf.re.eq(self.valid & self.ready),
            self.last.eq(self.valid & (frames_sent == (self.frames - 1))),
        ]

        self.sync += [
            If(self.start,
                frames_sent.eq(0)
            ).Elif(self.valid & self.ready,
                If(frames_sent != (self.frames - 1),
                    frames_sent.eq(frames_sent + 1)
                )
            )
        ]


# -----------------------------------------------------------------------------
# UCStreamMux
# -----------------------------------------------------------------------------
class UCStreamMux(LiteXModule):
    """
    Selects between:
      - RampSource (internal test pattern)
      - External beat stream (typically SamplePackerStream)

    Control:
      - start: starts ramp transfer when use_external=0
      - use_external: 0=ramp, 1=external stream
      - ramp_length_beats: ramp length in beats
    """

    def __init__(self, bus_data_width: int = 256, max_beats: int = (1 << 23)):
        # Control
        self.start        = Signal()
        self.use_external = Signal()

        self.ramp_length_beats = Signal(32, reset=256)

        # Muxed stream outputs
        self.valid = Signal()
        self.ready = Signal()
        self.data  = Signal(bus_data_width)
        self.bytes = Signal(max=(bus_data_width // 8) + 1)
        self.last  = Signal()

        # External stream inputs
        self.ext_valid = Signal()
        self.ext_ready = Signal()
        self.ext_data  = Signal(bus_data_width)
        self.ext_bytes = Signal(max=(bus_data_width // 8) + 1)
        self.ext_last  = Signal()

        # Internal ramp generator
        self.submodules.ramp = RampSource(bus_data_width=bus_data_width, max_beats=max_beats)

        self.comb += [
            self.ramp.length_beats.eq(self.ramp_length_beats),
            self.ramp.start.eq(self.start & ~self.use_external),
        ]

        self.comb += [
            self.valid.eq(Mux(self.use_external, self.ext_valid, self.ramp.valid)),
            self.data .eq(Mux(self.use_external, self.ext_data,  self.ramp.data)),
            self.bytes.eq(Mux(self.use_external, self.ext_bytes, self.ramp.bytes)),
            self.last .eq(Mux(self.use_external, self.ext_last,  self.ramp.last)),
        ]

        self.comb += [
            self.ext_ready.eq(self.ready & self.use_external),
            self.ramp.ready.eq(self.ready & ~self.use_external),
        ]
