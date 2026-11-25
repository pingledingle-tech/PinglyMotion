#!/usr/bin/env python3
"""Merge technical pull requests that carry the `auto-merge` label.

Used by .github/workflows/auto-merge.yml; tests: tests/test_ci_auto_merge.py.
The project owner chose this on 22 Nov 2026 (docs/DECISIONS.md); the ruleset
on main is not changed by it and still decides what may be merged.

A pull request is squash-merged only when ALL of these hold:

- it is open, not a draft, targets main and its head branch is in this
  repository (no forks);
- it changes nothing under .github/workflows/ (the workflow's token may not
  merge such a change; a session merges it as before);
- it carries the label LABEL, and the newest "labeled" event for that label
  was made by an account with write, maintain or admin permission;
- every required status check of the rules on main (read from the API at run
  time; nothing is merged when they cannot be read) has a completed run on the
  pull request's current head commit with the conclusion success or skipped;
- no other check run or commit status on that commit is failing, cancelled or
  still running. Only the newest run of each check name counts, so a run that
  was cancelled and then re-run does not block.

Anything else: nothing is changed, and the reason is printed. A pending check
means "wait for the next trigger". The merge is asked for with the head commit
the checks were read on, so a push in between makes GitHub refuse it.

A merge made with the workflow's own token starts no workflow on main, so after
a merge this script starts the Build workflow on main (workflow_dispatch), and
the Pages workflow when the pull request changed a website file.

The script does not update the owner test list or the jobs list (issues 47
and 74); the session that labelled the pull request still does that.

Usage (GH_TOKEN and GITHUB_REPOSITORY set, as in the workflow):
  auto_merge.py [--pr N] [--dry-run]
"""

import argparse
import fnmatch
import json
import os
import subprocess
import sys

LABEL = "auto-merge"
BASE_BRANCH = "main"
WRITE_PERMISSIONS = {"admin", "maintain", "write"}

OK_CONCLUSIONS = {"success", "skipped"}
# conclusions of a non-required check that do not block a merge
HARMLESS_CONCLUSIONS = {"success", "skipped", "neutral"}

# GitHub does not let the workflow's own token change workflow files, so a
# pull request that touches them is left for a session to merge
WORKFLOW_DIR = ".github/workflows/"

# check runs that belong to this workflow itself and are ignored
OWN_CHECKS = {"auto-merge"}

# keep in step with the push paths of .github/workflows/pages.yml
PAGES_PATHS = (
    "docs/index.html",
    "docs/home.html",
    "docs/download.html",
    "docs/privacy.html",
    "docs/site.css",
    "docs/site.js",
    "docs/mockup/**",
    "tools/site/**",
    ".github/workflows/pages.yml",
)


def newest_runs(check_runs):
    """The newest check run of each name (the highest id wins)."""
    newest = {}
    for run in check_runs:
        name = run.get("name")
        if name in OWN_CHECKS:
            continue
        if name not in newest or run.get("id", 0) > newest[name].get("id", 0):
            newest[name] = run
    return newest


def decide(pr, repo, required, check_runs, statuses, labeler, labeler_permission, files=()):
    """('merge' | 'wait' | 'skip', reason) for one pull request.

    pr: the pull request object of the REST API.
    repo: "owner/name" of this repository.
    required: list of required check names, or None when they could not be read.
    check_runs: the check runs on the head commit (REST objects).
    statuses: the commit statuses on the head commit (REST objects, newest first).
    labeler: login of the account that added the label last, or None.
    labeler_permission: that account's permission on the repository, or None.
    files: the paths the pull request changes, or None when they could not be read.
    """
    if pr.get("state") != "open":
        return "skip", "not open"
    labels = {lbl.get("name") for lbl in pr.get("labels", [])}
    if LABEL not in labels:
        return "skip", f"no {LABEL} label"
    if pr.get("draft"):
        return "skip", "draft"
    if (pr.get("base") or {}).get("ref") != BASE_BRANCH:
        return "skip", f"does not target {BASE_BRANCH}"
    head_repo = ((pr.get("head") or {}).get("repo") or {}).get("full_name")
    if head_repo != repo:
        return "skip", "head branch is not in this repository (fork)"
    if files is None:
        return "skip", "could not read the changed files"
    if any(f.startswith(WORKFLOW_DIR) for f in files):
        return "skip", f"changes {WORKFLOW_DIR}: the workflow token may not merge that; merge it by hand"
    if not labeler:
        return "skip", "could not find who added the label"
    if labeler_permission not in WRITE_PERMISSIONS:
        return "skip", f"label added by an account without write permission ({labeler_permission or 'unknown'})"
    if not required:
        return "skip", "required checks of main could not be read"

    newest = newest_runs(check_runs)
    for name in required:
        run = newest.get(name)
        if run is None:
            return "wait", f"required check {name} has no run on the head commit yet"
        if run.get("status") != "completed":
            return "wait", f"required check {name} is {run.get('status')}"
        if run.get("conclusion") not in OK_CONCLUSIONS:
            return "skip", f"required check {name} ended with {run.get('conclusion')}"

    for name, run in sorted(newest.items()):
        if run.get("status") != "completed":
            return "wait", f"check {name} is {run.get('status')}"
        if run.get("conclusion") not in HARMLESS_CONCLUSIONS:
            return "skip", f"check {name} ended with {run.get('conclusion')}"

    seen = set()
    for status in statuses:  # newest first: the first of each context counts
        context = status.get("context")
        if context in seen:
            continue
        seen.add(context)
        if status.get("state") == "pending":
            return "wait", f"status {context} is pending"
        if status.get("state") != "success":
            return "skip", f"status {context} is {status.get('state')}"

    return "merge", "all required checks green, nothing failing or running"


