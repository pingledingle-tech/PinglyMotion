#!/usr/bin/env python3
"""Check that tests/data/generated is what tools/gen_edge_bvh.py writes (byte for byte),
and that the copies in fuzz/corpus/bvh are the same files. Run: python3 tests/test_gen_edge_bvh.py"""

import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TOOL = os.path.join(ROOT, "tools", "gen_edge_bvh.py")
DATA = os.path.join(ROOT, "tests", "data", "generated")
CORPUS = os.path.join(ROOT, "fuzz", "corpus", "bvh")


def read(path):
    with open(path, "rb") as f:
        return f.read()


class GeneratedFiles(unittest.TestCase):
    def test_output_is_what_the_repository_has(self):
        with tempfile.TemporaryDirectory() as tmp:
            subprocess.run([sys.executable, TOOL, tmp], check=True)
            written = sorted(os.listdir(tmp))
            self.assertEqual(written, sorted(n for n in os.listdir(DATA) if n.endswith(".bvh")))
            for name in written:
                self.assertEqual(read(os.path.join(tmp, name)), read(os.path.join(DATA, name)), name)

    def test_two_runs_are_equal(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            subprocess.run([sys.executable, TOOL, a], check=True)
            subprocess.run([sys.executable, TOOL, b], check=True)
            for name in os.listdir(a):
                self.assertEqual(read(os.path.join(a, name)), read(os.path.join(b, name)), name)

    def test_fuzz_seeds_are_copies(self):
        seeds = [n for n in os.listdir(CORPUS) if "_edge_" in n]
        self.assertTrue(seeds)
        for name in seeds:
            original = "edge_" + name.split("_edge_", 1)[1]
            self.assertEqual(read(os.path.join(CORPUS, name)), read(os.path.join(DATA, original)), name)

    def test_nothing_in_the_windows_zip_folder(self):
        # generated edge cases are not test motions for the owner
        motions = os.path.join(ROOT, "tests", "motions")
        self.assertFalse([n for n in os.listdir(motions) if n.startswith("edge_")])


if __name__ == "__main__":
    unittest.main()
