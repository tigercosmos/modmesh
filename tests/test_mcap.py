# Copyright (c) 2026, solvcon team <contact@solvcon.net>
# BSD 3-Clause License, see COPYING

import os
import struct
import tempfile

import unittest

import solvcon as sc

# tests/data/make_mcap_fixtures.py writes the fixtures. A chunked fixture
# takes its name from its chunk compression.
CHUNKED = ("vehicle_none", "vehicle_lz4", "vehicle_zstd")
UNCHUNKED = "vehicle_unchunked"
FIXTURES = CHUNKED + (UNCHUNKED,)

STATUS = "/vehicle/status"
IMU = "/vehicle/imu"
TOPICS = (STATUS, IMU)

# What the generator recorded: status messages this far apart in nanoseconds,
# starting here, and one imu message per twelve of them.
START_TIME = 1700000000000000000
PERIOD = 10000000
MESSAGE_COUNT = 96
END_TIME = START_TIME + (MESSAGE_COUNT - 1) * PERIOD
# The 256-byte chunk size of the generator packs the messages into this many
# chunks. Regenerating the fixtures with another chunk size changes it.
CHUNK_COUNT = 19


@unittest.skipUnless(sc.mcap.HAS_MCAP, "built without BUILD_MCAP")
class McapReaderTB(unittest.TestCase):
    TESTDIR = os.path.abspath(os.path.dirname(__file__))
    DATADIR = os.path.join(TESTDIR, "data")

    def path(self, name):
        return os.path.join(self.DATADIR, "%s.mcap" % name)


class McapSummaryTC(McapReaderTB):
    def test_topics(self):
        for name in FIXTURES:
            with self.subTest(name=name):
                reader = sc.mcap.Reader(self.path(name))
                self.assertEqual(reader.topics(),
                                 {STATUS: "vhcl_msgs/msg/Status",
                                  IMU: "vhcl_msgs/msg/Imu"})

    def test_time_range(self):
        for name in FIXTURES:
            with self.subTest(name=name):
                reader = sc.mcap.Reader(self.path(name))
                self.assertTrue(reader.has_time_range())
                self.assertEqual(reader.time_range(),
                                 (START_TIME, END_TIME))

    def test_path(self):
        path = self.path("vehicle_zstd")
        self.assertEqual(sc.mcap.Reader(path).path, path)

    def test_not_an_mcap_file(self):
        path = os.path.join(self.DATADIR, "rectangle.msh")
        with self.assertRaisesRegex(RuntimeError, "not an MCAP file"):
            sc.mcap.Reader(path)

    def test_missing_file(self):
        with self.assertRaisesRegex(RuntimeError, "cannot open"):
            sc.mcap.Reader(os.path.join(self.DATADIR, "no_such.mcap"))

    def test_chunk_count(self):
        for name in CHUNKED:
            with self.subTest(name=name):
                reader = sc.mcap.Reader(self.path(name))
                self.assertEqual(reader.chunk_count(), CHUNK_COUNT)

    def test_unchunked_file_has_no_chunk_index(self):
        reader = sc.mcap.Reader(self.path(UNCHUNKED))
        self.assertEqual(reader.chunk_count(), 0)

    def test_summary_offset_past_the_footer(self):
        """A corrupt offset must not size a read from an underflow."""
        # The footer holds summary_start as its first uint64, and the tail is
        # the footer record plus the closing magic.
        tail = 9 + 20 + 8
        with open(self.path("vehicle_none"), "rb") as stream:
            data = bytearray(stream.read())
        for summary_start in (len(data) - tail, len(data), 2 ** 63):
            with self.subTest(summary_start=summary_start):
                struct.pack_into("<Q", data, len(data) - tail + 9,
                                 summary_start)
                with tempfile.TemporaryDirectory() as tmp:
                    path = os.path.join(tmp, "corrupt.mcap")
                    with open(path, "wb") as stream:
                        stream.write(bytes(data))
                    with self.assertRaisesRegex(RuntimeError, "MCAP"):
                        sc.mcap.Reader(path)


# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
