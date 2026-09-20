import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

# Widget construction needs a QApplication, but not a visible display or an
# event loop - the "offscreen" platform plugin makes this work in CI/headless
# environments. Must be set before importing anything from PySide6.QtWidgets.
import os

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtWidgets import QApplication, QComboBox, QLineEdit, QSpinBox

from uc_gui.cmd_catalog import Command, EnumChoice, FreeText, IntRange, NoArgs
from uc_gui.command_meta import Family
from uc_gui.command_panel import CommandPanel
from uc_gui.widget_factory import CommandRow, FamilyRow, make_value_widget, value_widget_text

_app = QApplication.instance() or QApplication([])


class MakeValueWidgetTests(unittest.TestCase):
    def test_no_args_has_no_widget(self):
        command = Command("cap_arm", "Pulse cap_arm", NoArgs())
        self.assertIsNone(make_value_widget(command))

    def test_int_range_makes_spinbox_with_matching_range(self):
        command = Command("phase_nco", "Set phase", IntRange(0, 16777215))
        widget = make_value_widget(command)
        self.assertIsInstance(widget, QSpinBox)
        self.assertEqual((widget.minimum(), widget.maximum()), (0, 16777215))

    def test_enum_choice_makes_combobox_with_labels(self):
        command = Command("cap_enable", "mode", EnumChoice(((0, "ramp"), (1, "capture"))))
        widget = make_value_widget(command)
        self.assertIsInstance(widget, QComboBox)
        self.assertEqual(widget.count(), 2)
        self.assertEqual(widget.itemText(0), "ramp (0)")
        self.assertEqual(widget.itemData(1), 1)

    def test_free_text_makes_line_edit(self):
        command = Command("cap_beats", "beats", FreeText())
        self.assertIsInstance(make_value_widget(command), QLineEdit)

    def test_default_prefills_spinbox_when_in_range(self):
        command = Command("final_shift", "Set final shift", IntRange(0, 7))
        widget = make_value_widget(command, default="0x3")
        self.assertEqual(widget.value(), 3)

    def test_out_of_range_default_is_ignored_not_raised(self):
        command = Command("phase_nco", "Set phase", IntRange(0, 100))
        widget = make_value_widget(command, default="99999")
        self.assertEqual(widget.value(), 0)  # falls back to the spinbox's own default

    def test_unparseable_default_is_ignored_not_raised(self):
        command = Command("phase_nco", "Set phase", IntRange(0, 100))
        widget = make_value_widget(command, default="not-a-number")
        self.assertEqual(widget.value(), 0)

    def test_default_selects_matching_enum_item(self):
        command = Command("cap_enable", "mode", EnumChoice(((0, "ramp"), (1, "capture"))))
        widget = make_value_widget(command, default="1")
        self.assertEqual(widget.currentIndex(), 1)

    def test_default_prefills_line_edit(self):
        command = Command("cap_beats", "beats", FreeText())
        widget = make_value_widget(command, default="4096")
        self.assertEqual(widget.text(), "4096")


class ValueWidgetTextTests(unittest.TestCase):
    def test_reads_spinbox_value(self):
        box = QSpinBox()
        box.setRange(0, 100)
        box.setValue(42)
        self.assertEqual(value_widget_text(box), "42")

    def test_reads_combobox_selected_data(self):
        box = QComboBox()
        box.addItem("ramp (0)", userData=0)
        box.addItem("capture (1)", userData=1)
        box.setCurrentIndex(1)
        self.assertEqual(value_widget_text(box), "1")

    def test_reads_and_strips_line_edit_text(self):
        edit = QLineEdit("  0xA0000000  ")
        self.assertEqual(value_widget_text(edit), "0xA0000000")


class CommandRowTests(unittest.TestCase):
    def test_clicking_send_formats_command_with_value(self):
        sent = []
        command = Command("phase_nco", "Set phase", IntRange(0, 16777215))
        row = CommandRow(command, send_command=sent.append)
        row._value_widget.setValue(2_581_836)

        row._on_send_clicked()

        self.assertEqual(sent, ["phase_nco 2581836"])

    def test_clicking_send_with_no_args_sends_bare_name(self):
        sent = []
        command = Command("cap_arm", "Pulse cap_arm", NoArgs())
        row = CommandRow(command, send_command=sent.append)

        row._on_send_clicked()

        self.assertEqual(sent, ["cap_arm"])


class FamilyRowTests(unittest.TestCase):
    def _family(self):
        return Family(
            label="Gain",
            group="gain",
            help="Fixed-point gain coefficient.",
            channels=(("Channel 1", "gain1"), ("Channel 2", "gain2")),
            default="0x40000000",
        )

    def test_sends_selected_channels_command_with_value(self):
        # Real gain1..5 commands advertise FreeText (arg_spec NULL) since
        # the firmware takes a raw 32-bit hex register value.
        sent = []
        representative = Command("gain1", "Set gain1", FreeText())
        row = FamilyRow(self._family(), representative, send_command=sent.append)

        row.channel_box.setCurrentIndex(1)  # "Channel 2" -> gain2
        row._on_send_clicked()

        self.assertEqual(sent, ["gain2 0x40000000"])  # family default carried over

    def test_default_from_family_prefills_the_value_widget(self):
        representative = Command("gain1", "Set gain1", FreeText())
        row = FamilyRow(self._family(), representative, send_command=lambda _: None)
        self.assertEqual(row._value_widget.text(), "0x40000000")


class CommandPanelTests(unittest.TestCase):
    def test_builds_without_error_for_mixed_commands(self):
        commands = [
            Command("cap_arm", "Pulse cap_arm", NoArgs()),
            Command("phase_nco", "Set phase", IntRange(0, 16777215)),
            Command("cap_enable", "mode", EnumChoice(((0, "ramp"), (1, "capture")))),
            Command("help", "Show help", NoArgs()),
        ]
        panel = CommandPanel(commands, send_command=lambda _: None)
        self.assertIsNotNone(panel)


if __name__ == "__main__":
    unittest.main()
