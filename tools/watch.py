#!/usr/bin/env python3
"""The watch: a script (no AI) that GitHub runs by itself and that looks after the checks.

Used by .github/workflows/watch.yml (when a check workflow ends, every 10 minutes, and on request).
Standard-library Python plus the `gh` CLI with the workflow's own token, REST only.

What it does, every run, for every open pull request and for `main`:

  - measures the normal time of each check (median of the last 20 green Build runs); a check
    that has run longer than twice that (never under 10 minutes) is STUCK: the run is cancelled
    and its failed jobs are rerun ONCE (never more than one rerun per run and two per pull request)
  - a failed Windows job, when it is the only failed job of its run: one rerun after the run ended
  - required build checks shown as skipped on a ready pull request that changes program files: a comment
    (the fix is an empty commit pushed by the job; the watch never pushes)
  - a pull request with no push and no comment for 3 hours: one short comment
  - `main` red (the newest finished Build run with event=push failed): a comment on the newest merged
    pull request and a line in the run summary

It keeps ONE comment per pull request, found by a fixed first line, and edits it instead of adding
new ones. What it did is also printed and written to the job summary.

It never merges, never starts a job, never creates or deletes branches, never dismisses an alert,
never changes settings or which checks are required, never weakens a check, and never pushes.

  python3 tools/watch.py --dry-run     only print what it would do (writes nothing to GitHub)
  python3 tools/watch.py               do it (needs GH_TOKEN with the permissions of watch.yml)

Tests (fake GitHub data, no network): python3 tests/test_watch.py
"""

import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "ci"))

import changes  # noqa: E402  (tools/ci/changes.py: which files need the program builds)

MARKER = "<!-- pinglymotion-watch -->"
STATE_RE = re.compile(r"<!-- pinglymotion-watch-state (\{.*?\}) -->", re.S)
SAMPLES = 20                   # green runs used to measure the normal time
FLOOR_MINUTES = 10             # a check is never stuck before this
FACTOR = 2                     # stuck at this many times the normal time
MIN_SAMPLES = 3                # fewer green runs than this: no measurement, use the generous limit
UNMEASURED_MINUTES = 60        # the limit for a check with too few samples
IDLE_HOURS = 3
MAX_RERUNS_PER_RUN = 1
MAX_RERUNS_PER_PR = 2
LOG_LINES = 10                 # how many past actions the comment keeps
BUILD_CHECKS = ("linux", "windows", "asan-ubsan")
FAILED = ("failure", "timed_out", "startup_failure")


# ---------------------------------------------------------------- time and numbers

def parse_time(value):
    return datetime.strptime(value, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)


def minutes_between(start, end):
    return (parse_time(end) - parse_time(start)).total_seconds() / 60.0


def median(values):
    ordered = sorted(values)
    n = len(ordered)
    if n == 0:
        return None
    mid = n // 2
    return ordered[mid] if n % 2 else (ordered[mid - 1] + ordered[mid]) / 2.0


def stuck_limit(durations):
    """Minutes after which a check counts as stuck, from the minutes of its recent green runs.
    Twice the median, never under FLOOR_MINUTES; with fewer than MIN_SAMPLES runs there is no
    measurement and the limit is the generous UNMEASURED_MINUTES."""
    if len(durations) < MIN_SAMPLES:
        return UNMEASURED_MINUTES
    return max(FLOOR_MINUTES, FACTOR * median(durations))


def durations_from_jobs(jobs_of_runs):
    """{check name: [minutes]} from the job lists of green runs (only successful, finished jobs)."""
    found = {}
    for jobs in jobs_of_runs:
        for job in jobs:
            if job.get("conclusion") != "success" or not job.get("started_at") or not job.get("completed_at"):
                continue
            found.setdefault(job["name"], []).append(minutes_between(job["started_at"], job["completed_at"]))
    return found


def limits_from(durations):
    return {name: stuck_limit(values) for name, values in durations.items()}


# ---------------------------------------------------------------- reading check runs

def run_id_of(check_run):
    """The workflow run id from a check run's details address (.../actions/runs/<id>/job/<id>), or None."""
    match = re.search(r"/actions/runs/(\d+)", check_run.get("details_url") or "")
    return int(match.group(1)) if match else None


