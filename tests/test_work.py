#!/usr/bin/env python3
"""Tests for tools/work.py (the work board). Run: python3 tests/test_work.py"""

import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools"))

import work  # noqa: E402

REPO_FILES = [
    "src/foo.cpp",
    "src/foo.h",
    "src/bar.cpp",
    "src/sub/deep.cpp",
    "docs/PRODUCT.md",
    "CMakeLists.txt",
]


def job(branch, files, status="active"):
    text = "Branch: %s\nStatus: %s\nPull request: #1\nFiles:\n%s\n\n## What\nText.\n" % (
        branch, status, "\n".join("- " + f for f in files))
    return work.parse_job(text)


class ParseTests(unittest.TestCase):
    def test_reads_header(self):
        j = work.parse_job(
            "Branch: feat/x\nStatus: Active\nPull request: #7\nFiles:\n- src/a.*\n- `docs/b.md`\n\n"
            "## What\n- not a file\n")
        self.assertEqual(j.branch, "feat/x")
        self.assertTrue(j.active)
        self.assertEqual(j.pull_request, "#7")
        self.assertEqual(j.files, ["src/a.*", "docs/b.md"])
        self.assertEqual(j.problems, [])

    def test_text_after_what_is_ignored(self):
        j = work.parse_job("Branch: a\nStatus: done\nFiles:\n- x\n## What\nStatus: active\n- y\n")
        self.assertEqual(j.status, "done")
        self.assertEqual(j.files, ["x"])

    def test_problems(self):
        self.assertTrue(work.parse_job("Status: active\nFiles:\n- x\n").problems)
        self.assertTrue(work.parse_job("Branch: a\nStatus: maybe\n").problems)
        self.assertTrue(work.parse_job("Branch: a\nStatus: done\nFiles: src/x\n").problems)
        self.assertTrue(work.parse_job("Branch: a\nBranch: b\nStatus: done\n").problems)

    def test_file_name(self):
        self.assertEqual(work.job_file_name("feat/camera-keys"), "feat-camera-keys.md")


class PatternTests(unittest.TestCase):
    def test_star_stays_in_directory(self):
        self.assertTrue(work.matches("src/foo.*", "src/foo.cpp"))
        self.assertFalse(work.matches("src/*", "src/sub/deep.cpp"))
        self.assertTrue(work.matches("src/**", "src/sub/deep.cpp"))
        self.assertTrue(work.matches("src/", "src/sub/deep.cpp"))
        self.assertTrue(work.matches("**/deep.cpp", "src/sub/deep.cpp"))
        self.assertFalse(work.matches("src/foo.c", "src/foo.cpp"))

    def test_overlap(self):
        o = work.overlap
        self.assertEqual(o("src/foo.*", "src/foo.cpp", REPO_FILES), "src/foo.cpp")
        self.assertIsNotNone(o("src/*.cpp", "src/foo.*", REPO_FILES))
        self.assertIsNotNone(o("src/", "src/sub/deep.cpp", REPO_FILES))
        self.assertIsNone(o("src/foo.*", "src/bar.*", REPO_FILES))
        self.assertIsNone(o("docs/A.md", "docs/B.md", REPO_FILES))

    def test_overlap_with_new_files(self):
        self.assertEqual(work.overlap("src/new.cpp", "src/new.*", REPO_FILES), "src/new.cpp")
        self.assertEqual(work.overlap("src/new.*", "src/new.*", REPO_FILES), "src/new.*")
        self.assertIsNone(work.overlap("src/new.cpp", "src/old.*", REPO_FILES))


class OverlapTests(unittest.TestCase):
    def test_only_active_jobs_clash(self):
        a = job("a", ["src/foo.*"])
        b = job("b", ["src/foo.cpp"])
        c = job("c", ["src/foo.cpp"], status="done")
        found = work.all_overlaps([a, b, c], REPO_FILES)
        self.assertEqual(len(found), 1)
        self.assertEqual((found[0][0].branch, found[0][1].branch), ("a", "b"))

    def test_no_clash(self):
        self.assertEqual(work.all_overlaps([job("a", ["src/foo.*"]), job("b", ["src/bar.*"])], REPO_FILES), [])

    def test_mine_filter(self):
        a, b, c = job("a", ["src/foo.*"]), job("b", ["src/foo.h"]), job("c", ["src/bar.*"])
        d = job("d", ["src/bar.cpp"])
        self.assertEqual(len(work.all_overlaps([a, b, c, d], REPO_FILES)), 2)
        self.assertEqual(len(work.all_overlaps([a, b, c, d], REPO_FILES, mine="a")), 1)
        self.assertEqual(work.all_overlaps([a, b, c, d], REPO_FILES, mine="zzz"), [])

    def test_unlisted_changes(self):
        j = job("feat/x", ["src/foo.*"])
        changed = ["src/foo.cpp", "src/bar.cpp", "docs/work/feat-x.md"]
        self.assertEqual(work.unlisted_changes(j, changed), ["src/bar.cpp"])


class LocalFilesTests(unittest.TestCase):
    def test_reads_directory_and_checks_names(self):
        with tempfile.TemporaryDirectory() as root:
            directory = os.path.join(root, work.WORK_DIR)
            os.makedirs(directory)
            files = {
                "feat-x.md": "Branch: feat/x\nStatus: active\nFiles:\n- src/x.*\n",
                "wrong-name.md": "Branch: feat/y\nStatus: done\nFiles:\n",
                "README.md": "Branch: ignored\n",
                "HISTORY.md": "Branch: ignored\n",
            }
            for name, text in files.items():
                with open(os.path.join(directory, name), "w", encoding="utf-8") as handle:
                    handle.write(text)
            jobs = work.local_jobs(root)
            self.assertEqual(sorted(j.branch for j in jobs), ["feat/x", "feat/y"])
            bad = [j for j in jobs if j.branch == "feat/y"][0]
            self.assertTrue(any("does not match" in p for p in bad.problems))
            good = [j for j in jobs if j.branch == "feat/x"][0]
            self.assertEqual(good.problems, [])


if __name__ == "__main__":
    unittest.main()
