#!/usr/bin/env python3
"""Tests for tools/blender/bvh_compare.py (the Blender part is not run in CI).
Run: python3 tests/test_bvh_compare.py"""

import os
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TOOL = os.path.join(ROOT, "tools", "blender")
sys.path.insert(0, TOOL)
import bvh_compare  # noqa: E402

WALK = os.path.join(ROOT, "tests", "motions", "cmu_02_01_walk.bvh")
WAVE = os.path.join(ROOT, "tests", "motions", "mixamo_style_wave.bvh")


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def run_tool(*args):
    return subprocess.run([sys.executable, "-I", os.path.join(TOOL, "bvh_compare.py"), *args],
                          capture_output=True, text=True)


class Compare(unittest.TestCase):
    def test_identical_files(self):
        a = bvh_compare.load(WALK)
        self.assertEqual(bvh_compare.compare(a, bvh_compare.load(WALK)), [])
        self.assertEqual(run_tool(WALK, WALK).returncode, 0)

    def test_different_skeleton(self):
        diffs = bvh_compare.compare(bvh_compare.load(WALK), bvh_compare.load(WAVE))
        self.assertTrue(any("missing" in d for d in diffs))
        self.assertEqual(run_tool(WALK, WAVE).returncode, 1)

    def changed(self, edit):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "changed.bvh")
            with open(path, "w", encoding="utf-8") as f:
                f.write(edit(read(WALK)))
            return bvh_compare.compare(bvh_compare.load(WALK), bvh_compare.load(path)), run_tool(WALK, path)

    def test_one_value_changed(self):
        def edit(text):
            head, motion = text.split("Frame Time:", 1)
            lines = motion.split("\n")
            nums = lines[1].split()
            nums[4] = str(float(nums[4]) + 5.0)  # a rotation value in the first frame
            lines[1] = " ".join(nums)
            return head + "Frame Time:" + "\n".join(lines)
        diffs, proc = self.changed(edit)
        self.assertTrue(any(d.startswith("frame 0,") for d in diffs), diffs)
        self.assertEqual(proc.returncode, 1)

    def test_small_change_is_inside_tolerance(self):
        def edit(text):
            head, motion = text.split("Frame Time:", 1)
            lines = motion.split("\n")
            nums = lines[1].split()
            nums[4] = str(float(nums[4]) + 0.0001)
            lines[1] = " ".join(nums)
            return head + "Frame Time:" + "\n".join(lines)
        diffs, proc = self.changed(edit)
        self.assertEqual(diffs, [])
        self.assertEqual(proc.returncode, 0)

    def test_angle_wrap_is_equal(self):
        self.assertEqual(bvh_compare.angle_diff(180.0, -180.0), 0.0)
        self.assertAlmostEqual(bvh_compare.angle_diff(359.0, 1.0), 2.0)

    def test_frame_time_changed(self):
        diffs, proc = self.changed(lambda t: re.sub(r"(Frame Time:\s*)[0-9.]+", r"\g<1>0.041667", t))
        self.assertTrue(any("frame time" in d for d in diffs), diffs)
        self.assertEqual(proc.returncode, 1)

    def test_frame_removed(self):
        def edit(text):
            lines = text.rstrip("\n").split("\n")[:-1]  # drop the last frame line
            frames = bvh_compare.load(WALK).frames - 1
            return re.sub(r"(Frames:\s*)\d+", lambda m: m.group(1) + str(frames), "\n".join(lines) + "\n")
        diffs, proc = self.changed(edit)
        self.assertTrue(any("frame count" in d for d in diffs), diffs)
        self.assertEqual(proc.returncode, 1)

    def test_unreadable_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "bad.bvh")
            with open(path, "w") as f:
                f.write("not a bvh file\n")
            self.assertEqual(run_tool(WALK, path).returncode, 2)
            self.assertEqual(run_tool(WALK, os.path.join(tmp, "missing.bvh")).returncode, 2)


if __name__ == "__main__":
    unittest.main()
