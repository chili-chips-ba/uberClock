import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui.cmd_catalog import (
    Command,
    CmdListParseError,
    EnumChoice,
    FreeText,
    IntRange,
    NoArgs,
    parse_arg_spec,
    parse_cmdlist,
)


class ParseArgSpecTests(unittest.TestCase):
    def test_none_means_no_args(self):
        self.assertEqual(parse_arg_spec("none"), NoArgs())

    def test_int_range(self):
        self.assertEqual(parse_arg_spec("int:0:7"), IntRange(low=0, high=7))

    def test_int_range_with_negative_bounds(self):
        self.assertEqual(parse_arg_spec("int:-2048:2047"), IntRange(low=-2048, high=2047))

    def test_enum_choice(self):
        spec = parse_arg_spec("enum:0=ramp,1=capture")
        self.assertEqual(spec, EnumChoice(options=((0, "ramp"), (1, "capture"))))

    def test_empty_string_is_free_text(self):
        # Firmware prints "" (not "NULL") for a NULL arg_spec.
        self.assertEqual(parse_arg_spec(""), FreeText())

    def test_unrecognized_string_is_free_text(self):
        self.assertEqual(parse_arg_spec("whatever"), FreeText())


class ParseCmdlistTests(unittest.TestCase):
    def test_parses_all_command_kinds(self):
        output = (
            "boot banner noise\n"
            "--CMDLIST-BEGIN--\n"
            "cmdlist\tDump all commands\tnone\n"
            "phase_nco\tSet NCO phase increment\tint:0:16777215\n"
            "cap_enable\t0=ramp, 1=capture\tenum:0=ramp,1=capture\n"
            "cap_beats\tSet capture length in beats\t\n"
            "--CMDLIST-END--\n"
            "uc> "
        )
        commands = parse_cmdlist(output)
        self.assertEqual(
            commands,
            [
                Command("cmdlist", "Dump all commands", NoArgs()),
                Command("phase_nco", "Set NCO phase increment", IntRange(0, 16777215)),
                Command(
                    "cap_enable",
                    "0=ramp, 1=capture",
                    EnumChoice(((0, "ramp"), (1, "capture"))),
                ),
                Command("cap_beats", "Set capture length in beats", FreeText()),
            ],
        )

    def test_skips_malformed_lines(self):
        output = (
            "--CMDLIST-BEGIN--\n"
            "this line has no tabs at all\n"
            "help\tShow help\tnone\n"
            "--CMDLIST-END--\n"
        )
        commands = parse_cmdlist(output)
        self.assertEqual(commands, [Command("help", "Show help", NoArgs())])

    def test_ignores_text_outside_markers(self):
        output = (
            "garbage\tignored\tnone\n"
            "--CMDLIST-BEGIN--\n"
            "help\tShow help\tnone\n"
            "--CMDLIST-END--\n"
            "more garbage\tignored\tnone\n"
        )
        commands = parse_cmdlist(output)
        self.assertEqual(commands, [Command("help", "Show help", NoArgs())])

    def test_raises_when_markers_missing(self):
        with self.assertRaises(CmdListParseError):
            parse_cmdlist("uc> help\nunknown command\n")


if __name__ == "__main__":
    unittest.main()
