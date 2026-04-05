from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class CSRFieldSpec:
    name: str
    width: int
    description: str
    kind: str = "storage"
    reset: int = 0
    uc_name: str | None = None
    uc_config: bool = False


def storage(name: str, width: int, description: str, *, reset: int = 0,
            uc_name: str | None = None, uc_config: bool = False) -> CSRFieldSpec:
    return CSRFieldSpec(
        name=name,
        width=width,
        description=description,
        kind="storage",
        reset=reset,
        uc_name=uc_name,
        uc_config=uc_config,
    )


def status(name: str, width: int, description: str) -> CSRFieldSpec:
    return CSRFieldSpec(
        name=name,
        width=width,
        description=description,
        kind="status",
    )


def _config_fields():
    yield storage(
        "example_control", 8,
        "Example writable control register for CSR integration/testing."
    )
    yield storage(
        "phase_inc_nco", 26,
        "Main NCO phase increment controlling carrier frequency.",
        uc_config=True,
    )
    for ch in range(1, 6):
        yield storage(
            f"phase_inc_down_{ch}", 26,
            f"Downsampler phase increment for channel {ch}.",
            uc_config=True,
        )
    yield storage(
        "phase_inc_down_ref", 26,
        "Reference downsampler phase increment.",
        uc_config=True,
    )
    for ch in range(1, 6):
        yield storage(
            f"phase_inc_cpu{ch}", 26,
            f"CPU-controlled NCO phase increment for channel {ch}.",
            uc_config=True,
        )

    yield storage(
        "nco_mag", 12,
        "Magnitude applied to main NCO output (signed).",
        uc_config=True,
    )
    for ch in range(1, 6):
        yield storage(
            f"mag_cpu{ch}", 12,
            f"Magnitude applied to CPU NCO channel {ch} (signed).",
            uc_config=True,
        )

    yield storage(
        "input_select", 2,
        "Selects signal source feeding the DSP pipeline.",
        uc_config=True,
    )
    yield storage(
        "upsampler_input_mux", 2,
        "Selects input source for the upsampler stage.",
        uc_config=True,
    )
    yield storage(
        "output_select_ch1", 4,
        "Output mux selection for DAC channel 1.",
        uc_name="output_sel_ch1",
        uc_config=True,
    )
    yield storage(
        "output_select_ch2", 4,
        "Output mux selection for DAC channel 2.",
        uc_name="output_sel_ch2",
        uc_config=True,
    )
    yield storage(
        "lowspeed_dbg_select", 3,
        "Selects low-speed debug signal exported by the DSP.",
        uc_config=True,
    )
    yield storage(
        "highspeed_dbg_select", 3,
        "Selects high-speed debug signal exported by the DSP.",
        uc_config=True,
    )

    for ch in range(1, 6):
        yield storage(
            f"gain{ch}", 32,
            f"Gain coefficient applied to channel {ch}.",
            uc_config=True,
        )

    for ch in range(1, 6):
        yield storage(
            f"upsampler_input_x{ch}", 16,
            f"Direct X (I) input sample for upsampler channel {ch}.",
            uc_name=f"ups_in_x{ch}",
            uc_config=True,
        )
        yield storage(
            f"upsampler_input_y{ch}", 16,
            f"Direct Y (Q) input sample for upsampler channel {ch}.",
            uc_name=f"ups_in_y{ch}",
            uc_config=True,
        )

    yield storage(
        "final_shift", 3,
        "Final arithmetic right-shift applied to DSP output.",
        uc_config=True,
    )
    yield storage(
        "cap_enable", 1,
        "Enable high-speed capture from DSP into DDR memory.",
        uc_config=True,
    )
    yield storage(
        "cap_beats", 32,
        "Number of 256-bit beats captured into DDR.",
        reset=256,
        uc_config=True,
    )
    yield storage(
        "cap_arm", 1,
        "Pulse to arm low-speed internal capture RAM.",
        uc_config=True,
    )
    yield storage(
        "cap_idx", 16,
        "Read index for low-speed capture RAM.",
        uc_config=True,
    )


def _ds_fifo_fields():
    yield storage(
        "ds_fifo_pop", 1,
        "Write (strobe) to pop one 5-channel downsampled frame from the FIFO."
    )
    for ch in range(1, 6):
        yield status(
            f"ds_fifo_x{ch}", 16,
            f"Latched X sample for channel {ch} from the last popped downsample FIFO frame."
        )
        yield status(
            f"ds_fifo_y{ch}", 16,
            f"Latched Y sample for channel {ch} from the last popped downsample FIFO frame."
        )
    yield status(
        "ds_fifo_overflow", 1,
        "Sticky: downsample frame FIFO overflow (UC tried to enqueue while full)."
    )
    yield status(
        "ds_fifo_underflow", 1,
        "Sticky: downsample frame FIFO underflow (CPU popped while empty)."
    )
    yield storage(
        "ds_fifo_clear", 1,
        "Write (strobe) to clear downsample frame FIFO overflow/underflow flags."
    )
    yield status(
        "ds_fifo_flags", 8,
        "FIFO flags packed into status bits: bit0=readable (SYS can pop). Remaining bits are reserved (0)."
    )


def _ups_fifo_fields():
    for ch in range(1, 6):
        yield storage(
            f"ups_fifo_x{ch}", 16,
            f"Frame X sample for channel {ch} to enqueue into the SYS->UC upsampler FIFO."
        )
        yield storage(
            f"ups_fifo_y{ch}", 16,
            f"Frame Y sample for channel {ch} to enqueue into the SYS->UC upsampler FIFO."
        )
    yield storage(
        "ups_fifo_push", 1,
        "Write (strobe) to enqueue one 5-channel frame into the upsampler FIFO."
    )
    yield status(
        "ups_fifo_overflow", 1,
        "Sticky: upsampler frame FIFO overflow (CPU pushed while full)."
    )
    yield status(
        "ups_fifo_underflow", 1,
        "Sticky: upsampler frame FIFO underflow (UC consumed while empty)."
    )
    yield storage(
        "ups_fifo_clear", 1,
        "Write (strobe) to clear upsampler frame FIFO overflow/underflow flags."
    )
    yield status(
        "ups_fifo_flags", 8,
        "FIFO flags packed into status bits: bit1=writable (CPU can enqueue a frame). Remaining bits are reserved (0)."
    )


def _readback_fields():
    yield status(
        "example_status", 8,
        "Example read-only status register mirroring example_control."
    )
    yield status(
        "cap_done", 1,
        "Indicates low-speed capture RAM has completed."
    )
    yield status(
        "cap_data", 16,
        "Captured low-speed sample at cap_idx."
    )
    yield status(
        "magnitude", 16,
        "Downsampled magnitude"
    )
    yield status(
        "phase", 25,
        "Downsampled phase"
    )


CSR_FIELDS = tuple(
    list(_config_fields())
    + list(_ds_fifo_fields())
    + list(_ups_fifo_fields())
    + list(_readback_fields())
)


def iter_csr_fields(*, kind: str | None = None, uc_config: bool | None = None):
    for field in CSR_FIELDS:
        if kind is not None and field.kind != kind:
            continue
        if uc_config is not None and field.uc_config != uc_config:
            continue
        yield field


def build_uc_config_map(bank):
    return {
        (field.uc_name or field.name): getattr(bank, field.name).storage
        for field in iter_csr_fields(kind="storage", uc_config=True)
    }
