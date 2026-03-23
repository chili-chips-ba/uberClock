from __future__ import annotations

from typing import Mapping

from migen import *
from litex.gen import *
from litex.soc.interconnect.csr import CSRStorage, CSRStatus
from migen.genlib.fifo import AsyncFIFO
from migen.genlib.cdc import ClockDomainsRenamer


# =============================================================================
# CSR config snapshot / atomic commit across clock domains (SYS -> UC)
# =============================================================================
class CsrConfigSnapshotFIFO(LiteXModule):
    """
    Atomic CSR config transfer from SYS -> UC.

    Software writes any number of CSRs, then strobes `commit` to enqueue one
    packed config frame. UC pops frames from an AsyncFIFO and updates shadow
    registers atomically (all fields change on the same UC cycle).

    For each field name "foo", this module exports a UC-domain shadow register:
        self.cfg_foo_uc
    """

    FIFO_FLAG_READABLE_BIT = 0
    FIFO_FLAG_WRITABLE_BIT = 1
    FIFO_FLAG_RESERVED_BITS = 6

    def __init__(
        self,
        fields: Mapping[str, Signal],
        cd_write: str = "sys",
        cd_read: str = "uc",
        fifo_depth: int = 4,
        *,
        commit_description: str | None = None,
    ):
        # ----------------------------
        # CSRs (write domain)
        # ----------------------------
        self.commit = CSRStorage(
            1,
            description=(
                commit_description
                if commit_description is not None
                else "Write (strobe) to snapshot all configured fields and enqueue one config frame for the UC domain."
            ),
        )
        self.overflow = CSRStatus(
            1,
            description="Sticky: commit attempted while config FIFO was full (frame dropped).",
        )
        self.fifo_flags = CSRStatus(
            8,
            description=(
                "FIFO flags packed into status bits: "
                "bit0=readable (UC has pending frame), bit1=writable (SYS can enqueue). "
                "Remaining bits are reserved (0)."
            ),
        )

        # ----------------------------
        # Packed frame width
        # ----------------------------
        total_width = sum(len(sig) for sig in fields.values())
        assert total_width > 0, "CsrConfigSnapshotFIFO requires at least one field."

        frame_sys = Signal(total_width, name="cfg_frame_sys")
        frame_uc  = Signal(total_width, name="cfg_frame_uc")

        # ----------------------------
        # Pack SYS signals into frame_sys (LSB-first)
        # ----------------------------
        offset = 0
        for name, sig in fields.items():
            w = len(sig)
            self.comb += frame_sys[offset:offset + w].eq(sig)
            offset += w

        # ----------------------------
        # Async FIFO crossing write->read domains
        # ----------------------------
        fifo = AsyncFIFO(width=total_width, depth=fifo_depth)
        self.submodules.fifo = ClockDomainsRenamer({"write": cd_write, "read": cd_read})(fifo)

        # Commit pulse in write domain (1 cycle)
        commit_pulse_sys = Signal(name="cfg_commit_pulse_sys")
        self.comb += commit_pulse_sys.eq(self.commit.re)

        # Write-side: push frame if FIFO is writable
        self.comb += [
            fifo.din.eq(frame_sys),
            fifo.we.eq(commit_pulse_sys & fifo.writable),
        ]

        # Grab domain sync objects once
        sync_w = getattr(self.sync, cd_write)
        sync_r = getattr(self.sync, cd_read)

        # Sticky overflow flag (write domain)
        sync_w += If(commit_pulse_sys & ~fifo.writable,
            self.overflow.status.eq(1)
        )

        # SYS-visible FIFO flags
        self.comb += self.fifo_flags.status.eq(
            Cat(fifo.readable, fifo.writable, C(0, self.FIFO_FLAG_RESERVED_BITS))
        )

        # ----------------------------
        # Read-side: pop frames and update shadow regs
        # ----------------------------
        self.cfg_commit_uc = Signal(name="cfg_commit_uc")  # 1-cycle strobe in read domain

        pop_frame_uc = Signal(name="cfg_pop_frame_uc")
        self.comb += [
            pop_frame_uc.eq(fifo.readable),
            fifo.re.eq(pop_frame_uc),
        ]

        # Latch FIFO output and generate 1-cycle commit strobe (read domain)
        sync_r += [
            self.cfg_commit_uc.eq(0),
            If(pop_frame_uc,
                frame_uc.eq(fifo.dout),
                self.cfg_commit_uc.eq(1),
            ),
        ]

        # ----------------------------
        # Unpack UC shadow registers
        # ----------------------------
        offset = 0
        for name, sig in fields.items():
            w = len(sig)
            out = Signal(w, name=f"cfg_{name}_uc")
            setattr(self, f"cfg_{name}_uc", out)

            sync_r += If(self.cfg_commit_uc,
                out.eq(frame_uc[offset:offset + w])
            )
            offset += w
