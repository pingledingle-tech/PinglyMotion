#!/usr/bin/env python3
"""Tests for tools/ci/auto_merge.py. Run: python3 tests/test_ci_auto_merge.py"""

import copy
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools", "ci"))

import auto_merge as am  # noqa: E402

REPO = "example-owner/example-repo"
REQUIRED = ["linux", "windows", "asan-ubsan", "privacy", "analyze", "dependency-review"]


def pr(**changes):
    base = {
        "number": 1,
        "state": "open",
        "draft": False,
        "labels": [{"name": "auto-merge"}],
        "base": {"ref": "main"},
        "head": {"sha": "abc", "repo": {"full_name": REPO}},
    }
    base.update(changes)
    return base


def run(name, conclusion="success", status="completed", run_id=1):
    return {"id": run_id, "name": name, "status": status,
            "conclusion": conclusion if status == "completed" else None}


def all_green():
    runs = [run(n, run_id=i + 1) for i, n in enumerate(REQUIRED)]
    runs += [run("changes", run_id=20), run("website", "skipped", run_id=21),
             run("CodeQL", run_id=22), run("work-board", run_id=23)]
    return runs


def decide(p=None, required=REQUIRED, runs=None, statuses=None,
           labeler="writer", permission="write", files=("tools/work.py",)):
    return am.decide(p or pr(), REPO, required,
                     all_green() if runs is None else runs,
                     statuses or [], labeler, permission, files)[0]


def replace(runs, name, **fields):
    runs = copy.deepcopy(runs)
    for r in runs:
        if r["name"] == name:
            r.update(fields)
    return runs


class Decide(unittest.TestCase):
    def test_label_and_all_green_merges(self):
        self.assertEqual(decide(), "merge")

    def test_admin_and_maintain_may_label(self):
        self.assertEqual(decide(permission="admin"), "merge")
        self.assertEqual(decide(permission="maintain"), "merge")

    def test_required_check_pending_waits(self):
        runs = replace(all_green(), "windows", status="in_progress", conclusion=None)
        self.assertEqual(decide(runs=runs), "wait")

    def test_required_check_without_any_run_waits(self):
        runs = [r for r in all_green() if r["name"] != "analyze"]
        self.assertEqual(decide(runs=runs), "wait")

    def test_required_check_failing_does_not_merge(self):
        for bad in ("failure", "cancelled", "timed_out", "action_required", "neutral"):
            runs = replace(all_green(), "asan-ubsan", conclusion=bad)
            self.assertEqual(decide(runs=runs), "skip", bad)

    def test_other_check_failing_does_not_merge(self):
        runs = replace(all_green(), "work-board", conclusion="failure")
        self.assertEqual(decide(runs=runs), "skip")

    def test_other_check_running_waits(self):
        runs = all_green() + [run("fuzz lsl", status="queued", run_id=30)]
        self.assertEqual(decide(runs=runs), "wait")

    def test_only_the_newest_run_of_a_name_counts(self):
        runs = all_green() + [run("linux", "cancelled", run_id=0)]
        self.assertEqual(decide(runs=runs), "merge")
        runs = all_green() + [run("linux", "failure", run_id=99)]
        self.assertEqual(decide(runs=runs), "skip")
        runs = all_green() + [run("linux", status="in_progress", run_id=99)]
        self.assertEqual(decide(runs=runs), "wait")

    def test_skipped_required_check_counts_as_passed(self):
        runs = replace(all_green(), "windows", conclusion="skipped")
        self.assertEqual(decide(runs=runs), "merge")

    def test_draft_does_not_merge(self):
        self.assertEqual(decide(pr(draft=True)), "skip")

    def test_fork_does_not_merge(self):
        self.assertEqual(decide(pr(head={"sha": "abc", "repo": {"full_name": "someone/fork"}})), "skip")
        self.assertEqual(decide(pr(head={"sha": "abc", "repo": None})), "skip")

    def test_other_base_branch_does_not_merge(self):
        self.assertEqual(decide(pr(base={"ref": "test/combined"})), "skip")

    def test_without_label_or_closed_does_not_merge(self):
        self.assertEqual(decide(pr(labels=[{"name": "documentation"}])), "skip")
        self.assertEqual(decide(pr(state="closed")), "skip")

    def test_label_added_without_write_permission_does_not_merge(self):
        for perm in ("read", "triage", "none", None):
            self.assertEqual(decide(permission=perm), "skip", perm)
        self.assertEqual(decide(labeler=None, permission=None), "skip")

    def test_missing_required_check_names_does_not_merge(self):
        self.assertEqual(decide(required=None), "skip")
        self.assertEqual(decide(required=[]), "skip")

    def test_commit_statuses(self):
        ok = [{"context": "ci/x", "state": "success"}]
        self.assertEqual(decide(statuses=ok), "merge")
        self.assertEqual(decide(statuses=[{"context": "ci/x", "state": "pending"}]), "wait")
        self.assertEqual(decide(statuses=[{"context": "ci/x", "state": "failure"}]), "skip")
        # newest first: an old failure under a newer success does not count
        self.assertEqual(decide(statuses=ok + [{"context": "ci/x", "state": "failure"}]), "merge")

    def test_workflow_change_is_left_for_a_session(self):
        self.assertEqual(decide(files=["docs/DEV.md", ".github/workflows/build.yml"]), "skip")
        self.assertEqual(decide(files=None), "skip")
        self.assertEqual(decide(files=[]), "merge")

    def test_own_check_is_ignored(self):
        runs = all_green() + [run("auto-merge", status="in_progress", run_id=50)]
        self.assertEqual(decide(runs=runs), "merge")


