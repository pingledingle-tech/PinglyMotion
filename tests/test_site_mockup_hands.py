#!/usr/bin/env python3
"""The Hands tab in the clickable mockup uses the real data: the nine Firestorm hand
presets (names, order and numbers) and the 30 finger joints of joints.json.
Run: python3 tests/test_site_mockup_hands.py"""

import json
import math
import os
import re
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
DATA = os.path.join(ROOT, "bin", "data", "firestorm")
FINGERS = ["Thumb", "Index", "Middle", "Ring", "Pinky"]


def read(*parts):
    with open(os.path.join(ROOT, *parts), encoding="utf-8") as f:
        return f.read()


def mockup_presets():
    html = read("docs", "mockup", "index.html")
    block = re.search(r"const HAND_PRESETS = \[[^\n]*\n(.*?)\];", html, re.S)
    assert block, "HAND_PRESETS not found in the mockup"
    return [json.loads(line.strip().rstrip(",")) for line in block.group(1).splitlines() if line.strip()]


def preset_file(name):
    text = read("bin", "data", "firestorm", name)
    parts = re.split(r"<key>(m\w+)</key>", text)[1:]
    out = {}
    for key, body in zip(parts[0::2], parts[1::2]):
        rot = re.search(r"<key>rotation</key>\s*<array>(.*?)</array>", body, re.S)
        out[key] = [round(math.degrees(float(x))) for x in re.findall(r"<real>([^<]+)</real>", rot.group(1))]
    return out


class MockupHands(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(read("bin", "data", "firestorm", "joints.json"))
        self.presets = mockup_presets()

    def test_names_and_order(self):
        self.assertEqual([p[0] for p in self.presets], [p["name"] for p in self.data["handPresets"]])
        self.assertEqual(len(self.presets), 9)

    def test_numbers_match_the_files(self):
        for p, entry in zip(self.presets, self.data["handPresets"]):
            vals = preset_file(entry["file"])
            for side, hand in (("Left", p[1]), ("Right", p[2])):
                want = [vals["mHand%s%d%s" % (f, n, side)] for f in FINGERS for n in (1, 2, 3)]
                self.assertEqual(hand, want, "%s, %s hand" % (p[0], side))

    def test_mirror_rule(self):
        # the mockup copies a hand to the other with X and Z turned round (HAND_MIRROR)
        for p in self.presets:
            for left, right in zip(p[1], p[2]):
                self.assertEqual([-left[0], left[1], -left[2]], right, p[0])

    def test_finger_joints(self):
        hands = [j for j in self.data["joints"] if j["tab"] == "Hands"]
        self.assertEqual(len(hands), 30)
        names = {j["name"] for j in hands}
        html = read("docs", "mockup", "index.html")
        for f, fname in (("Thumb", "Thumb"), ("Index", "Index Finger"), ("Middle", "Middle Finger"), ("Ring", "Ring Finger"), ("Pinky", "Pinky Finger")):
            self.assertIn('["%s", "%s"]' % (f, fname), html)
            for part in ("Base", "Mid", "Tip"):
                self.assertIn(fname + " " + part, names)
        self.assertIn('const PARTS = [["1", "Base"], ["2", "Mid"], ["3", "Tip"]];', html)


if __name__ == "__main__":
    unittest.main()
