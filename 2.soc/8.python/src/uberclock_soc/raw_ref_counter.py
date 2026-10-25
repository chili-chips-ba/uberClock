"""DSP-cycle counter gated by Schmitt crossings of signed raw ADC2 samples.

Core uses its local synchronous domain (renamed to UC by the integrator).
Output is a coherent registered record with a one-cycle valid pulse. No mixer,
filter, decimation enable, nominal sample-rate conversion or CPU timer is used.
"""
from migen import Module, Signal, If, Mux, Cat
from migen.genlib.cdc import ClockDomainsRenamer, MultiReg
from migen.genlib.fifo import AsyncFIFO


class RawReferenceCounter(Module):
    def __init__(self, timeout_cycles=130_000_000):
        self.sample = Signal((12, True))
        self.enable = Signal()
        self.periods = Signal(32, reset=1_000_000)
        self.hysteresis = Signal(12, reset=16)
        self.valid = Signal()
        self.cycles = Signal(32)
        self.measured_periods = Signal(32)
        self.adc_min = Signal((12, True))
        self.adc_max = Signal((12, True))
        self.min_period_cycles = Signal(32)
        self.max_period_cycles = Signal(32)
        self.result_flags = Signal(8)
        self.end_tick = Signal(64)
        self.sequence = Signal(32)

        tick = Signal(64)
        armed = Signal()
        active = Signal()
        cycles = Signal(32)
        edges = Signal(32)
        target = Signal(32)
        period = Signal(32)
        pmin = Signal(32, reset=0xffffffff)
        pmax = Signal(32)
        amin = Signal((12, True))
        amax = Signal((12, True))
        watchdog = Signal(32)
        bad_sent = Signal()
        threshold = Signal((13, True))
        crossing = Signal()
        next_pmin = Signal(32)
        next_pmax = Signal(32)
        next_amin = Signal((12, True))
        next_amax = Signal((12, True))
        quality = Signal(8)
        next_quality = Signal(8)
        config_ok = Signal()
        self.comb += [
            threshold.eq(self.hysteresis),
            crossing.eq(armed & (self.sample >= threshold)),
            config_ok.eq((self.periods > 0) & (self.periods <= 10_000_000) &
                         (self.hysteresis > 0) & (self.hysteresis <= 1023)),
            next_pmin.eq(Mux(period + 1 < pmin, period + 1, pmin)),
            next_pmax.eq(Mux(period + 1 > pmax, period + 1, pmax)),
            next_amin.eq(Mux(self.sample < amin, self.sample, amin)),
            next_amax.eq(Mux(self.sample > amax, self.sample, amax)),
            # At nominal 65 MHz / 10 MHz, expected edge spacing is 6 or 7.
            next_quality.eq(quality | Mux((period + 1 < 3) | (period + 1 > 16), 4, 0)),
        ]
        self.sync += [
            tick.eq(tick + 1),
            self.valid.eq(0),
            If(~self.enable,
                armed.eq(0), active.eq(0), cycles.eq(0), edges.eq(0),
                period.eq(0), watchdog.eq(0), bad_sent.eq(0), quality.eq(0),
                self.sequence.eq(0),
            ).Elif(~config_ok,
                armed.eq(0), active.eq(0), watchdog.eq(0),
                If(~bad_sent,
                    self.valid.eq(1), self.result_flags.eq(2),
                    self.cycles.eq(0), self.measured_periods.eq(0),
                    self.adc_min.eq(self.sample), self.adc_max.eq(self.sample),
                    self.min_period_cycles.eq(0), self.max_period_cycles.eq(0),
                    self.end_tick.eq(tick), self.sequence.eq(self.sequence + 1),
                    bad_sent.eq(1),
                )
            ).Else(
                bad_sent.eq(0), watchdog.eq(watchdog + 1),
                If(self.sample <= -threshold, armed.eq(1)),
                If(crossing, armed.eq(0)),
                If(active,
                    cycles.eq(cycles + 1), period.eq(period + 1),
                    amin.eq(next_amin), amax.eq(next_amax),
                ),
                If(watchdog == timeout_cycles - 1,
                    self.valid.eq(1), self.result_flags.eq(1 | quality),
                    self.cycles.eq(Mux(active, cycles + 1, 0)),
                    self.measured_periods.eq(Mux(active, edges, 0)),
                    self.adc_min.eq(Mux(active, next_amin, self.sample)),
                    self.adc_max.eq(Mux(active, next_amax, self.sample)),
                    self.min_period_cycles.eq(Mux(active & (edges != 0), pmin, 0)),
                    self.max_period_cycles.eq(Mux(active, pmax, 0)),
                    self.end_tick.eq(tick), self.sequence.eq(self.sequence + 1),
                    active.eq(0), watchdog.eq(0), armed.eq(0),
                ).Elif(crossing,
                    If(~active,
                        active.eq(1), cycles.eq(0), edges.eq(0),
                        target.eq(self.periods), period.eq(0),
                        pmin.eq(0xffffffff), pmax.eq(0), quality.eq(0),
                        amin.eq(self.sample), amax.eq(self.sample), watchdog.eq(0),
                    ).Else(
                        period.eq(0), pmin.eq(next_pmin), pmax.eq(next_pmax),
                        quality.eq(next_quality), edges.eq(edges + 1),
                        If(edges == target - 1,
                            self.valid.eq(1), self.result_flags.eq(next_quality),
                            self.cycles.eq(cycles + 1), self.measured_periods.eq(target),
                            self.adc_min.eq(next_amin), self.adc_max.eq(next_amax),
                            self.min_period_cycles.eq(next_pmin), self.max_period_cycles.eq(next_pmax),
                            self.end_tick.eq(tick), self.sequence.eq(self.sequence + 1),
                            # Closing edge is also start of next measurement.
                            cycles.eq(0), edges.eq(0), target.eq(self.periods),
                            pmin.eq(0xffffffff), pmax.eq(0), quality.eq(0),
                            amin.eq(self.sample), amax.eq(self.sample), watchdog.eq(0),
                        )
                    )
                )
            )
        ]


class RawReferenceCounterCDC(Module):
    """UC counter to SYS atomic result FIFO. Config inputs already belong to UC."""
    def __init__(self, timeout_cycles=130_000_000):
        self.submodules.counter = counter = ClockDomainsRenamer("uc")(RawReferenceCounter(timeout_cycles))
        self.sample = counter.sample
        self.enable = counter.enable
        self.periods = counter.periods
        self.hysteresis = counter.hysteresis
        self.pop = Signal()
        self.record = Signal(256)
        self.overflow = Signal()
        self.running = Signal()
        fifo = AsyncFIFO(width=256, depth=16)
        self.submodules.result_fifo = ClockDomainsRenamer({"write": "uc", "read": "sys"})(fifo)
        self.readable = fifo.readable
        self.comb += [
            fifo.din.eq(Cat(counter.cycles, counter.measured_periods, counter.adc_min, counter.adc_max,
                counter.min_period_cycles, counter.max_period_cycles, counter.result_flags,
                counter.end_tick, counter.sequence)),
            fifo.we.eq(counter.valid & counter.enable & fifo.writable),
            fifo.re.eq(self.pop & fifo.readable),
        ]
        overflow_uc = Signal()
        self.sync.uc += If(~counter.enable, overflow_uc.eq(0)).Elif(counter.valid & ~fifo.writable, overflow_uc.eq(1))
        self.specials += MultiReg(overflow_uc, self.overflow, "sys")
        self.specials += MultiReg(counter.enable, self.running, "sys")
        self.sync += If(self.pop & fifo.readable, self.record.eq(fifo.dout))
