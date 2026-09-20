"""Hand-curated, human-facing metadata layered on top of the raw
``cmdlist`` dump: friendly group titles, per-channel family grouping,
readable enum labels for signal-routing selects, and sensible defaults.

Kept as a pure-Python data + transform module (no Qt), for the same
reason ``cmd_catalog.py`` is: this is the part with actual judgment
calls (which group a command belongs in, what "channel 6" is actually
wired to) and is worth covering with tests independent of any widget.

Firmware commands not listed here still work - ``group_for()`` falls
back to "general" and nothing here touches their parsed arg spec - so a
newly added firmware command doesn't break the panel, it just isn't
specially labeled yet.
"""

from __future__ import annotations

from dataclasses import dataclass, replace

from .cmd_catalog import Command, EnumChoice

# ---------------------------------------------------------------- groups
# (group_key, title, expanded_by_default), in the order they're shown.
# Groups people touch constantly (gain, routing, the core signal chain)
# default open; rarely-touched diagnostics/advanced groups default closed
# so the panel doesn't open as one huge wall of buttons.
GROUPS: tuple[tuple[str, str, bool], ...] = (
    ("input", "Input && NCO", True),  # "&&" -> literal "&" (Qt reads lone "&" as a mnemonic marker)
    ("downconvert", "Downconversion", True),
    ("cpu_nco", "CPU-Injected NCO", True),
    ("gain", "Gain", True),
    ("output", "Output Routing", True),
    ("dsp", "DSP / FIFO", True),
    ("tracking", "Frequency Tracking", False),
    ("siggen", "Signal Generator", False),
    ("capture", "Debug Capture (advanced)", False),
    ("uberddr3", "UberDDR3 / Ethernet", False),
    ("ddr", "DDR Diagnostics", False),
    ("general", "General", False),
)
GROUP_TITLES: dict[str, str] = {key: title for key, title, _ in GROUPS}
GROUP_EXPANDED: dict[str, bool] = {key: expanded for key, _, expanded in GROUPS}
GROUP_ORDER: list[str] = [key for key, _, _ in GROUPS]

# Every command name the firmware currently registers (see
# 2.soc-litex/2.sw/cmd_list.c), mapped to one of the group keys above.
COMMAND_GROUPS: dict[str, str] = {
    "cmdlist": "general",
    "help_uc": "general",
    "fft64_peak": "dsp",
    "phase_nco": "input",
    "nco_mag": "input",
    "input_select": "input",
    "upsampler_input_mux": "output",
    "upsampler_x": "cpu_nco",
    "upsampler_y": "cpu_nco",
    "ds_pop": "dsp",
    "ds_status": "dsp",
    "ups_push": "dsp",
    "ups_status": "dsp",
    "dsp_test": "dsp",
    "dsp_run": "dsp",
    "fft_fs": "dsp",
    "fft_ds": "dsp",
    "fft_ds_peak": "dsp",
    "fft32_ds_y": "dsp",
    "phase": "dsp",
    "magnitude": "dsp",
    "track3": "tracking",
    "trackq_start": "tracking",
    "trackq_probe": "tracking",
    "trackq_stop": "tracking",
    "final_shift": "gain",
    "lowspeed_dbg_select": "capture",
    "highspeed_dbg_select": "capture",
    "cap_arm": "capture",
    "cap_done": "capture",
    "cap_rd": "capture",
    "cap_enable": "capture",
    "cap_beats": "capture",
    "cap_start": "capture",
    "cap_status": "capture",
    "cap_dump": "capture",
    "ub_help": "uberddr3",
    "ub_info": "uberddr3",
    "ub_mode": "uberddr3",
    "ub_setmode": "uberddr3",
    "ub_start": "uberddr3",
    "ub_ramp": "uberddr3",
    "ub_cap": "uberddr3",
    "ub_wait": "uberddr3",
    "ub_hexdump": "uberddr3",
    "ub_send": "uberddr3",
    "sig3_start": "siggen",
    "sig3_stop": "siggen",
    "sig3_amp": "siggen",
    "sig3_freqs": "siggen",
    "sig3_enable_ch": "siggen",
    "sig3_disable_ch": "siggen",
    "help_ddr": "ddr",
    "ddrinfo": "ddr",
    "ddrwait": "ddr",
    "ddrprobe": "ddr",
    "ddrbyte": "ddr",
    "ddrtest": "ddr",
    "ddrtestb": "ddr",
    "ddrmap": "ddr",
    "ddrpat": "ddr",
    "timertest": "ddr",
    "timeinfo": "ddr",
    "output_select_ch1": "output",
    "output_select_ch2": "output",
    "phase_down_ref": "downconvert",
    # gain1..5 / phase_down_1..5 / phase_cpu1..5 / mag_cpu1..5 are folded
    # entirely into FAMILIES below and never rendered individually, but
    # are listed here too so group_for() still answers sensibly for them.
    **{f"gain{n}": "gain" for n in range(1, 6)},
    **{f"phase_down_{n}": "downconvert" for n in range(1, 6)},
    **{f"phase_cpu{n}": "cpu_nco" for n in range(1, 6)},
    **{f"mag_cpu{n}": "cpu_nco" for n in range(1, 6)},
}