def find_stuck(check_runs, limits, now):
    """Running checks that went over their limit: [(name, run id, minutes running, limit)]."""
    stuck = []
    for cr in check_runs:
        if cr.get("status") != "in_progress" or not cr.get("started_at"):
            continue
        run_id = run_id_of(cr)
        limit = limits.get(cr["name"], UNMEASURED_MINUTES)
        running = (now - parse_time(cr["started_at"])).total_seconds() / 60.0
        if run_id and running > limit:
            stuck.append((cr["name"], run_id, int(running), int(limit)))
    return stuck


def windows_failed_alone(jobs):
    """True when the jobs of one finished run show Windows failed and nothing else failed."""
    if any(j.get("status") != "completed" for j in jobs):
        return False
    failed = [j["name"] for j in jobs if j.get("conclusion") in FAILED]
    return failed == ["windows"]


def skipped_should_have_run(check_runs, changed_files, draft):
    """The three build checks all show skipped on a ready pull request that changes program files."""
    if draft:
        return False
    if not changes.classify(changed_files)["code"]:
        return False
    states = {cr["name"]: cr.get("conclusion") for cr in check_runs if cr["name"] in BUILD_CHECKS
              and cr.get("status") == "completed"}
    return all(states.get(name) == "skipped" for name in BUILD_CHECKS)


def main_is_red(build_runs):
    """The newest finished Build run on main with event=push, if it failed; else None."""
    pushes = [r for r in build_runs if r.get("event") == "push" and r.get("status") == "completed"]
    if not pushes:
        return None
    newest = max(pushes, key=lambda r: r.get("created_at") or "")
    return newest if newest.get("conclusion") in FAILED else None


def idle_since(head_date, comments, own_marker=MARKER):
    """ISO time of the latest push or comment of a pull request (the watch's own comment does not count)."""
    times = [head_date] if head_date else []
    for c in comments:
        if own_marker in (c.get("body") or ""):
            continue
        times.append(c.get("updated_at") or c.get("created_at"))
    times = [t for t in times if t]
    return max(times) if times else None


def is_idle(last_activity, now):
    return bool(last_activity) and (now - parse_time(last_activity)).total_seconds() > IDLE_HOURS * 3600


# ---------------------------------------------------------------- the one comment

def read_state(body):
    match = STATE_RE.search(body or "")
    if match:
        try:
            state = json.loads(match.group(1))
            state.setdefault("reruns", {})
            state.setdefault("log", [])
            return state
        except ValueError:
            pass
    return {"reruns": {}, "log": []}


def can_rerun(state, run_id):
    reruns = state["reruns"]
    if reruns.get(str(run_id), 0) >= MAX_RERUNS_PER_RUN:
        return False, "this run was already rerun once"
    if sum(reruns.values()) >= MAX_RERUNS_PER_PR:
        return False, "this pull request already had 2 reruns"
    return True, ""


def render_comment(findings, state):
    lines = [MARKER, "**Watch** (a script on GitHub, no AI; it edits this one comment)", ""]
    if findings:
        lines += ["- " + f for f in findings]
    else:
        lines.append("Nothing to report now.")
    if state["log"]:
        lines += ["", "What the watch did:"] + ["- " + entry for entry in state["log"][-LOG_LINES:]]
    lines += ["", "<!-- pinglymotion-watch-state %s -->" % json.dumps(state, sort_keys=True, separators=(",", ":"))]
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------- GitHub (REST only)

class GitHub:
    def __init__(self, slug, dry_run):
        self.slug = slug
        self.dry_run = dry_run

    def _gh(self, args, payload=None):
        cmd = ["gh", "api"] + args
        result = subprocess.run(cmd, input=json.dumps(payload) if payload is not None else None,
                                capture_output=True, text=True, timeout=120)
        if result.returncode != 0:
            raise RuntimeError(((result.stderr or result.stdout).strip().splitlines() or ["gh api failed"])[-1])
        return result.stdout

    def get(self, path):
        text = self._gh(["repos/%s/%s" % (self.slug, path)])
        return json.loads(text) if text.strip() else None

    def write(self, method, path, payload=None):
        """POST, PATCH or PUT. In a dry run nothing is sent."""
        if self.dry_run:
            return None
        args = ["-X", method, "repos/%s/%s" % (self.slug, path)]
        if payload is not None:
            args += ["--input", "-"]
        text = self._gh(args, payload)
        return json.loads(text) if text.strip() else None