def needs_pages(files):
    """True when one of the changed files is published by the Pages workflow."""
    for path in files:
        for pattern in PAGES_PATHS:
            if pattern.endswith("/**"):
                if path.startswith(pattern[:-2]):
                    return True
            elif fnmatch.fnmatchcase(path, pattern):
                return True
    return False


def required_checks(rules):
    """Names of the required status checks in the rules of a branch."""
    names = []
    for rule in rules:
        if rule.get("type") != "required_status_checks":
            continue
        for check in (rule.get("parameters") or {}).get("required_status_checks", []):
            if check.get("context") and check["context"] not in names:
                names.append(check["context"])
    return names


def last_labeler(events):
    """Login of the account behind the newest 'labeled' event for LABEL."""
    login = None
    for event in events:  # oldest first
        if event.get("event") == "labeled" and (event.get("label") or {}).get("name") == LABEL:
            login = (event.get("actor") or {}).get("login")
    return login


# --- GitHub API (only used by main) ---------------------------------------

def api(path, method="GET", fields=None, paginate=False):
    cmd = ["gh", "api", "-X", method, path]
    if paginate:
        cmd += ["--paginate", "--slurp"]
    for key, value in (fields or {}).items():
        cmd += ["-f", f"{key}={value}"]
    out = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout
    data = json.loads(out) if out.strip() else {}
    if paginate and isinstance(data, list):
        merged = []
        for page in data:
            if isinstance(page, list):
                merged.extend(page)
            elif isinstance(page, dict) and "check_runs" in page:
                merged.extend(page["check_runs"])
            else:
                merged.append(page)
        return merged
    return data


def safe(fn, *args, **kwargs):
    try:
        return fn(*args, **kwargs)
    except (subprocess.CalledProcessError, ValueError) as err:
        stderr = getattr(err, "stderr", "") or ""
        print(f"  API call failed: {' '.join(str(a) for a in args)} {stderr.strip()}")
        return None


def permission_of(repo, login):
    data = safe(api, f"repos/{repo}/collaborators/{login}/permission")
    if not data:
        return None
    role = data.get("role_name")
    return role if role in WRITE_PERMISSIONS else data.get("permission")


def handle(repo, pr, required, dry_run):
    number = pr["number"]
    sha = pr["head"]["sha"]
    events = safe(api, f"repos/{repo}/issues/{number}/events?per_page=100", paginate=True) or []
    labeler = last_labeler(events)
    permission = permission_of(repo, labeler) if labeler else None
    changed = safe(api, f"repos/{repo}/pulls/{number}/files?per_page=100", paginate=True)
    files = None if changed is None else [f.get("filename", "") for f in changed]
    runs = safe(api, f"repos/{repo}/commits/{sha}/check-runs?per_page=100", paginate=True)
    statuses = safe(api, f"repos/{repo}/commits/{sha}/statuses?per_page=100", paginate=True)
    if runs is None or statuses is None:
        print(f"#{number}: skip: could not read the checks of {sha[:10]}")
        return
    action, reason = decide(pr, repo, required, runs, statuses, labeler, permission, files)
    print(f"#{number}: {action}: {reason}")
    if action != "merge" or dry_run:
        return
    result = safe(api, f"repos/{repo}/pulls/{number}/merge", method="PUT",
                  fields={"merge_method": "squash", "sha": sha})
    if not result or not result.get("merged"):
        print(f"#{number}: GitHub refused the merge; nothing else done")
        return
    print(f"#{number}: merged as {result.get('sha', '')[:10]}")
    # a merge with the workflow token starts no workflow on main: start them
    workflows = ["build.yml"] + (["pages.yml"] if needs_pages(files) else [])
    for wf in workflows:
        if safe(api, f"repos/{repo}/actions/workflows/{wf}/dispatches", method="POST",
                fields={"ref": BASE_BRANCH}) is not None:
            print(f"  started {wf} on {BASE_BRANCH}")
        else:
            print(f"  could not start {wf} on {BASE_BRANCH}; it runs again at night")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--pr", type=int, help="only this pull request")
    parser.add_argument("--dry-run", action="store_true", help="decide, but do not merge")
    args = parser.parse_args()
    repo = os.environ.get("GITHUB_REPOSITORY")
    if not repo:
        print("GITHUB_REPOSITORY is not set")
        return 1

    rules = safe(api, f"repos/{repo}/rules/branches/{BASE_BRANCH}")
    required = required_checks(rules or [])
    if not required:
        print("Required checks of main could not be read: nothing is merged.")
        return 0
    print("Required checks: " + ", ".join(required))

    if args.pr:
        pr = safe(api, f"repos/{repo}/pulls/{args.pr}")
        prs = [pr] if pr else []
    else:
        prs = safe(api, f"repos/{repo}/pulls?state=open&base={BASE_BRANCH}&per_page=100",
                   paginate=True) or []
    prs = [p for p in prs if LABEL in {lbl.get("name") for lbl in p.get("labels", [])}]
    if not prs:
        print(f"No open pull request with the {LABEL} label.")
    for pr in prs:
        handle(repo, pr, required, args.dry_run)
    return 0


if __name__ == "__main__":
    sys.exit(main())
