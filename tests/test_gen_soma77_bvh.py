#!/usr/bin/env python3
"""Check that tests/retargetsoma/soma77_kimodo.bvh is what tools/gen_soma77_bvh.py
writes (byte for byte), and that it has the SOMA 77 layout. Run: python3 tests/test_gen_soma77_bvh.py"""

import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TOOL = os.path.join(ROOT, "tools", "gen_soma77_bvh.py")
FIXTURE = os.path.join(ROOT, "tests", "retargetsoma", "soma77_kimodo.bvh")


def read(path):
    with open(path, "rb") as f:
        return f.read()


class Soma77Fixture(unittest.TestCase):
    def test_output_is_what_the_repository_has(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "soma.bvh")
            subprocess.run([sys.executable, TOOL, out], check=True)
            self.assertEqual(read(out), read(FIXTURE))

    def test_layout(self):
        text = read(FIXTURE).decode("ascii")
        joints = [line.split()[1] for line in text.splitlines()
                  if line.strip().startswith(("ROOT ", "JOINT "))]
        self.assertEqual(len(joints), 78)  # the wrapper Root and the 77 joints of SOMA
        self.assertEqual(joints[:2], ["Root", "Hips"])
        self.assertEqual(len(set(joints)), 78)
        self.assertNotIn("End Site", text)  # Kimodo writes leaf joints without End Sites
        for name in ("LeftLeg", "LeftShin", "RightToeBase", "LeftHandIndex4", "RightHandThumb3", "Jaw"):
            self.assertIn(name, joints)

    def test_not_in_the_windows_zip(self):
        # a test file, not an example for the owner
        for folder in ("examples", os.path.join("tests", "motions"), os.path.join("tests", "data", "generated")):
            for _, _, files in os.walk(os.path.join(ROOT, folder)):
                self.assertNotIn("soma77_kimodo.bvh", files)


if __name__ == "__main__":
    unittest.main()
