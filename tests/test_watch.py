#!/usr/bin/env python3
"""Tests for tools/watch.py with fake GitHub data (no network). Run: python3 tests/test_watch.py"""

import json
import os
import sys
import unittest
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools"))

import watch  # noqa: E402

NOW = datetime(2026, 11, 25, 12, 0, 0, tzinfo=timezone.utc)


def job(name, minutes, conclusion="success"):
    return {"name": name, "conclusion": conclusion, "status": "completed",
            "started_at": "2026-11-25T10:00:00Z",
            "completed_at": "2026-11-25T%02d:%02d:00Z" % (10 + minutes // 60, minutes % 60)}


def check(name, status="completed", conclusion="success", started="2026-11-25T11:50:00Z", run=100):
    return {"name": name, "status": status, "conclusion": conclusion, "started_at": started,
            "details_url": "https://github.com/o/r/actions/runs/%d/job/1" % run}


class NormalTime(unittest.TestCase):
    def test_median_of_even_and_odd(self):
        self.assertEqual(watch.median([3, 1, 2]), 2)
        self.assertEqual(watch.median([1, 2, 3, 10]), 2.5)
        self.assertIsNone(watch.median([]))

    def test_limit_is_twice_the_median(self):
        self.assertEqual(watch.stuck_limit([10, 11, 12, 9, 10]), 20)
        self.assertEqual(watch.stuck_limit([21, 20, 22]), 42)

    def test_never_under_ten_minutes(self):
        self.assertEqual(watch.stuck_limit([1, 1, 2, 1]), 10)

    def test_too_few_samples_is_generous(self):
        self.assertEqual(watch.stuck_limit([5, 6]), watch.UNMEASURED_MINUTES)
        self.assertEqual(watch.stuck_limit([]), watch.UNMEASURED_MINUTES)

    def test_only_green_finished_jobs_count(self):
        found = watch.durations_from_jobs([[job("linux", 10), job("linux", 50, "failure"), job("windows", 12)]])
        self.assertEqual(found, {"linux": [10.0], "windows": [12.0]})

    def test_stuck_when_over_limit(self):
        runs = [check("linux", "in_progress", None, "2026-11-25T11:25:00Z", run=7),   # 35 min
                check("windows", "in_progress", None, "2026-11-25T11:50:00Z", run=7),  # 10 min
                check("privacy", "completed", "success")]
        stuck = watch.find_stuck(runs, {"linux": 20, "windows": 27}, NOW)
        self.assertEqual([(s[0], s[1]) for s in stuck], [("linux", 7)])

    def test_check_without_limit_uses_generous_default(self):
        runs = [check("odd", "in_progress", None, "2026-11-25T11:00:00Z")]  # 60 min exactly: not over
        self.assertEqual(watch.find_stuck(runs, {}, NOW), [])
        runs = [check("odd", "in_progress", None, "2026-11-25T10:50:00Z")]  # 70 min
        self.assertEqual(len(watch.find_stuck(runs, {}, NOW)), 1)


class OneRerun(unittest.TestCase):
    def test_second_rerun_of_same_run_refused(self):
        state = watch.read_state("")
        self.assertTrue(watch.can_rerun(state, 5)[0])
        state["reruns"]["5"] = 1
        self.assertFalse(watch.can_rerun(state, 5)[0])

    def test_third_rerun_of_a_pull_request_refused(self):
        state = {"reruns": {"1": 1, "2": 1}, "log": []}
        ok, why = watch.can_rerun(state, 3)
        self.assertFalse(ok)
        self.assertIn("2 reruns", why)

    def test_state_survives_the_comment(self):
        state = {"reruns": {"9": 1}, "log": ["did a thing"]}
        body = watch.render_comment(["a finding"], state)
        self.assertTrue(body.startswith(watch.MARKER))
        self.assertEqual(watch.read_state(body), state)

    def test_broken_state_is_empty_not_a_crash(self):
        self.assertEqual(watch.read_state("<!-- pinglymotion-watch-state {oops} -->"), {"reruns": {}, "log": []})


class FakeGitHub:
    def __init__(self, data, dry_run=False):
        self.data = data
        self.dry_run = dry_run
        self.writes = []

    def get(self, path):
        if path not in self.data:
            raise RuntimeError("not faked: " + path)
        return self.data[path]

    def write(self, method, path, payload=None):
        if self.dry_run:
            return None
        self.writes.append((method, path, payload))
        return {}


class Quiet:
    def __init__(self):
        self.lines = []

    def add(self, text):
        self.lines.append(text)


class StuckRun(unittest.TestCase):
    def data(self, comment_body=None):
        stuck = check("linux", "in_progress", None, "2026-11-25T11:00:00Z", run=77)  # 60 min
        comments = [{"id": 5, "body": comment_body, "created_at": "2026-11-25T11:00:00Z"}] if comment_body else []
        return {
            "actions/workflows/build.yml/runs?status=success&per_page=20": {"workflow_runs": [{"id": i} for i in range(3)]},
            "actions/runs/0/jobs?per_page=100": {"jobs": [job("linux", 10)]},
            "actions/runs/1/jobs?per_page=100": {"jobs": [job("linux", 10)]},
            "actions/runs/2/jobs?per_page=100": {"jobs": [job("linux", 10)]},
            "pulls?state=open&per_page=100": [{"number": 8, "draft": False, "head": {"sha": "abc"}}],
            "issues/8/comments?per_page=100": comments,
            "commits/abc/check-runs?per_page=100": {"check_runs": [stuck]},
            "pulls/8/files?per_page=100": [{"filename": "src/a.cpp"}],
            "commits/abc": {"commit": {"committer": {"date": "2026-11-25T11:55:00Z"}}},
            "actions/runs/77": {"status": "completed"},
            "actions?": {},
            "actions/runs?branch=main&event=push&per_page=20": {"workflow_runs": []},
            "pulls?state=closed&sort=updated&direction=desc&per_page=30": [],
        }

    def run_watch(self, data, dry=False):
        gh = FakeGitHub(data, dry)
        report = Quiet()
        watch.Watch(gh, report, now=NOW, wait=0, sleep=lambda s: None).run()
        return gh, report

    def test_stuck_run_is_cancelled_then_rerun_once_and_commented(self):
        gh, _ = self.run_watch(self.data())
        methods = [(m, p) for m, p, _ in gh.writes]
        self.assertEqual(methods[0], ("POST", "actions/runs/77/cancel"))
        self.assertEqual(methods[1], ("POST", "actions/runs/77/rerun-failed-jobs"))
        self.assertEqual(methods[2], ("POST", "issues/8/comments"))
        body = gh.writes[2][2]["body"]
        self.assertTrue(body.startswith(watch.MARKER))
        self.assertEqual(watch.read_state(body)["reruns"], {"77": 1})

    def test_second_time_only_a_comment_is_edited(self):
        first, _ = self.run_watch(self.data())
        gh, _ = self.run_watch(self.data(first.writes[2][2]["body"]))
        methods = [(m, p) for m, p, _ in gh.writes]
        self.assertEqual(methods, [("PATCH", "issues/comments/5")])
        self.assertIn("not rerun again", gh.writes[0][2]["body"])

    def test_dry_run_writes_nothing(self):
        gh, report = self.run_watch(self.data(), dry=True)
        self.assertEqual(gh.writes, [])
        self.assertTrue(any("would cancel and rerun" in l for l in report.lines))


class WindowsOnly(unittest.TestCase):
    def test_windows_alone_failed(self):
        self.assertTrue(watch.windows_failed_alone([job("windows", 5, "failure"), job("linux", 5), job("changes", 1)]))

    def test_windows_and_linux_failed(self):
        self.assertFalse(watch.windows_failed_alone([job("windows", 5, "failure"), job("linux", 5, "failure")]))

    def test_linux_alone_failed(self):
        self.assertFalse(watch.windows_failed_alone([job("windows", 5), job("linux", 5, "failure")]))

    def test_run_still_going(self):
        running = job("asan-ubsan", 5)
        running["status"] = "in_progress"
        self.assertFalse(watch.windows_failed_alone([job("windows", 5, "failure"), running]))

    def test_watch_reruns_a_lone_windows_failure_once(self):
        t = StuckRun()
        data = t.data()
        data["commits/abc/check-runs?per_page=100"] = {"check_runs": [check("windows", "completed", "failure", run=88)]}
        data["actions/runs/88/jobs?per_page=100"] = {"jobs": [job("windows", 5, "failure"), job("linux", 5)]}
        gh, _ = t.run_watch(data)
        self.assertIn(("POST", "actions/runs/88/rerun-failed-jobs"), [(m, p) for m, p, _ in gh.writes])
        self.assertNotIn(("POST", "actions/runs/88/cancel"), [(m, p) for m, p, _ in gh.writes])

    def test_watch_leaves_a_linux_failure_alone(self):
        t = StuckRun()
        data = t.data()
        data["commits/abc/check-runs?per_page=100"] = {"check_runs": [check("linux", "completed", "failure", run=88)]}
        gh, _ = t.run_watch(data)
        self.assertEqual([p for _, p, _ in gh.writes if "rerun" in p], [])


class SkippedButShouldHaveRun(unittest.TestCase):
    skipped = [check(n, "completed", "skipped") for n in watch.BUILD_CHECKS]

    def test_all_skipped_with_program_files(self):
        self.assertTrue(watch.skipped_should_have_run(self.skipped, ["src/a.cpp", "docs/x.md"], False))

    def test_text_only_may_skip(self):
        self.assertFalse(watch.skipped_should_have_run(self.skipped, ["docs/x.md", "README.md"], False))

    def test_draft_may_skip(self):
        self.assertFalse(watch.skipped_should_have_run(self.skipped, ["src/a.cpp"], True))

    def test_one_that_ran_is_fine(self):
        runs = [check("linux", "completed", "success"), check("windows", "completed", "skipped"), check("asan-ubsan", "completed", "skipped")]
        self.assertFalse(watch.skipped_should_have_run(runs, ["src/a.cpp"], False))

    def test_missing_checks_are_not_skipped(self):
        self.assertFalse(watch.skipped_should_have_run([], ["src/a.cpp"], False))

    def test_watch_comments_and_does_not_push(self):
        t = StuckRun()
        data = t.data()
        data["commits/abc/check-runs?per_page=100"] = {"check_runs": self.skipped}
        gh, _ = t.run_watch(data)
        self.assertEqual([(m, p) for m, p, _ in gh.writes], [("POST", "issues/8/comments")])
        self.assertIn("skipped", gh.writes[0][2]["body"])


class MainRed(unittest.TestCase):
    def runs(self, conclusion, status="completed"):
        return [{"id": 1, "event": "push", "status": status, "conclusion": conclusion, "created_at": "2026-11-25T09:00:00Z"},
                {"id": 2, "event": "schedule", "status": "completed", "conclusion": "failure", "created_at": "2026-11-25T10:00:00Z"}]

    def test_newest_push_run_failed(self):
        self.assertEqual(watch.main_is_red(self.runs("failure"))["id"], 1)

    def test_green_or_running_is_not_red(self):
        self.assertIsNone(watch.main_is_red(self.runs("success")))
        self.assertIsNone(watch.main_is_red(self.runs(None, "in_progress")))

    def test_only_the_newest_push_run_counts(self):
        older = {"id": 0, "event": "push", "status": "completed", "conclusion": "failure", "created_at": "2026-11-24T09:00:00Z"}
        newer = {"id": 3, "event": "push", "status": "completed", "conclusion": "success", "created_at": "2026-11-25T09:00:00Z"}
        self.assertIsNone(watch.main_is_red([older, newer]))

    def test_watch_comments_on_newest_merged_pull_request(self):
        t = StuckRun()
        data = t.data()
        data["pulls?state=open&per_page=100"] = []
        data["actions/runs?branch=main&event=push&per_page=20"] = {"workflow_runs": [
            {"id": 9, "name": "Build", "event": "push", "status": "completed", "conclusion": "failure",
             "created_at": "2026-11-25T09:00:00Z", "html_url": "https://example/9"}]}
        data["pulls?state=closed&sort=updated&direction=desc&per_page=30"] = [
            {"number": 3, "merged_at": "2026-11-24T10:00:00Z"}, {"number": 4, "merged_at": "2026-11-25T08:00:00Z"},
            {"number": 5, "merged_at": None}]
        data["issues/4/comments?per_page=100"] = []
        data["branches/main"] = {"commit": {"sha": "mmm"}}
        data["commits/mmm/check-runs?per_page=100"] = {"check_runs": []}
        gh, report = t.run_watch(data)
        self.assertEqual([(m, p) for m, p, _ in gh.writes], [("POST", "issues/4/comments")])
        self.assertIn("main is red", gh.writes[0][2]["body"])
        self.assertTrue(any("main is red" in l for l in report.lines))


class Idle(unittest.TestCase):
    def test_idle_after_three_hours(self):
        self.assertTrue(watch.is_idle("2026-11-25T08:59:00Z", NOW))
        self.assertFalse(watch.is_idle("2026-11-25T09:30:00Z", NOW))

    def test_latest_of_push_and_comment_counts_but_not_the_watch(self):
        comments = [{"body": "hi", "created_at": "2026-11-25T11:00:00Z"},
                    {"body": watch.MARKER + " x", "created_at": "2026-11-25T11:59:00Z"}]
        self.assertEqual(watch.idle_since("2026-11-25T05:00:00Z", comments), "2026-11-25T11:00:00Z")



class EveryJobHasATimeLimit(unittest.TestCase):
    """A hung check must be stopped by GitHub itself: every job in every workflow has timeout-minutes
    (a job that calls a reusable workflow cannot have one; the called jobs carry their own)."""

    def test_timeouts(self):
        import glob
        import re
        missing = []
        folder = os.path.join(HERE, "..", ".github", "workflows")
        for path in sorted(glob.glob(os.path.join(folder, "*.yml"))):
            current = None
            in_jobs = False
            for line in open(path, encoding="utf-8").read().split("\n"):
                if line == "jobs:":
                    in_jobs = True
                    continue
                if not in_jobs:
                    continue
                m = re.match(r"^  ([A-Za-z0-9_-]+):\s*$", line)
                if m:
                    current = {"file": os.path.basename(path), "job": m.group(1), "limit": False}
                    missing.append(current)
                elif current and re.match(r"^    (timeout-minutes|uses):", line):
                    current["limit"] = True
        self.assertEqual([(j["file"], j["job"]) for j in missing if not j["limit"]], [])


if __name__ == "__main__":
    unittest.main()
