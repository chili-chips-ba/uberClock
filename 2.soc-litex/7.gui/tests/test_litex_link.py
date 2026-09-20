import os
import sys
import time
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui.litex_link import (
    GatewareReloadWorker,
    LitexTermWorker,
    build_command,
    build_reload_gateware_command,
    find_newest_bin,
)

# `cat` echoes stdin back to stdout, which is enough to exercise the PTY
# plumbing (spawn, write, read-back, teardown) without needing litex_term
# or real hardware attached.
CAT = ["cat"]


def _drain_until(worker: LitexTermWorker, predicate, timeout: float = 2.0) -> str:
    """Poll `worker` until `predicate(accumulated_text)` is true or timeout."""
    deadline = time.monotonic() + timeout
    text = ""
    while time.monotonic() < deadline:
        text += "".join(worker.poll())
        if predicate(text):
            return text
        time.sleep(0.05)
    raise AssertionError(f"condition not met within {timeout}s; got: {text!r}")


class FindNewestBinTests(unittest.TestCase):
    def test_returns_none_when_no_bin_files(self):
        with mock.patch("glob.glob", return_value=[]):
            self.assertIsNone(find_newest_bin("/nonexistent"))

    def test_picks_most_recently_modified_file(self):
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            older = Path(tmp) / "old.bin"
            newer = Path(tmp) / "new.bin"
            older.write_bytes(b"old")
            newer.write_bytes(b"new")
            os.utime(older, (1_000_000, 1_000_000))
            os.utime(newer, (2_000_000, 2_000_000))

            self.assertEqual(find_newest_bin(tmp), str(newer))


class BuildCommandTests(unittest.TestCase):
    def test_raises_when_litex_term_missing(self):
        with mock.patch("uc_gui.litex_link.find_litex_term", return_value=None):
            with self.assertRaises(FileNotFoundError):
                build_command("/dev/ttyUSB0", "/path/kernel.bin")

    def test_builds_expected_argv(self):
        with mock.patch("uc_gui.litex_link.find_litex_term", return_value="/usr/bin/litex_term"):
            argv = build_command("/dev/ttyUSB0", "/path/kernel.bin")
        self.assertEqual(argv, ["/usr/bin/litex_term", "/dev/ttyUSB0", "--kernel=/path/kernel.bin"])


class LitexTermWorkerTests(unittest.TestCase):
    def test_not_running_before_start(self):
        worker = LitexTermWorker()
        self.assertFalse(worker.is_running)

    def test_send_line_before_start_raises(self):
        worker = LitexTermWorker()
        with self.assertRaises(RuntimeError):
            worker.send_line("hello")

    def test_start_send_roundtrip_and_stop(self):
        worker = LitexTermWorker()
        worker.start(CAT)
        try:
            self.assertTrue(worker.is_running)

            worker.send_line("hello uberclock")
            text = _drain_until(worker, lambda t: "hello uberclock" in t)
            self.assertIn("hello uberclock", text)
        finally:
            worker.stop()

        self.assertFalse(worker.is_running)

    def test_request_reboot_sends_reboot_line(self):
        worker = LitexTermWorker()
        worker.start(CAT)
        try:
            worker.request_reboot()
            text = _drain_until(worker, lambda t: "reboot" in t)
            self.assertIn("reboot", text)
        finally:
            worker.stop()

    def test_is_running_becomes_false_when_child_exits_on_its_own(self):
        worker = LitexTermWorker()
        worker.start(["sh", "-c", "exit 0"])
        try:
            deadline = time.monotonic() + 2.0
            while worker.is_running and time.monotonic() < deadline:
                time.sleep(0.05)
            self.assertFalse(worker.is_running)
        finally:
            worker.stop()

    def test_stop_is_safe_to_call_when_never_started(self):
        worker = LitexTermWorker()
        worker.stop()  # must not raise
        self.assertFalse(worker.is_running)

    def test_starting_again_stops_previous_process(self):
        worker = LitexTermWorker()
        worker.start(CAT)
        first_process = worker._process
        try:
            worker.start(CAT)
            self.assertNotEqual(worker._process.pid, first_process.pid)
            self.assertIsNotNone(first_process.poll(), "previous process should have been terminated")
        finally:
            worker.stop()


