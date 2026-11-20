#!/usr/bin/env python3
"""Tests of tools/wave_example.py and examples/wave.dingle (issue #53 step 6).

Run: python3 tests/test_wave_example.py
The program's own loader reads wave.dingle in tests/tst_roundtrip.cpp.
"""

import json
import os
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))

import wave_example  # noqa: E402

RELAXED = os.path.join(ROOT, "bin", "data", "Relaxed.dingle")
WAVE = os.path.join(ROOT, "examples", "wave.dingle")
SCRIPT = os.path.join(ROOT, "tools", "wave_example.py")
TEACHING = os.path.join(ROOT, "docs", "TEACHING.md")


def load(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def preset(start, target):
    """The choice a segment plays, read as Easing::segmentPreset does."""
    curve = start.get("curve")
    if curve:
        return {"ease-out-overshoot": "Overshoot, then settle", "constant": "Hold, then snap",
                "linear": "Even", "ease-in": "Slow start", "ease-out": "Slow finish",
                "ease-in-out": "Slow both ends"}[curve["type"]]
    return {(False, False): "Even", (True, False): "Slow start",
            (False, True): "Slow finish", (True, True): "Slow both ends"}[
                (start["easeOut"], target["easeIn"])]


class WaveExampleTest(unittest.TestCase):

    def setUp(self):
        self.relaxed = load(RELAXED)
        self.wave = load(WAVE)
        self.names = [j["name"] for j in self.wave["skeleton"]]

    def track(self, name):
        joint = self.names.index(name)
        return next(t for t in self.wave["tracks"] if t["joint"] == joint)

    def test_committed_file_is_the_script_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "wave.dingle")
            subprocess.run([sys.executable, SCRIPT, RELAXED, out], check=True)
            with open(out, encoding="utf-8") as a, open(WAVE, encoding="utf-8") as b:
                self.assertEqual(a.read(), b.read())

    def test_timing_and_loop(self):
        self.assertEqual(self.wave["timing"]["frames"], 61)
        self.assertEqual(self.wave["timing"]["frameTime"], self.relaxed["timing"]["frameTime"])
        self.assertEqual(self.wave["loop"], {"in": 1, "out": 60})

    def test_five_keys_on_the_moving_joints(self):
        for name in wave_example.MOVING:
            frames = [k["frame"] for k in self.track(name)["rotation"]]
            self.assertEqual(frames, [0, 20, 30, 40, 60], name)

    def test_other_joints_keep_the_relaxed_pose(self):
        relaxed = {t["joint"]: t for t in self.relaxed["tracks"]}
        for track in self.wave["tracks"]:
            if self.names[track["joint"]] in wave_example.MOVING:
                continue
            self.assertEqual(track, relaxed[track["joint"]])

    def test_same_skeleton_and_settings(self):
        for name in ("format", "version", "figure", "scale", "skeleton", "camera"):
            self.assertEqual(self.relaxed[name], self.wave[name])

    def test_first_and_last_key_are_the_relaxed_pose(self):
        relaxed = {t["joint"]: t for t in self.relaxed["tracks"]}
        for name in wave_example.MOVING:
            keys = self.track(name)["rotation"]
            rest = relaxed[self.names.index(name)]["rotation"][0]["values"]
            self.assertEqual(keys[0]["values"], rest)
            self.assertEqual(keys[-1]["values"], rest)

    def test_each_pose_change_has_its_own_choice(self):
        expected = ["Slow both ends", "Overshoot, then settle", "Even", "Slow finish"]
        for name in wave_example.MOVING:
            keys = self.track(name)["rotation"]
            got = [preset(keys[i], keys[i + 1]) for i in range(4)]
            self.assertEqual(got, expected, name)
        self.assertEqual(len(set(expected)), 4)

    def test_choices_are_stored_as_the_tile_row_stores_them(self):
        keys = self.track("rForeArm")["rotation"]
        self.assertEqual([(k["easeIn"], k["easeOut"]) for k in keys],
                         [(False, True), (True, True), (True, False), (False, False), (True, False)])
        self.assertEqual([k.get("curve") for k in keys],
                         [{"type": "ease-in-out"}, {"type": "ease-out-overshoot"}, None, None, None])

    def test_hand_swings_out_and_back(self):
        keys = self.track("rForeArm")["rotation"]
        self.assertEqual(keys[1]["values"], keys[3]["values"])
        self.assertNotEqual(keys[1]["values"], keys[2]["values"])

    def test_overshoot_stays_inside_the_joint_limits(self):
        # forearm bend (Y) 70 -> 25 goes 8 % past: 21.4, limit 0 to 146 (docs/ASSISTANT_GUIDE.md)
        keys = self.track("rForeArm")["rotation"]
        start, end = keys[1]["values"][0], keys[2]["values"][0]
        past = end + (end - start) * 0.08
        self.assertGreater(past, 0)
        self.assertLess(start, 146)

    def test_notes_are_the_reviewed_texts(self):
        with open(TEACHING, encoding="utf-8") as f:
            teaching = f.read()
        texts = {}
        for row in teaching.splitlines():
            m = re.match(r"\| (WAVE-0\d) \|[^|]*\|[^|]*\| ([^|]*) \|", row)
            if m:
                texts[m.group(1)] = m.group(2).replace("<br>", "\n")
        notes = self.wave["extensions"][wave_example.EXTENSION]
        self.assertEqual([k["frame"] for k in notes["keys"]], [0, 20, 30, 40, 60])
        for i, key in enumerate(notes["keys"]):
            self.assertEqual(key["note"], texts["WAVE-0%d" % (i + 1)])
        self.assertEqual(notes["task"], texts["WAVE-06"])
        self.assertEqual([k["choice"] for k in notes["keys"]],
                         ["Slow both ends", "Overshoot, then settle", "Even", "Slow finish", None])


if __name__ == "__main__":
    unittest.main()
