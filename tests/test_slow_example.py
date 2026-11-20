#!/usr/bin/env python3
"""Tests of tools/slow_example.py and examples/jump_slow.dingle.

Run: python3 tests/test_slow_example.py
The program's own loader reads jump_slow.dingle in tests/tst_roundtrip.cpp.
"""

import json
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))

import slow_example  # noqa: E402

SOURCE = os.path.join(ROOT, "examples", "jump.dingle")
SLOW = os.path.join(ROOT, "examples", "jump_slow.dingle")
SCRIPT = os.path.join(ROOT, "tools", "slow_example.py")


def load(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def all_keys(data, kind):
    for track in data["tracks"]:
        for key in track.get(kind, []):
            yield track["joint"], key


class SlowExampleTest(unittest.TestCase):

    def setUp(self):
        self.src = load(SOURCE)
        self.slow = load(SLOW)

    def test_committed_file_is_the_script_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "jump_slow.dingle")
            subprocess.run([sys.executable, SCRIPT, SOURCE, out], check=True)
            with open(out, encoding="utf-8") as a, open(SLOW, encoding="utf-8") as b:
                self.assertEqual(a.read(), b.read())

    def test_frame_count_and_loop_scale(self):
        self.assertEqual(self.src["timing"]["frames"], 30)
        self.assertEqual(self.slow["timing"]["frames"], 75)
        self.assertEqual(self.slow["loop"], {"in": 23, "out": 48})
        self.assertEqual(self.slow["timing"]["frameTime"], self.src["timing"]["frameTime"])

    def test_key_frames_scale_and_values_stay(self):
        for kind in ("rotation", "position"):
            src_keys = list(all_keys(self.src, kind))
            slow_keys = list(all_keys(self.slow, kind))
            self.assertEqual(len(src_keys), len(slow_keys))
            for (j1, a), (j2, b) in zip(src_keys, slow_keys):
                self.assertEqual(j1, j2)
                self.assertEqual(b["frame"], slow_example.scale_frame(a["frame"], 2.5))
                self.assertEqual(a["values"], b["values"])
                self.assertEqual(a["easeIn"], b["easeIn"])
                self.assertEqual(a["easeOut"], b["easeOut"])

    def test_frame_zero_is_unchanged(self):
        for kind in ("rotation", "position"):
            for (j, key) in all_keys(self.slow, kind):
                if key["frame"] == 0:
                    src = [k for (jj, k) in all_keys(self.src, kind) if jj == j and k["frame"] == 0]
                    self.assertEqual(src[0]["values"], key["values"])

    def test_keys_sorted_unique_and_inside_frames(self):
        frames = self.slow["timing"]["frames"]
        for track in self.slow["tracks"]:
            for kind in ("rotation", "position"):
                keys = track.get(kind, [])
                numbers = [k["frame"] for k in keys]
                self.assertEqual(numbers, sorted(set(numbers)))
                if numbers:
                    self.assertEqual(numbers[0], 0)
                    self.assertLess(numbers[-1], frames)

    def test_same_skeleton_and_settings(self):
        for name in ("format", "version", "figure", "scale", "skeleton"):
            self.assertEqual(self.src[name], self.slow[name])

    def test_rounding_is_half_up(self):
        self.assertEqual(slow_example.scale_frame(0, 2.5), 0)
        self.assertEqual(slow_example.scale_frame(1, 2.5), 3)
        self.assertEqual(slow_example.scale_frame(9, 2.5), 23)
        self.assertEqual(slow_example.scale_frame(19, 2.5), 48)

    def test_factor_below_one_is_refused(self):
        with self.assertRaises(ValueError):
            slow_example.slow_down(self.src, 0.5)


if __name__ == "__main__":
    unittest.main()
