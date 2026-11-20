#!/usr/bin/env python3
"""Check that tests/retargetsmpl/*.bvh are what tools/gen_smpl22_bvh.py writes
(byte for byte), and that they have the SMPL 22 layout. Run: python3 tests/test_gen_smpl22_bvh.py"""

import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TOOL = os.path.join(ROOT, "tools", "gen_smpl22_bvh.py")
FOLDER = os.path.join(ROOT, "tests", "retargetsmpl")
FILES = ("smpl22_momask.bvh", "smpl22_names.bvh")


def read(path):
    with open(path, "rb") as f:
        return f.read()


def joints(path):
    return [line.split()[1] for line in read(path).decode("ascii").splitlines()
            if line.strip().startswith(("ROOT ", "JOINT "))]


class Smpl22Fixtures(unittest.TestCase):
    def test_output_is_what_the_repository_has(self):
        with tempfile.TemporaryDirectory() as tmp:
            subprocess.run([sys.executable, TOOL, tmp], check=True)
            self.assertEqual(sorted(os.listdir(tmp)), sorted(FILES))
            for name in FILES:
                self.assertEqual(read(os.path.join(tmp, name)), read(os.path.join(FOLDER, name)), name)

    def test_layout(self):
        momask = joints(os.path.join(FOLDER, "smpl22_momask.bvh"))
        self.assertEqual(len(momask), 22)
        self.assertEqual(momask[0], "Hips")
        for name in ("LeftUpLeg", "LeftToe", "Spine2", "RightForeArm", "Head"):
            self.assertIn(name, momask)
        named = joints(os.path.join(FOLDER, "smpl22_names.bvh"))
        self.assertEqual(len(named), 25)  # the wrapper root, the 22 joints and two hand joints
        self.assertEqual(named[:2], ["root", "Pelvis"])
        for name in ("L_Hip", "R_Wrist", "Spine3", "L_Hand"):
            self.assertIn(name, named)

    def test_not_in_the_windows_zip(self):
        # test files, not examples for the owner
        for folder in ("examples", os.path.join("tests", "motions"), os.path.join("tests", "data", "generated")):
            for _, _, files in os.walk(os.path.join(ROOT, folder)):
                for name in FILES:
                    self.assertNotIn(name, files)


if __name__ == "__main__":
    unittest.main()
