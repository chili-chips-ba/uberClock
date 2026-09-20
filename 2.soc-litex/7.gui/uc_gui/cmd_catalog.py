"""Parse the firmware's ``cmdlist`` output into typed Python objects.

No Qt dependency here on purpose: this module only knows about the text
protocol produced by ``cmd_cmdlist()`` in ``2.soc-litex/2.sw/cmd_list.c``,
which prints one ``name\\thelp\\targ_spec`` line per registered command
between ``--CMDLIST-BEGIN--`` / ``--CMDLIST-END--`` markers. Keeping the
parsing logic Qt-free means it can be unit-tested with plain ``unittest``
and no display, and reused later.

The ``arg_spec`` grammar (documented next to the command table in
``cmd_list.c`` and in ``console.h``) is:

    "none"                   -> command takes no argument
    "int:<low>:<high>"       -> one integer argument, inclusive range
    "enum:<v>=<label>,..."   -> one integer argument, restricted to
                                labeled values
    "" (empty / anything else) -> free-form text argument

The firmware prints "" (not the literal string "NULL") when a command
was registered with a NULL help or arg_spec, so the fallback case must
also cover the empty string.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Union

BEGIN_MARKER = "--CMDLIST-BEGIN--"
END_MARKER = "--CMDLIST-END--"


@dataclass(frozen=True)
class NoArgs:
    """Command takes no arguments (``arg_spec == "none"``)."""


@dataclass(frozen=True)
class IntRange:
    """Command takes one integer argument within ``[low, high]``."""

    low: int
    high: int


@dataclass(frozen=True)
class EnumChoice:
    """Command takes one integer argument, restricted to labeled values."""

    options: tuple[tuple[int, str], ...]  # ((value, label), ...)


@dataclass(frozen=True)
class FreeText:
    """Command takes arbitrary text (arg_spec was empty/unrecognized)."""


ArgSpec = Union[NoArgs, IntRange, EnumChoice, FreeText]


@dataclass(frozen=True)
class Command:
    """One console command, as advertised by the firmware."""

    name: str
    help: str
    args: ArgSpec


class CmdListParseError(ValueError):
    """Raised when ``cmdlist`` output doesn't contain the expected markers."""


def parse_arg_spec(raw: str) -> ArgSpec:
    """Translate one ``arg_spec`` field into a typed :class:`ArgSpec`."""
    if raw == "none":
        return NoArgs()

    if raw.startswith("int:"):
        _, low, high = raw.split(":")
        return IntRange(low=int(low), high=int(high))

    if raw.startswith("enum:"):
        options = []
        for pair in raw[len("enum:"):].split(","):
            value, label = pair.split("=")
            options.append((int(value), label))
        return EnumChoice(options=tuple(options))

    return FreeText()


def parse_cmdlist(output: str) -> list[Command]:
    """Parse the full text output of the firmware's ``cmdlist`` command.

    Lines outside the BEGIN/END markers (boot banner, console echo, a
    partially-received previous line) are ignored, and any line that
    doesn't split into exactly 3 tab-separated fields is skipped rather
    than aborting the whole parse - stray noise on a UART link is
    normal and shouldn't take down the command panel.

    Raises:
        CmdListParseError: if both markers aren't present at all, since
            that means the response isn't a ``cmdlist`` dump.
    """
    if BEGIN_MARKER not in output or END_MARKER not in output:
        raise CmdListParseError(
            f"cmdlist output missing {BEGIN_MARKER}/{END_MARKER} markers"
        )

    body = output.split(BEGIN_MARKER, 1)[1].split(END_MARKER, 1)[0]

    commands = []
    for line in body.splitlines():
        if not line.strip():
            continue
        # Deliberately not stripping `line` itself: a trailing empty
        # arg_spec field (e.g. "cap_beats\thelp text\t") ends in a tab,
        # and str.strip() treats tabs as whitespace - stripping would
        # silently swallow that empty field and drop the whole command.
        fields = line.split("\t")
        if len(fields) != 3:
            continue
        name, help_text, raw_arg_spec = fields
        commands.append(Command(name=name, help=help_text, args=parse_arg_spec(raw_arg_spec)))
    return commands
