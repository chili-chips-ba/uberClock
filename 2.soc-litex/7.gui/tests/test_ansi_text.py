import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui.ansi_text import AnsiStripper, StyledSegment, TextStyle, split_ansi


class TextStyleApplySgrTests(unittest.TestCase):
    def test_reset_clears_color_and_bold(self):
        style = TextStyle(color="green", bold=True).apply_sgr([0])
        self.assertEqual(style, TextStyle())

    def test_bold_and_bright_green_combine(self):
        style = TextStyle().apply_sgr([92, 1])
        self.assertEqual(style, TextStyle(color="#69ff69", bold=True))

    def test_unsupported_code_is_ignored_not_raised(self):
        style = TextStyle().apply_sgr([4])  # underline, unsupported
        self.assertEqual(style, TextStyle())


class SplitAnsiTests(unittest.TestCase):
    def test_plain_text_is_one_unstyled_segment(self):
        segments, end_style, pending = split_ansi("help")
        self.assertEqual(segments, [StyledSegment("help", TextStyle())])
        self.assertEqual(end_style, TextStyle())
        self.assertEqual(pending, "")

    def test_real_boot_prompt_from_the_field(self):
        # Exact bytes reported from a live litex_term session.
        text = "\x1b[92;1mlitex\x1b[0m> "
        segments, end_style, pending = split_ansi(text)
        self.assertEqual(
            segments,
            [
                StyledSegment("litex", TextStyle(color="#69ff69", bold=True)),
                StyledSegment("> ", TextStyle()),
            ],
        )
        self.assertEqual(end_style, TextStyle())
        self.assertEqual(pending, "")

    def test_doubled_prompt_from_the_field(self):
        # Exact bytes reported from a live uberClock console session:
        # the prompt is wrapped in a *nested* color code.
        text = "\x1b[92;1m\x1b[92;1muberClock\x1b[0m>\x1b[0m > cap_start\n"
        segments, end_style, pending = split_ansi(text)
        self.assertEqual(
            segments,
            [
                StyledSegment("uberClock", TextStyle(color="#69ff69", bold=True)),
                StyledSegment(">", TextStyle()),
                StyledSegment(" > cap_start\n", TextStyle()),
            ],
        )
        self.assertEqual(end_style, TextStyle())
        self.assertEqual(pending, "")

    def test_style_carries_across_chunks(self):
        first_segments, mid_style, pending = split_ansi("\x1b[31mred start")
        self.assertEqual(mid_style, TextStyle(color="red"))
        self.assertEqual(pending, "")

        second_segments, end_style, _ = split_ansi(" red continues\x1b[0m plain", mid_style)

        self.assertEqual(first_segments, [StyledSegment("red start", TextStyle(color="red"))])
        self.assertEqual(
            second_segments,
            [
                StyledSegment(" red continues", TextStyle(color="red")),
                StyledSegment(" plain", TextStyle()),
            ],
        )
        self.assertEqual(end_style, TextStyle())

    def test_non_sgr_csi_sequence_is_dropped_without_crashing(self):
        # "\x1b[K" is erase-line, not a color code - shouldn't leak or raise.
        segments, _, _ = split_ansi("before\x1b[Kafter")
        self.assertEqual(
            segments,
            [
                StyledSegment("before", TextStyle()),
                StyledSegment("after", TextStyle()),
            ],
        )

    def test_escape_sequence_split_across_chunks_is_held_back(self):
        # Regression test: real serial data arrives via arbitrary-sized
        # reads, so a chunk can end mid-escape-sequence. Reported live as
        # a raw ESC byte leaking into the log as literal text.
        segments, style, pending = split_ansi("before\x1b[92;1")
        self.assertEqual(segments, [StyledSegment("before", TextStyle())])
        self.assertEqual(style, TextStyle())  # not applied yet - sequence isn't finished
        self.assertEqual(pending, "\x1b[92;1")

        # The caller prepends `pending` to the next chunk's raw text.
        segments2, style2, pending2 = split_ansi(pending + "muberClock", style)
        self.assertEqual(segments2, [StyledSegment("uberClock", TextStyle(color="#69ff69", bold=True))])
        self.assertEqual(pending2, "")

    def test_bare_trailing_escape_byte_is_held_back(self):
        segments, _, pending = split_ansi("hello\x1b")
        self.assertEqual(segments, [StyledSegment("hello", TextStyle())])
        self.assertEqual(pending, "\x1b")


class AnsiStripperTests(unittest.TestCase):
    def test_strips_color_codes_across_chunks(self):
        stripper = AnsiStripper()
        out = stripper.feed("\x1b[92;1muberClock\x1b[0m> cap_start\n")
        self.assertEqual(out, "uberClock> cap_start\n")

    def test_holds_back_split_escape_sequence(self):
        stripper = AnsiStripper()
        out1 = stripper.feed("before \x1b[92;1")
        out2 = stripper.feed("muberClock\x1b[0m> ")
        self.assertEqual(out1 + out2, "before uberClock> ")
        self.assertNotIn("\x1b", out1 + out2)


if __name__ == "__main__":
    unittest.main()
