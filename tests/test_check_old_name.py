#!/usr/bin/env python3
"""Tests for tools/check_old_name.py."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
import check_old_name as c  # noqa: E402

OLD = "ping" + "mator"


class Tests(unittest.TestCase):
    def test_plain_hit(self):
        self.assertEqual(c.bad_text("src/a.cpp", "x\n// " + OLD.upper() + "\n"), [2])

    def test_clean(self):
        self.assertEqual(c.bad_text("src/a.cpp", "PinglyMotion\n"), [])

    def test_formerly_sentence(self):
        text = "PinglyMotion, formerly called " + OLD.capitalize() + ".\n"
        self.assertEqual(c.bad_text("README.md", text), [])
        self.assertEqual(c.bad_text("src/a.cpp", text), [1])

    def test_formerly_does_not_hide_second_hit(self):
        text = "formerly called " + OLD + " and " + OLD + "\n"
        self.assertEqual(c.bad_text("README.md", text), [1])

    def test_fork_names(self):
        text = "branch feat/" + OLD + "-link and " + OLD + "_link\n"
        self.assertEqual(c.bad_text("docs/SECURITY_AUDIT.md", text), [])
        self.assertEqual(c.bad_text("docs/DEV.md", text), [1])

    def test_rename_job_files(self):
        self.assertEqual(c.bad_text("docs/work/refactor-rename-docs.md", OLD), [])
        self.assertEqual(c.bad_text("docs/work/feat-x.md", OLD), [1])


if __name__ == "__main__":
    unittest.main()