def group_for(command_name: str) -> str:
    return COMMAND_GROUPS.get(command_name, "general")


# ------------------------------------------------------------ enum labels
# Friendly labels for signal-routing selects, taken from the actual
# hardware mux wiring (1.hw/uberclock/uberclock.v) rather than guessed
# from the short firmware help string.
_LS_DBG_LABELS: tuple[tuple[int, str], ...] = (
    (0, "Gain out Y1"), (1, "Gain out Y2"), (2, "Gain out Y3"),
    (3, "Gain out Y4"), (4, "Gain out Y5"), (5, "Upsampler in X1"),
    (6, "Ref Y (downsampled)"), (7, "Unused (0)"),
)
_HS_DBG_LABELS: tuple[tuple[int, str], ...] = (
    (0, "Filter input"), (1, "Filter input 1"),
    (2, "Sum (scaled)"), (3, "NCO cosine"),
)
_OUTPUT_SELECT_LABELS: tuple[tuple[int, str], ...] = (
    (0, "Gain out Y1"), (1, "Gain out Y2"), (2, "Gain out Y3"),
    (3, "Gain out Y4"), (4, "Gain out Y5"),
    (5, "TX channel 1"), (6, "TX channel 2"), (7, "TX channel 3"),
    (8, "TX channel 4"), (9, "TX channel 5"),
    (10, "NCO cosine"), (11, "Filter input"), (12, "Upsampler in X1"),
    (13, "Filter input 1"), (14, "Ref Y (downsampled)"), (15, "Sum"),
)

ENUM_OVERRIDES: dict[str, tuple[tuple[int, str], ...]] = {
    "lowspeed_dbg_select": _LS_DBG_LABELS,
    "highspeed_dbg_select": _HS_DBG_LABELS,
    "output_select_ch1": _OUTPUT_SELECT_LABELS,
    "output_select_ch2": _OUTPUT_SELECT_LABELS,
}


def apply_enum_overrides(commands: list[Command]) -> list[Command]:
    """Replace known commands' parsed arg spec with a friendlier
    ``EnumChoice`` built from real hardware signal names. Commands not in
    ``ENUM_OVERRIDES`` pass through untouched."""
    result = []
    for command in commands:
        options = ENUM_OVERRIDES.get(command.name)
        result.append(replace(command, args=EnumChoice(options=options)) if options else command)
    return result


# -------------------------------------------------------------- defaults
# Only set where the meaning is actually known (unity gain, "off"/zero) -
# guessing a plausible-looking default for a command we don't understand
# would be worse than leaving the widget's own default in place.
DEFAULTS: dict[str, str] = {
    **{f"gain{n}": "0x40000000" for n in range(1, 6)},
    "final_shift": "0",
    "lowspeed_dbg_select": "0",
    "highspeed_dbg_select": "0",
}


# --------------------------------------------------------------- families
@dataclass(frozen=True)
class Family:
    """A set of per-channel commands (``gain1``..``gain5``, ...) that
    share one arg spec and differ only by channel - rendered as a single
    channel dropdown + value control instead of one row per channel."""

    label: str
    group: str
    help: str
    channels: tuple[tuple[str, str], ...]  # (channel_label, command_name)
    default: str = ""


FAMILIES: tuple[Family, ...] = (
    Family(
        label="Gain",
        group="gain",
        help="Fixed-point gain coefficient, Q2.30 (0x40000000 = unity gain, 1.0).",
        channels=tuple((f"Channel {n}", f"gain{n}") for n in range(1, 6)),
        default="0x40000000",
    ),
    Family(
        label="Downconversion phase",
        group="downconvert",
        help="Downconversion NCO phase increment (0..524287).",
        channels=tuple((f"Channel {n}", f"phase_down_{n}") for n in range(1, 6))
        + (("Ref", "phase_down_ref"),),
    ),
    Family(
        label="CPU NCO phase",
        group="cpu_nco",
        help="CPU-injected NCO phase increment (0..16777215).",
        channels=tuple((f"Channel {n}", f"phase_cpu{n}") for n in range(1, 6)),
    ),
    Family(
        label="CPU NCO magnitude",
        group="cpu_nco",
        help="CPU-injected NCO magnitude, signed 12-bit (-2048..2047).",
        channels=tuple((f"Channel {n}", f"mag_cpu{n}") for n in range(1, 6)),
    ),
    Family(
        label="DAC output source",
        group="output",
        help="Selects which internal signal drives each DAC output.",
        channels=(("DAC 1", "output_select_ch1"), ("DAC 2", "output_select_ch2")),
    ),
)


def family_commands() -> set[str]:
    """Every command name consumed by a family - used to exclude them
    from the individual per-command listing so they aren't shown twice."""
    return {name for family in FAMILIES for _, name in family.channels}
