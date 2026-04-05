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
from .uberclock_regspec import iter_csr_fields

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
        for field in iter_csr_fields():
            csr_cls = CSRStorage if field.kind == "storage" else CSRStatus
            kwargs = {
                "name": field.name,
                "description": field.description,
            }
            if field.kind == "storage":
                kwargs["reset"] = field.reset
            setattr(self, field.name, csr_cls(field.width, **kwargs))
