#!/usr/bin/env python3
"""Weekly read-only report on the repository, written as Markdown.

Used by .github/workflows/weekly-report.yml, which appends the output to the
run's job summary. Reads GitHub through `gh api` (GET only) and the job files
in the checkout; it never writes anything to GitHub. The logic is in
build_report(), which takes plain data (the JSON the API returns) so the tests
need no network (tests/test_weekly_report.py).

Run by hand: python3 tools/weekly_report.py   (needs gh and GH_TOKEN)
"""

import json
import os
import subprocess
import sys
from datetime import datetime, timedelta, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import work  # noqa: E402

DAYS = 7
LISTS = ((47, "Owner test list"), (74, "Jobs list"))


def parse_time(value):
    return datetime.strptime(value, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)


def age_text(since, now):
    seconds = int((now - since).total_seconds())
    days, rest = divmod(max(seconds, 0), 86400)
    hours = rest // 3600
    if days:
        return "%d d %d h" % (days, hours)
    return "%d h" % hours


def merged_this_week(closed_pulls, now):
    start = now - timedelta(days=DAYS)
    found = []
    for pull in closed_pulls:
        merged = pull.get("merged_at")
        if merged and parse_time(merged) >= start:
            found.append(pull)
    return sorted(found, key=lambda p: p["number"])


def build_stats(runs, now):
    """Success rate of the Build workflow on main, and failures and reruns of all workflows."""
    start = now - timedelta(days=DAYS)
    build_total = build_ok = 0
    failed = {}
    reruns = {}
    for run in runs:
        created = run.get("created_at")
        if not created or parse_time(created) < start:
            continue
        name = run.get("name", "?")
        conclusion = run.get("conclusion")
        if name == "Build" and run.get("head_branch") == "main" and conclusion in ("success", "failure", "cancelled", "timed_out"):
            if conclusion != "cancelled":
                build_total += 1
                if conclusion == "success":
                    build_ok += 1
        if conclusion in ("failure", "timed_out"):
            failed[name] = failed.get(name, 0) + 1
        if (run.get("run_attempt") or 1) > 1:
            reruns[name] = reruns.get(name, 0) + 1
    return build_total, build_ok, failed, reruns


def list_size(body):
    lines = [l for l in (body or "").splitlines() if l.strip()]
    items = [l for l in lines if l.lstrip().startswith(("- ", "* ")) or l.lstrip()[:2].rstrip(".").isdigit()]
    return len(body or ""), len(lines), len(items)


def active_jobs(jobs):
    return [j for j in jobs if j.status == "active"]


def counts(mapping):
    if not mapping:
        return "none"
    return ", ".join("%s %d" % (k, mapping[k]) for k in sorted(mapping))


def build_report(closed_pulls, open_pulls, runs, issues, jobs, now):
    out = ["# Weekly report", "", "Read %s UTC, covering the last %d days." % (now.strftime("%Y-%m-%d %H:%M"), DAYS), ""]
    merged = merged_this_week(closed_pulls, now)
    out.append("## Merged pull requests (%d)" % len(merged))
    out.append("")
    out.extend(["- #%d %s" % (p["number"], p["title"]) for p in merged] or ["- none"])
    out.append("")
    out.append("## Open pull requests (%d)" % len(open_pulls))
    out.append("")
    rows = []
    for pull in sorted(open_pulls, key=lambda p: p["created_at"]):
        kind = "draft" if pull.get("draft") else "ready"
        rows.append("- #%d %s (%s, open %s)" % (pull["number"], pull["title"], kind, age_text(parse_time(pull["created_at"]), now)))
    out.extend(rows or ["- none"])
    out.append("")
    total, ok, failed, reruns = build_stats(runs, now)
    out.append("## Build on main")
    out.append("")
    if total:
        out.append("- %d of %d runs succeeded (%d%%)." % (ok, total, round(100 * ok / total)))
    else:
        out.append("- No finished Build runs on main this week.")
    out.append("")
    out.append("## Failed or flaky checks")
    out.append("")
    out.append("- Failed workflow runs: %s." % counts(failed))
    out.append("- Runs that needed a rerun: %s." % counts(reruns))
    out.append("")
    out.append("## Lists")
    out.append("")
    for number, title in LISTS:
        body = issues.get(number)
        if body is None:
            out.append("- Issue %d (%s): could not be read." % (number, title))
        else:
            chars, lines, items = list_size(body)
            out.append("- Issue %d (%s): %d lines, %d list entries, %d characters." % (number, title, lines, items, chars))
    out.append("")
    active = active_jobs(jobs)
    out.append("## Job files still active (%d)" % len(active))
    out.append("")
    out.extend(["- %s (pull request %s)" % (j.branch, j.pull_request or "none") for j in active] or ["- none"])
    out.append("")
    return "\n".join(out)


# ----------------------------------------------------------------- GitHub (GET only)


def gh_json(path):
    result = subprocess.run(["gh", "api", path], capture_output=True, text=True, check=True)
    return json.loads(result.stdout)


def gather(slug, now):
    base = "repos/%s/" % slug
    since = (now - timedelta(days=DAYS)).strftime("%Y-%m-%d")
    closed = gh_json(base + "pulls?state=closed&sort=updated&direction=desc&per_page=100")
    opened = gh_json(base + "pulls?state=open&per_page=100")
    runs = gh_json(base + "actions/runs?created=%%3E%%3D%s&per_page=100" % since).get("workflow_runs", [])
    issues = {}
    for number, _ in LISTS:
        try:
            issues[number] = gh_json(base + "issues/%d" % number).get("body") or ""
        except (subprocess.CalledProcessError, ValueError):
            issues[number] = None
    return closed, opened, runs, issues


def main():
    now = datetime.now(timezone.utc).replace(microsecond=0)
    slug = work.repo_slug()
    closed, opened, runs, issues = gather(slug, now)
    root = os.path.join(HERE, "..")
    print(build_report(closed, opened, runs, issues, work.local_jobs(root), now))
    return 0


if __name__ == "__main__":
    sys.exit(main())
