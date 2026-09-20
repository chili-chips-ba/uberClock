import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui.cmd_catalog import Command, EnumChoice, IntRange
from uc_gui.command_meta import (
    FAMILIES,
    GROUP_EXPANDED,
    GROUP_ORDER,
    GROUP_TITLES,
    apply_enum_overrides,
    family_commands,
    group_for,
)


class GroupForTests(unittest.TestCase):
    def test_known_command_maps_to_its_curated_group(self):
        self.assertEqual(group_for("ddrinfo"), "ddr")
        self.assertEqual(group_for("sig3_start"), "siggen")

    def test_unknown_command_falls_back_to_general(self):
        # A firmware command added after this file was last updated
        # shouldn't crash the panel - it should just land in General.
        self.assertEqual(group_for("some_brand_new_command"), "general")

    def test_every_group_key_has_a_title_and_expanded_flag(self):
        for key in GROUP_ORDER:
            self.assertIn(key, GROUP_TITLES)
            self.assertIn(key, GROUP_EXPANDED)


class ApplyEnumOverridesTests(unittest.TestCase):
    def test_lowspeed_dbg_select_gets_friendly_labels(self):
        commands = [Command("lowspeed_dbg_select", "help", IntRange(0, 7))]
        overridden = apply_enum_overrides(commands)
        self.assertIsInstance(overridden[0].args, EnumChoice)
        labels = dict(overridden[0].args.options)
        self.assertEqual(labels[6], "Ref Y (downsampled)")

    def test_unrelated_command_is_untouched(self):
        original = Command("phase_nco", "help", IntRange(0, 100))
        overridden = apply_enum_overrides([original])
        self.assertEqual(overridden, [original])


class FamiliesTests(unittest.TestCase):
    def test_gain_family_covers_channels_1_through_5(self):
        gain = next(f for f in FAMILIES if f.label == "Gain")
        names = [name for _, name in gain.channels]
        self.assertEqual(names, ["gain1", "gain2", "gain3", "gain4", "gain5"])

    def test_downconversion_family_includes_the_ref_channel(self):
        downconvert = next(f for f in FAMILIES if f.label == "Downconversion phase")
        names = [name for _, name in downconvert.channels]
        self.assertIn("phase_down_ref", names)

    def test_family_commands_matches_every_channel_across_all_families(self):
        expected = {name for family in FAMILIES for _, name in family.channels}
        self.assertEqual(family_commands(), expected)
        self.assertIn("gain3", family_commands())
        self.assertNotIn("cap_arm", family_commands())


if __name__ == "__main__":
    unittest.main()
