"""Render ANSI SGR (color/style) escape codes from the serial console
stream instead of leaking raw escape bytes into the log view.

Both the LiteX BIOS and this firmware's console write CSI SGR sequences
(``\\x1b[<params>m``) to color prompts, e.g. the boot prompt is literally
``\\x1b[92;1mlitex\\x1b[0m> `` (bright green + bold "litex", then reset).
Feeding that straight into ``QTextEdit.insertPlainText`` has no notion of
color and just prints the escape bytes, which is what showed up as
``␛[92;1mlitex␛[0m>`` in the log.

This module is split in two layers on purpose:

- Parsing (``TextStyle``, ``split_ansi``) is plain Python, no Qt, so it's
  unit-testable without a running application.
- Rendering (``AnsiLogView``) is a thin Qt widget that applies the parsed
  styles as it appends text.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, replace

# Matches any CSI sequence: ESC '[' <params> <final-letter>. SGR sequences
# (color/style) end in 'm'; sequences ending in other letters (cursor
# movement, erase-line, ...) are recognized so they're consumed instead of
# leaking into the visible text, but they carry no style change.
_CSI_RE = re.compile(r"\x1b\[([0-9;]*)([A-Za-z])")

# Matches a CSI sequence that's been cut off before its final letter, e.g.
# "\x1b" or "\x1b[92;1" sitting right at the end of a chunk. Serial data
# arrives via arbitrary-sized reads, so a sequence can straddle two reads -
# without this, the trailing "\x1b" would be treated as literal text.
_INCOMPLETE_CSI_RE = re.compile(r"\x1b(\[[0-9;]*)?$")

_STANDARD_COLORS = {
    30: "black", 31: "red", 32: "green", 33: "yellow",
    34: "blue", 35: "magenta", 36: "cyan", 37: "white",
}
_BRIGHT_COLORS = {
    90: "gray", 91: "#ff6b6b", 92: "#69ff69", 93: "#ffe066",
    94: "#74b9ff", 95: "#e066ff", 96: "#66ffe0", 97: "white",
}


@dataclass(frozen=True)
class TextStyle:
    """The SGR state in effect for text that follows it."""

    color: str | None = None
    bold: bool = False

    def apply_sgr(self, params: list[int]) -> "TextStyle":
        """Return the style resulting from applying SGR parameter codes.

        Unsupported codes (underline, background color, ...) are ignored
        rather than raising - better to drop a style than to crash on a
        firmware boot message we don't fully parse.
        """
        style = self
        for code in params:
            if code == 0:
                style = TextStyle()
            elif code == 1:
                style = replace(style, bold=True)
            elif code == 22:
                style = replace(style, bold=False)
            elif code == 39:
                style = replace(style, color=None)
            elif code in _STANDARD_COLORS:
                style = replace(style, color=_STANDARD_COLORS[code])
            elif code in _BRIGHT_COLORS:
                style = replace(style, color=_BRIGHT_COLORS[code])
        return style


@dataclass(frozen=True)
class StyledSegment:
    text: str
    style: TextStyle


def split_ansi(
    text: str, start_style: TextStyle = TextStyle()
) -> tuple[list[StyledSegment], TextStyle, str]:
    """Split `text` into styled segments, carrying style across escape codes.

    Returns ``(segments, end_style, pending)``:

    - callers append each segment with its style, and pass `end_style`
      back in as `start_style` for the next chunk of serial data, since a
      color started in one chunk (e.g. the prompt) commonly continues
      into the next.
    - `pending` is a possible incomplete escape sequence left dangling at
      the end of `text` (see `_INCOMPLETE_CSI_RE`). Callers must prepend
      it to the *raw* text of the next chunk before calling `split_ansi`
      again, rather than treating it as a finished segment now.
    """
    segments: list[StyledSegment] = []
    style = start_style
    pos = 0
    for match in _CSI_RE.finditer(text):
        literal = text[pos:match.start()]
        if literal:
            segments.append(StyledSegment(literal, style))
        if match.group(2) == "m":
            params = [int(p) for p in match.group(1).split(";") if p] or [0]
            style = style.apply_sgr(params)
        pos = match.end()

    tail = text[pos:]
    incomplete = _INCOMPLETE_CSI_RE.search(tail)
    if incomplete:
        tail, pending = tail[:incomplete.start()], tail[incomplete.start():]
    else:
        pending = ""

    if tail:
        segments.append(StyledSegment(tail, style))
    return segments, style, pending


class AnsiStripper:
    """Strips ANSI codes from a live, chunked text stream.

    Wraps the same style/pending bookkeeping `AnsiLogView` uses, so
    anything else that needs plain text from the same raw stream (e.g. a
    capture controller parsing command responses) doesn't have to
    duplicate the chunk-boundary handling in `split_ansi`.
    """

    def __init__(self) -> None:
        self._style = TextStyle()
        self._pending = ""

    def feed(self, text: str) -> str:
        segments, self._style, self._pending = split_ansi(self._pending + text, self._style)
        return "".join(segment.text for segment in segments)