class Helpers(unittest.TestCase):
    def test_required_checks_from_rules(self):
        rules = [
            {"type": "deletion"},
            {"type": "pull_request", "parameters": {"allowed_merge_methods": ["squash"]}},
            {"type": "required_status_checks", "parameters": {"required_status_checks": [
                {"context": "linux", "integration_id": 15368},
                {"context": "privacy", "integration_id": 15368}]}},
        ]
        self.assertEqual(am.required_checks(rules), ["linux", "privacy"])
        self.assertEqual(am.required_checks([]), [])

    def test_last_labeler(self):
        events = [
            {"event": "labeled", "label": {"name": "auto-merge"}, "actor": {"login": "first"}},
            {"event": "labeled", "label": {"name": "bug"}, "actor": {"login": "other"}},
            {"event": "unlabeled", "label": {"name": "auto-merge"}, "actor": {"login": "first"}},
            {"event": "labeled", "label": {"name": "auto-merge"}, "actor": {"login": "second"}},
        ]
        self.assertEqual(am.last_labeler(events), "second")
        self.assertIsNone(am.last_labeler([]))

    def test_needs_pages(self):
        self.assertTrue(am.needs_pages(["docs/index.html"]))
        self.assertTrue(am.needs_pages(["README.md", "tools/site/build_status.py"]))
        self.assertTrue(am.needs_pages(["docs/mockup/a/b.css"]))
        self.assertFalse(am.needs_pages(["docs/DEV.md", "src/main.cpp"]))
        self.assertFalse(am.needs_pages([]))

    def test_pages_paths_match_the_pages_workflow(self):
        path = os.path.join(HERE, "..", ".github", "workflows", "pages.yml")
        with open(path, encoding="utf-8") as f:
            lines = f.read().split("\n")
        start = lines.index("    paths:")
        paths = []
        for line in lines[start + 1:]:
            if not line.startswith("      - "):
                break
            paths.append(line.strip()[2:].strip())
        self.assertEqual(sorted(paths), sorted(am.PAGES_PATHS))


if __name__ == "__main__":
    unittest.main()
