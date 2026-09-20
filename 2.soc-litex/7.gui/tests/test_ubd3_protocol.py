import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from uc_gui.ubd3_protocol import HEADER_SIZE, MAGIC, PacketHeader, Reassembler, parse_header


def _packet(seq: int, offset: int, total: int, payload: bytes) -> bytes:
    return struct.pack("<IIII", MAGIC, seq, offset, total) + payload


class ParseHeaderTests(unittest.TestCase):
    def test_parses_a_valid_header(self):
        packet = _packet(seq=3, offset=64, total=256, payload=b"x" * 32)
        self.assertEqual(parse_header(packet), PacketHeader(seq=3, offset=64, total=256))

    def test_wrong_magic_returns_none(self):
        bad = struct.pack("<IIII", 0xDEADBEEF, 0, 0, 16) + b"x" * 16
        self.assertIsNone(parse_header(bad))

    def test_too_short_returns_none(self):
        self.assertIsNone(parse_header(b"short"))


class ReassemblerTests(unittest.TestCase):
    def test_single_packet_completes_transfer(self):
        r = Reassembler()
        r.feed_packet(_packet(0, 0, 8, b"ABCDEFGH"))
        self.assertTrue(r.is_complete)
        self.assertEqual(r.buffer, b"ABCDEFGH")
        self.assertEqual(r.missing_bytes, 0)

    def test_multiple_packets_in_order(self):
        r = Reassembler()
        r.feed_packet(_packet(0, 0, 12, b"ABCD"))
        r.feed_packet(_packet(1, 4, 12, b"EFGH"))
        r.feed_packet(_packet(2, 8, 12, b"IJKL"))
        self.assertTrue(r.is_complete)
        self.assertEqual(r.buffer, b"ABCDEFGHIJKL")

    def test_out_of_order_packets_still_assemble_correctly(self):
        # The firmware doesn't guarantee ordering under packet loss/retry
        # scenarios on the wire - offset-based placement must not care.
        r = Reassembler()
        r.feed_packet(_packet(2, 8, 12, b"IJKL"))
        r.feed_packet(_packet(0, 0, 12, b"ABCD"))
        r.feed_packet(_packet(1, 4, 12, b"EFGH"))
        self.assertTrue(r.is_complete)
        self.assertEqual(r.buffer, b"ABCDEFGHIJKL")

    def test_missing_middle_packet_is_reported_incomplete(self):
        r = Reassembler()
        r.feed_packet(_packet(0, 0, 12, b"ABCD"))
        r.feed_packet(_packet(2, 8, 12, b"IJKL"))  # packet 1 (offset 4) lost
        self.assertFalse(r.is_complete)
        self.assertEqual(r.missing_bytes, 4)

    def test_stray_non_ubd3_packet_is_ignored(self):
        r = Reassembler()
        r.feed_packet(_packet(0, 0, 4, b"ABCD"))
        r.feed_packet(b"not a ubd3 packet at all, just noise on the port")
        self.assertTrue(r.is_complete)
        self.assertEqual(r.buffer, b"ABCD")

    def test_duplicate_packet_is_harmless(self):
        r = Reassembler()
        packet = _packet(0, 0, 4, b"ABCD")
        r.feed_packet(packet)
        r.feed_packet(packet)
        self.assertTrue(r.is_complete)
        self.assertEqual(r.buffer, b"ABCD")

    def test_no_packets_yet_is_not_complete_and_has_no_total(self):
        r = Reassembler()
        self.assertFalse(r.is_complete)
        self.assertIsNone(r.total)
        self.assertEqual(r.missing_bytes, 0)


if __name__ == "__main__":
    unittest.main()
