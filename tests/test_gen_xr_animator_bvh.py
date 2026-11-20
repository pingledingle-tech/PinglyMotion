#!/usr/bin/env python3
"""Check that tests/data/xr_animator is what tools/gen_xr_animator_bvh.py writes (byte for byte).
Run: python3 tests/test_gen_xr_animator_bvh.py"""

import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TOOL = os.path.join(ROOT, "tools", "gen_xr_animator_bvh.py")
DATA = os.path.join(ROOT, "tests", "data", "xr_animator")


def read(path):
    with open(path, "rb") as f:
        return f.read()


class GeneratedFile(unittest.TestCase):
    def test_output_is_what_the_repository_has(self):
        with tempfile.TemporaryDirectory() as tmp:
            subprocess.run([sys.executable, TOOL, tmp], check=True)
            written = sorted(os.listdir(tmp))
            self.assertEqual(written, sorted(n for n in os.listdir(DATA) if n.endswith(".bvh")))
            for name in written:
                self.assertEqual(read(os.path.join(tmp, name)), read(os.path.join(DATA, name)), name)


if __name__ == "__main__":
    unittest.main()
