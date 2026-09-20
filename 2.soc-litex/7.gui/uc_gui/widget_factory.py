"""Turn a parsed :class:`~uc_gui.cmd_catalog.Command` into Qt widgets.

All PySide6 code for "one command -> one widget" lives here, kept
separate from ``cmd_catalog.py``'s plain-Python parsing so the parsing
logic stays unit-testable without a Qt application instance.
"""

from __future__ import annotations

from typing import Callable, Optional

from PySide6.QtWidgets import QComboBox, QHBoxLayout, QLabel, QLineEdit, QPushButton, QSpinBox, QWidget

from .cmd_catalog import Command, EnumChoice, FreeText, IntRange, NoArgs
from .command_meta import Family

# Sends a fully-formatted command line (e.g. "cap_beats 4096") to the board.
SendCommand = Callable[[str], None]


def make_value_widget(command: Command, default: str = "") -> Optional[QWidget]:
    """Build the input widget for a command's argument.

    Returns ``None`` for commands that take no argument, so callers can
    tell "no widget needed" apart from "unhandled case" (which raises).

    ``default`` (e.g. "0x40000000") pre-fills the widget when it parses
    against the arg spec; an unparseable or out-of-range default is
    silently ignored rather than raised, so a bad guess in
    ``command_meta.DEFAULTS`` degrades to "no default" instead of
    crashing the panel.
    """
    args = command.args

    if isinstance(args, NoArgs):
        return None

    if isinstance(args, IntRange):
        box = QSpinBox()
        box.setRange(args.low, args.high)
        value = _parse_int(default)
        if value is not None and args.low <= value <= args.high:
            box.setValue(value)
        return box

    if isinstance(args, EnumChoice):
        box = QComboBox()
        for value, label in args.options:
            box.addItem(f"{label} ({value})", userData=value)
        default_value = _parse_int(default)
        if default_value is not None:
            index = box.findData(default_value)
            if index >= 0:
                box.setCurrentIndex(index)
        return box

    if isinstance(args, FreeText):
        edit = QLineEdit()
        edit.setText(default)
        return edit

    raise TypeError(f"Unhandled arg spec type: {type(args).__name__}")


def _parse_int(text: str) -> Optional[int]:
    """Parse an int accepting "0x..." hex, returning None instead of
    raising for anything that isn't a whole number."""
    try:
        return int(text, 0)
    except (TypeError, ValueError):
        return None


def value_widget_text(widget: QWidget) -> str:
    """Read the current value out of a widget built by :func:`make_value_widget`,
    formatted the way the firmware console expects it as a command argument."""
    if isinstance(widget, QSpinBox):
        return str(widget.value())
    if isinstance(widget, QComboBox):
        return str(widget.currentData())
    if isinstance(widget, QLineEdit):
        return widget.text().strip()
    raise TypeError(f"Unhandled widget type: {type(widget).__name__}")


class CommandRow(QWidget):
    """One command as a row: ``[optional value widget] [Send button]``.

    Clicking the button sends ``"<name> <value>"`` (or just ``"<name>"``
    for no-argument commands) through ``send_command``.
    """

    def __init__(
        self,
        command: Command,
        send_command: SendCommand,
        default: str = "",
        parent: QWidget | None = None,
    ):
        super().__init__(parent)
        self.command = command
        self._send_command = send_command
        self._value_widget = make_value_widget(command, default=default)

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        if self._value_widget is not None:
            layout.addWidget(self._value_widget, 1)

        button = QPushButton(command.name)
        button.setToolTip(command.help)
        button.clicked.connect(self._on_send_clicked)
        layout.addWidget(button)

    def _on_send_clicked(self) -> None:
        if self._value_widget is None:
            self._send_command(self.command.name)
        else:
            value = value_widget_text(self._value_widget)
            self._send_command(f"{self.command.name} {value}")


class FamilyRow(QWidget):
    """A `command_meta.Family` as one row: a channel dropdown, one shared
    value widget, and a Send button - replaces e.g. five separate
    ``gain1``..``gain5`` rows with a single "Gain" row."""

    def __init__(
        self,
        family: Family,
        representative: Command,
        send_command: SendCommand,
        parent: QWidget | None = None,
    ):
        super().__init__(parent)
        self._send_command = send_command
        self._channel_commands = [name for _, name in family.channels]

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        label = QLabel(family.label)
        label.setToolTip(family.help)
        layout.addWidget(label)

        self.channel_box = QComboBox()
        for channel_label, _ in family.channels:
            self.channel_box.addItem(channel_label)
        layout.addWidget(self.channel_box)

        self._value_widget = make_value_widget(representative, default=family.default)
        if self._value_widget is not None:
            layout.addWidget(self._value_widget, 1)

        button = QPushButton("Send")
        button.setToolTip(family.help)
        button.clicked.connect(self._on_send_clicked)
        layout.addWidget(button)

    def _on_send_clicked(self) -> None:
        command_name = self._channel_commands[self.channel_box.currentIndex()]
        if self._value_widget is None:
            self._send_command(command_name)
        else:
            value = value_widget_text(self._value_widget)
            self._send_command(f"{command_name} {value}")
