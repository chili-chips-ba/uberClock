import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication, QGroupBox

from uc_gui.cmd_catalog import Command, EnumChoice, FreeText, IntRange, NoArgs
from uc_gui.command_meta import Family
from uc_gui.command_panel import CommandPanel, family_representative, organize_commands
from uc_gui.widget_factory import CommandRow, FamilyRow

_app = QApplication.instance() or QApplication([])


def _all_gain_commands():
    return [Command(f"gain{n}", f"Set gain{n}", FreeText()) for n in range(1, 6)]


class OrganizeCommandsTests(unittest.TestCase):
    def test_gain_channels_collapse_into_one_family_entry(self):
        sections = organize_commands(_all_gain_commands())
        gain_items = sections["gain"]
        self.assertEqual(len(gain_items), 1)
        self.assertIsInstance(gain_items[0], Family)

    def test_family_not_shown_when_none_of_its_channels_are_present(self):
        sections = organize_commands([Command("ddrinfo", "info", NoArgs())])
        self.assertNotIn("gain", sections)

    def test_partial_family_still_shows_up(self):
        # Only gain1..gain3 advertised (e.g. an older firmware build) -
        # the family row should still appear, just able to reach fewer
        # channels than a full build would offer.
        commands = [Command(f"gain{n}", f"Set gain{n}", FreeText()) for n in (1, 2, 3)]
        sections = organize_commands(commands)
        self.assertIn("gain", sections)

    def test_unrecognized_command_lands_in_general(self):
        sections = organize_commands([Command("brand_new_thing", "help", NoArgs())])
        self.assertIn("general", sections)
        self.assertEqual(sections["general"][0].name, "brand_new_thing")

    def test_lowspeed_dbg_select_arg_spec_is_overridden_before_grouping(self):
        sections = organize_commands([Command("lowspeed_dbg_select", "help", IntRange(0, 7))])
        command = sections["capture"][0]
        self.assertIsInstance(command.args, EnumChoice)


class FamilyRepresentativeTests(unittest.TestCase):
    def test_picks_first_present_channel(self):
        family = Family(
            label="Gain", group="gain", help="", channels=(("Ch 1", "gain1"), ("Ch 2", "gain2"))
        )
        by_name = {"gain2": Command("gain2", "Set gain2", FreeText())}
        self.assertEqual(family_representative(family, by_name).name, "gain2")

    def test_raises_if_no_channel_present(self):
        family = Family(label="Gain", group="gain", help="", channels=(("Ch 1", "gain1"),))
        with self.assertRaises(KeyError):
            family_representative(family, {})


class CommandPanelTests(unittest.TestCase):
    def test_builds_without_error_for_mixed_commands(self):
        commands = [
            Command("cap_arm", "Pulse cap_arm", NoArgs()),
            Command("phase_nco", "Set phase", IntRange(0, 16777215)),
            Command("cap_enable", "mode", EnumChoice(((0, "ramp"), (1, "capture")))),
            Command("help_uc", "Show help", NoArgs()),
        ]
        panel = CommandPanel(commands, send_command=lambda _: None)
        self.assertIsNotNone(panel)

    def test_gain_channels_render_as_a_single_family_row_not_five_buttons(self):
        panel = CommandPanel(_all_gain_commands(), send_command=lambda _: None)
        family_rows = panel.findChildren(FamilyRow)
        command_rows = panel.findChildren(CommandRow)
        self.assertEqual(len(family_rows), 1)
        self.assertEqual([r.command.name for r in command_rows if r.command.name.startswith("gain")], [])

    def test_groups_start_expanded_or_collapsed_per_metadata(self):
        commands = [
            Command("phase_nco", "Set phase", IntRange(0, 100)),  # group "input", expanded
            Command("ddrinfo", "info", NoArgs()),  # group "ddr", collapsed
        ]
        panel = CommandPanel(commands, send_command=lambda _: None)
        boxes = {box.title(): box for box in panel.findChildren(QGroupBox)}
        self.assertTrue(boxes["Input && NCO"].isChecked())
        self.assertFalse(boxes["DDR Diagnostics"].isChecked())


if __name__ == "__main__":
    unittest.main()