class Report:
    def __init__(self):
        self.lines = []

    def add(self, text):
        print(text)
        self.lines.append(text)

    def write_summary(self):
        path = os.environ.get("GITHUB_STEP_SUMMARY")
        if path and self.lines:
            with open(path, "a", encoding="utf-8") as f:
                f.write("## Watch\n\n" + "\n".join("- " + l for l in self.lines) + "\n")


class Watch:
    def __init__(self, gh, report, now=None, wait=120, sleep=time.sleep):
        self.gh = gh
        self.report = report
        self.now = now or datetime.now(timezone.utc)
        self.wait = wait
        self.sleep = sleep

    # --- reading
    def limits(self):
        runs = self.gh.get("actions/workflows/build.yml/runs?status=success&per_page=%d" % SAMPLES)
        jobs = []
        for run in (runs or {}).get("workflow_runs", [])[:SAMPLES]:
            try:
                jobs.append(self.gh.get("actions/runs/%d/jobs?per_page=100" % run["id"])["jobs"])
            except (RuntimeError, KeyError, TypeError):
                continue
        return limits_from(durations_from_jobs(jobs))

    def check_runs(self, sha):
        return (self.gh.get("commits/%s/check-runs?per_page=100" % sha) or {}).get("check_runs", [])

    def own_comment(self, number):
        for c in self.gh.get("issues/%d/comments?per_page=100" % number) or []:
            if MARKER in (c.get("body") or ""):
                return c
        return None

    # --- acting
    def restart(self, state, run_id, why, cancel_first):
        """Cancel (when stuck) and rerun the failed jobs of one run, once. Returns (text, did it):
        when it did not rerun, the text is a finding for the comment."""
        ok, reason = can_rerun(state, run_id)
        if not ok:
            return "run %d: %s, not rerun again (%s)." % (run_id, why, reason), False
        if self.gh.dry_run:
            return "run %d: %s; would %srerun the failed jobs once" % (run_id, why, "cancel and " if cancel_first else ""), False
        if cancel_first:
            self.gh.write("POST", "actions/runs/%d/cancel" % run_id)
            waited = 0
            while waited < self.wait:
                self.sleep(10)
                waited += 10
                if (self.gh.get("actions/runs/%d" % run_id) or {}).get("status") == "completed":
                    break
        self.gh.write("POST", "actions/runs/%d/rerun-failed-jobs" % run_id)
        state["reruns"][str(run_id)] = state["reruns"].get(str(run_id), 0) + 1
        return "run %d: %s; %sreran the failed jobs (rerun %d of this pull request)" % (
            run_id, why, "cancelled and " if cancel_first else "", sum(state["reruns"].values())), True

    def look_at_checks(self, sha, limits, state, findings, actions):
        runs = self.check_runs(sha)
        for name, run_id, running, limit in find_stuck(runs, limits, self.now):
            actions.append(self.restart(state, run_id, "check %s ran %d min (stuck limit %d)" % (name, running, limit), True))
        stuck_runs = {r for _, r, _, _ in find_stuck(runs, limits, self.now)}
        for run_id in sorted({run_id_of(cr) for cr in runs if cr["name"] == "windows" and cr.get("conclusion") in FAILED} - {None}):
            if run_id in stuck_runs or state["reruns"].get(str(run_id)):
                continue
            jobs = (self.gh.get("actions/runs/%d/jobs?per_page=100" % run_id) or {}).get("jobs", [])
            if windows_failed_alone(jobs):
                actions.append(self.restart(state, run_id, "the Windows job failed (and only that)", False))
        return runs

    def look_at_pull(self, pull, limits):
        number = pull["number"]
        draft = bool(pull.get("draft"))
        sha = pull["head"]["sha"]
        comment = self.own_comment(number)
        state = read_state(comment["body"] if comment else "")
        findings, actions = [], []
        if not draft:
            runs = self.look_at_checks(sha, limits, state, findings, actions)
            files = [f["filename"] for f in self.gh.get("pulls/%d/files?per_page=100" % number) or []]
            if skipped_should_have_run(runs, files, draft):
                findings.append("linux, windows and asan-ubsan show skipped, but this pull request changes program files "
                                "and should run them. The job should push an empty commit to start them (the watch does not push).")
        commit = self.gh.get("commits/%s" % sha) or {}
        head_date = ((commit.get("commit") or {}).get("committer") or {}).get("date")
        comments = self.gh.get("issues/%d/comments?per_page=100" % number) or []
        last = idle_since(head_date, comments)
        if is_idle(last, self.now):
            findings.append("No push and no comment since %s (more than %d hours)." % (last.replace("T", " ").rstrip("Z") + " UTC", IDLE_HOURS))
        for f in findings:
            self.report.add("#%d: %s" % (number, f))
        self.take(actions, state, findings, "#%d: " % number)
        if not findings and not actions:
            self.report.add("#%d: %s, nothing to do" % (number, "draft" if draft else "ready"))
        self.update_comment(number, comment, findings, state)

    def take(self, actions, state, findings, prefix):
        """Report what the watch did; a rerun goes to the comment's log, a refusal becomes a finding."""
        for text, did in actions:
            self.report.add(prefix + text)
            if did:
                state["log"].append(text)
            elif not self.gh.dry_run:
                findings.append(text)

    def update_comment(self, number, comment, findings, state):
        if not comment and not findings and not state["log"]:
            return
        state["log"] = state["log"][-LOG_LINES:]
        body = render_comment(findings, state)
        if comment and comment.get("body") == body:
            return
        if self.gh.dry_run:
            self.report.add("#%d: would %s the watch comment" % (number, "edit" if comment else "write"))
            return
        if comment:
            self.gh.write("PATCH", "issues/comments/%d" % comment["id"], {"body": body})
        else:
            self.gh.write("POST", "issues/%d/comments" % number, {"body": body})

    def look_at_main(self, limits):
        builds = (self.gh.get("actions/runs?branch=main&event=push&per_page=20") or {}).get("workflow_runs", [])
        builds = [r for r in builds if r.get("name") == "Build"]
        red = main_is_red(builds)
        closed = self.gh.get("pulls?state=closed&sort=updated&direction=desc&per_page=30") or []
        merged = [p for p in closed if p.get("merged_at")]
        target = max(merged, key=lambda p: p["merged_at"]) if merged else None
        state = {"reruns": {}, "log": []}
        comment = None
        if target:
            comment = self.own_comment(target["number"])
            state = read_state(comment["body"] if comment else "")
        findings, actions = [], []
        main = self.gh.get("branches/main") or {}
        sha = (main.get("commit") or {}).get("sha")
        if sha and target:
            self.look_at_checks(sha, limits, state, findings, actions)
        if red:
            findings.append("main is red: the Build run %d on main (%s) failed." % (red["id"], red.get("html_url", "")))
        for text in findings:
            self.report.add("main: " + text)
        self.take(actions, state, findings, "main: ")
        if not red:
            self.report.add("main: the newest finished Build run with event=push is not red")
        if target:
            self.update_comment(target["number"], comment, findings, state)

    def run(self):
        limits = self.limits()
        self.report.add("stuck limits in minutes: " + (", ".join("%s %d" % (n, l) for n, l in sorted(limits.items())) or "none measured"))
        pulls = self.gh.get("pulls?state=open&per_page=100") or []
        for pull in sorted(pulls, key=lambda p: p["number"]):
            try:
                self.look_at_pull(pull, limits)
            except (RuntimeError, KeyError, TypeError, ValueError) as error:
                self.report.add("#%d: could not look at it (%s)" % (pull["number"], error))
        try:
            self.look_at_main(limits)
        except (RuntimeError, KeyError, TypeError, ValueError) as error:
            self.report.add("main: could not look at it (%s)" % error)
        if not pulls:
            self.report.add("no open pull requests")


def slug():
    value = os.environ.get("GITHUB_REPOSITORY")
    if value:
        return value
    return "pingledingle-tech/PinglyMotion"


def main(argv):
    args = argv[1:]
    if any(a not in ("--dry-run",) for a in args):
        print(__doc__)
        return 2
    dry = "--dry-run" in args
    report = Report()
    report.add("dry run: nothing is written to GitHub" if dry else "live run")
    try:
        Watch(GitHub(slug(), dry), report).run()
    finally:
        report.write_summary()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
