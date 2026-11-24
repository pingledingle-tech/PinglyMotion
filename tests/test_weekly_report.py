#!/usr/bin/env python3
"""Tests for tools/weekly_report.py with canned API data (no network).
Run: python3 tests/test_weekly_report.py"""

import os
import sys
import unittest
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools"))

import weekly_report as wr  # noqa: E402
import work  # noqa: E402

NOW = datetime(2026, 11, 24, 12, 0, 0, tzinfo=timezone.utc)

CLOSED = [
    {"number": 12, "title": "Newer fix", "merged_at": "2026-11-23T10:00:00Z"},
    {"number": 10, "title": "Old merge", "merged_at": "2026-11-10T10:00:00Z"},
    {"number": 11, "title": "Closed unmerged", "merged_at": None},
    {"number": 9, "title": "Edge", "merged_at": "2026-11-17T12:00:00Z"},
]
OPEN = [
    {"number": 20, "title": "Draft job", "created_at": "2026-11-24T09:00:00Z", "draft": True},
    {"number": 15, "title": "Old ready", "created_at": "2026-11-21T06:00:00Z", "draft": False},
]
RUNS = [
    {"name": "Build", "head_branch": "main", "conclusion": "success", "created_at": "2026-11-23T10:00:00Z", "run_attempt": 1},
    {"name": "Build", "head_branch": "main", "conclusion": "failure", "created_at": "2026-11-22T10:00:00Z", "run_attempt": 2},
    {"name": "Build", "head_branch": "main", "conclusion": "cancelled", "created_at": "2026-11-22T11:00:00Z", "run_attempt": 1},
    {"name": "Build", "head_branch": "job/x", "conclusion": "failure", "created_at": "2026-11-22T12:00:00Z", "run_attempt": 1},
    {"name": "Build", "head_branch": "main", "conclusion": "success", "created_at": "2026-11-01T10:00:00Z", "run_attempt": 1},
    {"name": "CodeQL", "head_branch": "main", "conclusion": "timed_out", "created_at": "2026-11-23T01:00:00Z"},
]


def job(branch, status, pr):
    return work.parse_job("Branch: %s\nStatus: %s\nPull request: %s\nFiles:\n- a\n" % (branch, status, pr))


class ReportTests(unittest.TestCase):
    def report(self, **over):
        args = dict(closed_pulls=CLOSED, open_pulls=OPEN, runs=RUNS,
                    issues={47: "Nothing to test right now", 74: "- a\n- b\n1. c\n\ntext"},
                    jobs=[job("job/a", "active", "#20"), job("job/b", "done", "#1")], now=NOW)
        args.update(over)
        return wr.build_report(**args)

    def test_merged_last_seven_days_only(self):
        merged = [p["number"] for p in wr.merged_this_week(CLOSED, NOW)]
        self.assertEqual(merged, [9, 12])
        text = self.report()
        self.assertIn("- #12 Newer fix", text)
        self.assertNotIn("Old merge", text)
        self.assertNotIn("Closed unmerged", text)

    def test_open_pulls_with_age(self):
        text = self.report()
        self.assertIn("- #15 Old ready (ready, open 3 d 6 h)", text)
        self.assertIn("- #20 Draft job (draft, open 3 h)", text)
        self.assertLess(text.index("#15"), text.index("#20"))

    def test_build_rate_ignores_other_branches_cancelled_and_old(self):
        total, ok, failed, reruns = wr.build_stats(RUNS, NOW)
        self.assertEqual((total, ok), (2, 1))
        self.assertIn("1 of 2 runs succeeded (50%)", self.report())

    def test_failed_and_reruns(self):
        total, ok, failed, reruns = wr.build_stats(RUNS, NOW)
        self.assertEqual(failed, {"Build": 2, "CodeQL": 1})
        self.assertEqual(reruns, {"Build": 1})
        text = self.report()
        self.assertIn("Failed workflow runs: Build 2, CodeQL 1.", text)
        self.assertIn("needed a rerun: Build 1.", text)

    def test_no_runs(self):
        text = self.report(runs=[])
        self.assertIn("No finished Build runs on main this week.", text)
        self.assertIn("Failed workflow runs: none.", text)

    def test_list_sizes_and_unreadable(self):
        text = self.report(issues={47: "x", 74: None})
        self.assertIn("Issue 47 (Owner test list): 1 lines, 0 list entries, 1 characters.", text)
        self.assertIn("Issue 74 (Jobs list): could not be read.", text)
        self.assertIn("4 lines, 3 list entries", self.report())

    def test_active_jobs(self):
        text = self.report()
        self.assertIn("## Job files still active (1)", text)
        self.assertIn("- job/a (pull request #20)", text)
        self.assertNotIn("job/b", text)
        self.assertIn("## Job files still active (0)", self.report(jobs=[]))

    def test_empty_everything(self):
        text = wr.build_report([], [], [], {47: "", 74: ""}, [], NOW)
        self.assertIn("## Merged pull requests (0)", text)
        self.assertIn("## Open pull requests (0)", text)


if __name__ == "__main__":
    unittest.main()
