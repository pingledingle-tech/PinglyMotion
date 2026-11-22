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


BODY_74 = """Intro text. Updated 20 Nov (after the merge of pull requests 130, 134 and 157; open: pull request 156).

## Active jobs (open pull requests)

- **Panels P6** (pull request 156, job file `feat/panels-p6`): pop-out panels.
- **Install steps** (part of issue 7, pull request 163, job file `docs/x`): merged already.
- **Closed idea** (#170, no pull request): an issue number.

## Merged today (20 Nov 2026)
- Something old (pull request 130).

## Queued, not started

- Timeline (issue 14), pull request 999 is only queued.
"""


def done_job(branch, pr, status="done"):
    return work.parse_job("Branch: %s\nStatus: %s\nPull request: %s\nFiles:\n- x\n\n## What\nText.\n" % (branch, status, pr))


class ListDriftTests(unittest.TestCase):
    def test_pull_refs(self):
        self.assertEqual(work.pull_refs("pull requests 130, 134 and 157; pull request #9, issue 5, #7"),
                         [130, 134, 157, 9, 7])
        self.assertEqual(work.pull_refs("issue 17 and a#5 and &#7"), [])

    def test_section(self):
        active = work.list_section(BODY_74, "Active jobs")
        self.assertIn("Panels P6", active)
        self.assertNotIn("Something old", active)
        self.assertEqual(work.list_section(BODY_74, "Nothing here"), "")

    def test_what_sentence(self):
        text = "Branch: a\nStatus: done\n\n## What\n\nFirst one. Second\none.\n\n## More\nOther.\n"
        self.assertEqual(work.what_sentence(text), "First one.")
        self.assertEqual(work.what_sentence("Branch: a\n"), "")

    def test_check_lists_exact_findings(self):
        states = {156: "open", 163: "merged", 170: "closed"}
        jobs = [done_job("docs/x", "#163"), done_job("feat/panels-p6", "#156"), done_job("feat/n", "pending")]
        findings = work.check_lists(BODY_74, states, [156, 220], jobs)
        self.assertEqual(findings, [
            "pull request 163 is merged but still listed under Active jobs in #74",
            "pull request 170 is closed but still listed under Active jobs in #74",
            "pull request 220 is open but not mentioned in #74",
            "job file docs/work/docs-x.md says done, but pull request 163 is still listed under Active jobs in #74",
        ])

    def test_check_lists_clean(self):
        body = "## Active jobs\n\n- A (pull request 5)\n\n## Merged\n- B (pull request 4)\n"
        self.assertEqual(work.check_lists(body, {5: "open"}, [5], [done_job("b", "#4")]), [])

    def test_jobs_draft_exact(self):
        jobs = [work.parse_job("Branch: feat/a\nStatus: active\nPull request: #5\nFiles:\n- x\n\n## What\nDoes A. More text.\n"),
                done_job("feat/b", "#4")]
        text = work.jobs_draft(jobs, {5: "Title A", 6: "Title C"}, [(4, "Title B"), (3, "Title D")], "20 Nov 2026")
        self.assertEqual(text, "\n".join([
            "DRAFT (never written to GitHub; check it before pasting into #74)",
            "",
            "## Active jobs (open pull requests)",
            "",
            "- **feat/a** (pull request 5, job file `docs/work/feat-a.md`, status active): Does A.",
            "- **Title C** (pull request 6, no job file): needs a job file",
            "",
            "## Merged today (20 Nov 2026)",
            "",
            "- Text. (pull request 4)",
            "- Title D (pull request 3)",
        ]))


class ArchiveTests(unittest.TestCase):
    def named(self, branch, pr, status="done"):
        j = done_job(branch, pr, status)
        j.source = "local " + work.job_file_name(branch)
        return j

    def test_archive_plan(self):
        jobs = [self.named("a/merged", "#1"), self.named("a/open", "#2"), self.named("a/closed", "#3"),
                self.named("a/pending", "pending"), self.named("a/unknown", "#99"),
                self.named("a/active", "#1", "active"), self.named("a/empty", "")]
        moves, skipped = work.archive_plan(jobs, {1: "merged", 2: "open", 3: "closed"})
        self.assertEqual(moves, ["a-merged.md"])
        self.assertEqual(dict(skipped), {
            "a-open.md": "pull request 2 is open",
            "a-closed.md": "pull request 3 is closed",
            "a-pending.md": "no pull request number (pending)",
            "a-unknown.md": "pull request 99 not found on GitHub",
            "a-empty.md": "no pull request number (empty)",
        })


if __name__ == "__main__":
    unittest.main()