class BuildReloadGatewareCommandTests(unittest.TestCase):
    def _make_target_script(self, tmp_dir: str, with_build_dir: bool = True) -> Path:
        target = Path(tmp_dir) / "8.python" / "src" / "targets" / "alinx_ax7203_uberclock.py"
        target.parent.mkdir(parents=True)
        target.write_text("# stand-in target script\n")
        if with_build_dir:
            (Path(tmp_dir) / "3.build").mkdir()
        return Path(tmp_dir)

    def test_raises_when_litex_term_missing(self):
        with mock.patch("uc_gui.litex_link.find_litex_term", return_value=None):
            with self.assertRaises(FileNotFoundError):
                build_reload_gateware_command("/nonexistent")

    def test_raises_when_target_script_missing(self):
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            with mock.patch("uc_gui.litex_link.find_litex_term", return_value="/opt/litex/bin/litex_term"):
                with self.assertRaises(FileNotFoundError):
                    build_reload_gateware_command(tmp)

    def test_raises_when_build_directory_missing(self):
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            project_root = self._make_target_script(tmp, with_build_dir=False)
            with mock.patch("uc_gui.litex_link.find_litex_term", return_value="/opt/litex/bin/litex_term"):
                with self.assertRaises(FileNotFoundError):
                    build_reload_gateware_command(project_root)

    def test_builds_argv_env_and_cwd_matching_the_makefile_load_target(self):
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            project_root = self._make_target_script(tmp)
            with mock.patch("uc_gui.litex_link.find_litex_term", return_value="/opt/litex/bin/litex_term"):
                argv, env, cwd = build_reload_gateware_command(
                    project_root, sys_clk_freq="50e6", options=("--foo",)
                )

        self.assertEqual(argv[0], "/opt/litex/bin/python3")  # sibling of litex_term, same venv
        self.assertEqual(argv[1], str(project_root / "8.python" / "src" / "targets" / "alinx_ax7203_uberclock.py"))
        self.assertEqual(argv[2:], ["--load", "--sys-clk-freq=50e6", "--foo"])
        self.assertEqual(env["PYTHONPATH"], str(project_root / "8.python" / "src"))
        # Must match where `make load` actually runs from (3.build/), since
        # the target script resolves its build output relative to CWD.
        self.assertEqual(cwd, str(project_root / "3.build"))


class GatewareReloadWorkerTests(unittest.TestCase):
    def test_streams_output_and_reports_exit_code(self):
        worker = GatewareReloadWorker()
        worker.start(["python3", "-c", "print('programming...'); raise SystemExit(3)"])
        try:
            text = _drain_until(worker, lambda t: "exit code=3" in t)
            self.assertIn("programming...", text)
            self.assertEqual(worker.exit_code, 3)
            self.assertFalse(worker.is_running)
        finally:
            worker.stop()

    def test_runs_in_the_given_working_directory(self):
        # Regression test: the target script resolves its build output
        # relative to CWD, so a wrong `cwd` here is exactly what caused
        # "Open file FAIL" against a bitstream path under 7.gui/build/
        # instead of 3.build/build/ on real hardware.
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            worker = GatewareReloadWorker()
            worker.start(["python3", "-c", "import os; print(os.getcwd())"], cwd=tmp)
            try:
                text = _drain_until(worker, lambda t: "finished" in t)
                self.assertIn(str(Path(tmp).resolve()), text)
            finally:
                worker.stop()

    def test_start_raises_if_already_running(self):
        worker = GatewareReloadWorker()
        worker.start(["sleep", "1"])
        try:
            with self.assertRaises(RuntimeError):
                worker.start(["sleep", "1"])
        finally:
            worker.stop()

    def test_stop_is_safe_to_call_when_never_started(self):
        worker = GatewareReloadWorker()
        worker.stop()  # must not raise


if __name__ == "__main__":
    unittest.main()
