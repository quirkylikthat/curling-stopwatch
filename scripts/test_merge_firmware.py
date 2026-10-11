"""Tests for merge_firmware.py. Run: python3 -m unittest discover -s scripts"""

import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.dirname(__file__))
import merge_firmware as mf  # noqa: E402


def entry(ptype, subtype, offset, size, label):
    return struct.pack("<2sBBII16sI", b"\xaa\x50", ptype, subtype, offset, size, label.encode(), 0)


def table(*entries):
    # Real tables end with an MD5 entry (0xEBEB) and then erased flash.
    return b"".join(entries) + b"\xeb\xeb" + b"\xff" * 30 + b"\xff" * 64


# Same shape as the project's app3M_fat9M_16MB.csv.
TABLE = table(
    entry(1, 2, 0x9000, 0x5000, "nvs"),
    entry(1, 0, 0xE000, 0x2000, "otadata"),
    entry(0, 0x10, 0x10000, 0x300000, "app0"),
    entry(1, 0x81, 0x310000, 0x900000, "ffat"),
)


class ParseTests(unittest.TestCase):
    def test_parses_entries_and_stops_at_md5_marker(self):
        entries = mf.parse_partition_table(TABLE)
        self.assertEqual([e[4] for e in entries], ["nvs", "otadata", "app0", "ffat"])

    def test_layout_uses_the_table_not_constants(self):
        self.assertEqual(mf.find_layout(mf.parse_partition_table(TABLE)), (0x10000, 0x300000, 0xE000))

    def test_missing_app_partition_is_an_error(self):
        t = table(entry(1, 2, 0x9000, 0x5000, "nvs"))
        with self.assertRaises(mf.MergeError):
            mf.find_layout(mf.parse_partition_table(t))


class MergeTests(unittest.TestCase):
    def test_pieces_land_at_their_offsets_and_gaps_are_erased(self):
        image, _ = mf.build_image(b"BOOT", TABLE, b"OTA0", b"APP")
        self.assertEqual(image[0:4], b"BOOT")
        self.assertEqual(image[0x8000 : 0x8000 + len(TABLE)], TABLE)
        self.assertEqual(image[0xE000:0xE004], b"OTA0")
        self.assertEqual(image[0x10000:0x10003], b"APP")
        self.assertEqual(len(image), 0x10003)
        self.assertEqual(image[4], 0xFF)  # gap after the bootloader
        self.assertEqual(image[0x9000], 0xFF)  # nvs area left erased

    def test_overlap_is_rejected(self):
        with self.assertRaises(mf.MergeError):
            mf.merge([(0, "a", b"x" * 20), (10, "b", b"y")])

    def test_oversized_app_is_rejected(self):
        with self.assertRaises(mf.MergeError):
            mf.build_image(b"BOOT", TABLE, b"OTA0", b"x" * (0x300000 + 1))

    def test_bootloader_too_big_for_its_gap_is_rejected(self):
        with self.assertRaises(mf.MergeError):
            mf.build_image(b"x" * 0x8001, TABLE, b"OTA0", b"APP")


if __name__ == "__main__":
    unittest.main()
