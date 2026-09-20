"""Assemble parsed commands into a browsable, organized Qt panel.

This is the piece that answers "give me a window with all commands
nicely formatted": friendly group titles and per-channel families come
from `command_meta` (the curated, testable "policy" layer); this module
turns that organization into actual widgets (the Qt "mechanism" layer).
"""

from __future__ import annotations

from typing import Union

from PySide6.QtWidgets import QGridLayout, QGroupBox, QPushButton, QScrollArea, QVBoxLayout, QWidget

from .cmd_catalog import Command, NoArgs
from .command_meta import (
    DEFAULTS,
    FAMILIES,
    GROUP_EXPANDED,
    GROUP_ORDER,
    GROUP_TITLES,
    Family,
    apply_enum_overrides,
    family_commands,
    group_for,
)
from .widget_factory import CommandRow, FamilyRow, SendCommand

GroupItem = Union[Command, Family]

# Plain (no-argument) buttons are packed into a grid instead of one per
# row, since a lone button otherwise stretches to the full row width for
# no reason - this is what "buttons take way too much space" was about.
_PLAIN_BUTTON_COLUMNS = 3


def organize_commands(commands: list[Command]) -> dict[str, list[GroupItem]]:
    """Group commands for display: apply friendly enum overrides, fold
    per-channel commands into their `Family`, and bucket everything else
    by `command_meta.group_for()`.

    Returns only non-empty groups, in `GROUP_ORDER`. A command with no
    curated group mapping (e.g. one added to the firmware after this
    file was last updated) still shows up, just under "General".
    """
    commands = apply_enum_overrides(commands)
    by_name = {command.name: command for command in commands}
    consumed = family_commands()

    sections: dict[str, list[GroupItem]] = {key: [] for key in GROUP_ORDER}
    for family in FAMILIES:
        # Only show a family if at least one of its channels is actually
        # advertised by this firmware build.
        if any(name in by_name for _, name in family.channels):
            sections[family.group].append(family)

    for command in commands:
        if command.name not in consumed:
            sections[group_for(command.name)].append(command)

    def sort_key(item: GroupItem) -> str:
        return item.label if isinstance(item, Family) else item.name

    return {key: sorted(items, key=sort_key) for key, items in sections.items() if items}


def family_representative(family: Family, by_name: dict[str, Command]) -> Command:
    """The first channel command that's actually present, used to read
    the (possibly enum-overridden) arg spec shared by the whole family."""
    for _, name in family.channels:
        if name in by_name:
            return by_name[name]
    raise KeyError(f"none of {family.label}'s channel commands are present")


class CommandPanel(QWidget):
    """Scrollable, grouped list of command controls built from the
    firmware's `cmdlist` dump. Groups are collapsible `QGroupBox`es;
    which ones start open is decided by `command_meta.GROUP_EXPANDED`."""

    def __init__(self, commands: list[Command], send_command: SendCommand, parent: QWidget | None = None):
        super().__init__(parent)

        overridden = apply_enum_overrides(commands)
        by_name = {command.name: command for command in overridden}

        content = QWidget()
        content_layout = QVBoxLayout(content)

        for group_key, items in organize_commands(commands).items():
            box = QGroupBox(GROUP_TITLES[group_key])
            box.setCheckable(True)
            box.setChecked(GROUP_EXPANDED[group_key])

            inner = QWidget()
            inner.setVisible(GROUP_EXPANDED[group_key])
            box.toggled.connect(inner.setVisible)

            inner_layout = QVBoxLayout(inner)
            plain_commands = [
                item for item in items if isinstance(item, Command) and isinstance(item.args, NoArgs)
            ]
            for item in items:
                if item in plain_commands:
                    continue
                if isinstance(item, Family):
                    representative = family_representative(item, by_name)
                    inner_layout.addWidget(FamilyRow(item, representative, send_command))
                else:
                    inner_layout.addWidget(
                        CommandRow(item, send_command, default=DEFAULTS.get(item.name, ""))
                    )
            if plain_commands:
                inner_layout.addLayout(
                    _plain_button_grid(plain_commands, send_command)
                )

            box_layout = QVBoxLayout(box)
            box_layout.addWidget(inner)
            content_layout.addWidget(box)

        content_layout.addStretch(1)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setWidget(content)

        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.addWidget(scroll)


def _plain_button_grid(commands: list[Command], send_command: SendCommand) -> QGridLayout:
    grid = QGridLayout()
    for index, command in enumerate(commands):
        button = QPushButton(command.name)
        button.setToolTip(command.help)
        button.clicked.connect(lambda _checked=False, name=command.name: send_command(name))
        grid.addWidget(button, index // _PLAIN_BUTTON_COLUMNS, index % _PLAIN_BUTTON_COLUMNS)
    return grid
