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

    def test_formerly_sentence_no_longer_allowed(self):
        text = "PinglyMotion, formerly called " + OLD.capitalize() + ".\n"
        for path in ["README.md", "CLAUDE.md", "docs/SECURITY_AUDIT.md", "docs/DECISIONS.md", "src/a.cpp"]:
            self.assertEqual(c.bad_text(path, text), [1], path)

    def test_fork_names_no_longer_allowed(self):
        text = "branch feat/" + OLD + "-link and " + OLD + "_link\n"
        self.assertEqual(c.bad_text("docs/SECURITY_AUDIT.md", text), [1])

    def test_security_audit_new_wording(self):
        text = ("| Repository | the old repository of the project (now PinglyMotion) |\n"
                "- **Repository:** Aurora-Viewer-Fork, the fork's former live-link branch\n"
                "- **Files:** `crates/aurora-viewer/src/<live-link module>/mod.rs:184-207`\n")
        self.assertEqual(c.bad_text("docs/SECURITY_AUDIT.md", text), [])

    def test_security_audit_file_is_clean(self):
        path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs", "SECURITY_AUDIT.md")
        with open(path, encoding="utf-8") as f:
            self.assertEqual(c.bad_text("docs/SECURITY_AUDIT.md", f.read()), [])

    def test_rename_job_files(self):
        self.assertEqual(c.bad_text("docs/work/refactor-rename-docs.md", OLD), [])
        self.assertEqual(c.bad_text("docs/work/feat-x.md", OLD), [1])

    def test_archived_rename_job_files(self):
        self.assertEqual(c.bad_text("docs/work/archive/refactor-rename-docs.md", OLD), [])
        self.assertEqual(c.bad_text("docs/work/archive/feat-x.md", OLD), [1])


if __name__ == "__main__":
    unittest.main()
