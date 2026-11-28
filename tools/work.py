#!/usr/bin/env python3
"""Work board: one small file per job in docs/work/.

    work.py list            table of all jobs (this checkout and open pull requests)
    work.py check           fail when two active jobs claim overlapping files,
                            warn when this branch changes files its job file
                            does not list
    work.py archive         git mv every job file with "Status: done" whose pull
                            request is merged (asked from GitHub) into
                            docs/work/archive/. Skips "Pull request: pending"
                            and numbers GitHub does not know. Option: --dry-run
                            (only print what would move)
    work.py check-lists     check the queue in the jobs list (issue 74): warn
                            when the "## Queue" section is missing, or when a
                            queue entry names a pull request that is already
                            merged or closed (its job ran, so it should have
                            left the queue). Always exits 0. Never writes to
                            GitHub. The jobs list holds only the queue and
                            "Waiting for the owner"; what runs is the open pull
                            requests, what is done is their report comments.
                            Options: --annotate (print ::warning:: lines for CI),
                            --body-file PATH (use this text instead of issue 74)

    work.py status          one short read-only answer: every open pull request (draft or
                            ready, branch, merge conflict state, each required check with
                            how long it has run, minutes since the last push, "stopped?"
                            over 40, first line of the latest comment), the last 3 Build
                            runs on main (event=push), the numbered queue lines of the
                            jobs list (issue 74, 100 characters each) and the first line of
                            each test in the owner test list (issue 47). Check runs are
                            read with per_page=100. Never writes to GitHub. Exits 0.

Option for list and check:
    --no-remote   do not ask GitHub, use the job files in this checkout only

Option for check:
    --mine        only fail for overlaps that involve the current branch's job

A job file is docs/work/<branch with "/" replaced by "-">.md. Its header is
fixed (see docs/work/README.md):

    Branch: feat/example
    Status: active
    Pull request: #12
    Files:
    - src/example.*
    - docs/EXAMPLE.md

    ## What
    Free text.

Python 3 standard library only. GitHub is reached with the `gh` command
(REST only); if it is missing or fails, the script says so and goes on with
the local files.
"""

import base64
import fnmatch
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime, timezone

WORK_DIR = "docs/work"
NOT_JOBS = {"README.md", "HISTORY.md"}
STATUSES = ("active", "done")
JOBS_LIST_ISSUE = 74
ARCHIVE_DIR = WORK_DIR + "/archive"


def run(*cmd):
    return subprocess.run(cmd, check=True, capture_output=True, text=True).stdout


def git(*args):
    return run("git", *args)


def job_file_name(branch):
    return branch.replace("/", "-") + ".md"


# ---------------------------------------------------------------- parsing


class Job:
    def __init__(self, branch, status, pull_request, files, source, problems):
        self.what = ""  # first sentence of "## What", set by parse_job
        self.branch = branch
        self.status = status
        self.pull_request = pull_request
        self.files = files
        self.source = source  # "local" or "pull request #N"
        self.problems = problems

    @property
    def active(self):
        return self.status == "active"


def parse_job(text, source="local"):
    """Read the fixed header of a job file. Never raises; collects problems."""
    fields = {}
    files = []
    problems = []
    in_files = False
    for raw in text.splitlines():
        line = raw.strip()
        if line.startswith("##"):
            break
        if in_files and line.startswith("- "):
            pattern = line[2:].strip().strip("`")
            if pattern:
                files.append(pattern)
            continue
        if not line:
            in_files = False
            continue
        m = re.match(r"^(Branch|Status|Pull request|Files):\s*(.*)$", line)
        if not m:
            in_files = False
            continue
        key, value = m.group(1), m.group(2).strip()
        in_files = key == "Files"
        if key == "Files":
            if value:
                problems.append("'Files:' must be followed by a list, one '- path' per line")
        elif key in fields:
            problems.append("'%s:' appears twice" % key)
        else:
            fields[key] = value
    branch = fields.get("Branch", "")
    status = fields.get("Status", "").lower()
    if not branch:
        problems.append("missing 'Branch:'")
    if status not in STATUSES:
        problems.append("'Status:' must be active or done, not '%s'" % fields.get("Status", ""))
    job = Job(branch, status, fields.get("Pull request", ""), files, source, problems)
    job.what = what_sentence(text)
    return job


def what_sentence(text):
    """First sentence of the '## What' part of a job file ('' if there is none)."""
    m = re.search(r"^##\s+What\s*$", text, re.M)
    if not m:
        return ""
    body = text[m.end():]
    nxt = re.search(r"^##\s", body, re.M)
    if nxt:
        body = body[:nxt.start()]
    flat = " ".join(body.split())
    end = re.search(r"[.!?](?=\s|$)", flat)
    return flat[:end.end()] if end else flat


def pr_number(value):
    """Number from a 'Pull request:' value such as '#12', or None."""
    m = re.match(r"^#?(\d+)$", (value or "").strip())
    return int(m.group(1)) if m else None


def local_jobs(root="."):
    jobs = []
    directory = os.path.join(root, WORK_DIR)
    if not os.path.isdir(directory):
        return jobs
    for name in sorted(os.listdir(directory)):
        if not name.endswith(".md") or name in NOT_JOBS:
            continue
        with open(os.path.join(directory, name), encoding="utf-8") as handle:
            job = parse_job(handle.read())
        if job.branch and job_file_name(job.branch) != name:
            job.problems.append(
                "file name %s does not match branch %s (expected %s)"
                % (name, job.branch, job_file_name(job.branch))
            )
        job.source = "local " + name
        jobs.append(job)
    return jobs


# ----------------------------------------------------------------- GitHub


def repo_slug():
    slug = os.environ.get("GITHUB_REPOSITORY", "").strip()
    if slug:
        return slug
    url = git("remote", "get-url", "origin").strip()
    m = re.search(r"github\.com[:/]+([^/]+)/([^/]+?)(?:\.git)?/?$", url)
    if not m:
        raise RuntimeError("cannot read owner/repo from the origin URL")
    return m.group(1) + "/" + m.group(2)


def gh_api(*args):
    return run("gh", "api", *args)


def remote_jobs(notes):
    """Job files on the head branches of open pull requests.

    Returns a list of Jobs. Anything that goes wrong is added to `notes`.
    """
    try:
        slug = repo_slug()
        out = gh_api(
            "--paginate",
            "repos/%s/pulls?state=open&per_page=100" % slug,
            "--jq",
            ".[] | {number: .number, ref: .head.ref, repo: .head.repo.full_name}",
        )
    except FileNotFoundError:
        notes.append("gh is not installed: open pull requests not checked, local files only")
        return []
    except (subprocess.CalledProcessError, RuntimeError) as error:
        detail = (getattr(error, "stderr", "") or str(error)).strip()
        last = detail.splitlines()[-1] if detail else "no details"
        notes.append("GitHub call failed (%s): local files only" % last)
        return []
    jobs = []
    for line in out.splitlines():
        if not line.strip():
            continue
        pr = json.loads(line)
        repo = pr.get("repo") or slug
        path = "repos/%s/contents/%s/%s" % (repo, WORK_DIR, job_file_name(pr["ref"]))
        try:
            text = gh_api("-H", "Accept: application/vnd.github.raw", "-X", "GET", path, "-f", "ref=" + pr["ref"])
        except subprocess.CalledProcessError:
            notes.append("pull request #%d (%s) has no job file" % (pr["number"], pr["ref"]))
            continue
        job = parse_job(text, "pull request #%d" % pr["number"])
        if not job.pull_request or job.pull_request.lower() in ("pending", "none", "-"):
            job.pull_request = "#%d" % pr["number"]
        jobs.append(job)
    return jobs


def collect(no_remote, current):
    notes = []
    jobs = {}
    for job in local_jobs():
        jobs[job.branch] = job
    if not no_remote:
        for job in remote_jobs(notes):
            if job.branch != current:  # the checkout is newer than the pushed branch
                jobs[job.branch] = job
    return jobs, notes


# --------------------------------------------------------------- patterns


def glob_regex(pattern):
    """'**' crosses '/', '*' and '?' do not. A trailing '/' means the whole directory."""
    if pattern.endswith("/"):
        pattern += "**"
    out = []
    i = 0
    while i < len(pattern):
        c = pattern[i]
        if pattern.startswith("**/", i):
            out.append("(?:.*/)?")
            i += 3
        elif pattern.startswith("**", i):
            out.append(".*")
            i += 2
        elif c == "*":
            out.append("[^/]*")
            i += 1
        elif c == "?":
            out.append("[^/]")
            i += 1
        else:
            out.append(re.escape(c))
            i += 1
    return re.compile("^" + "".join(out) + "$")


def has_glob(pattern):
    return pattern.endswith("/") or any(c in pattern for c in "*?")


def matches(pattern, path):
    return glob_regex(pattern).match(path) is not None


def overlap(a, b, repo_files):
    """Path (or pattern) that both patterns claim, or None."""
    if a == b:
        return a
    ra, rb = glob_regex(a), glob_regex(b)
    for path in repo_files:
        if ra.match(path) and rb.match(path):
            return path
    # files that do not exist yet: a literal path against a pattern
    if not has_glob(a) and rb.match(a):
        return a
    if not has_glob(b) and ra.match(b):
        return b
    return None


def job_overlaps(job_a, job_b, repo_files):
    found = []
    for a in job_a.files:
        for b in job_b.files:
            where = overlap(a, b, repo_files)
            if where:
                found.append((a, b, where))
    return found


def all_overlaps(jobs, repo_files, mine=None):
    active = sorted((j for j in jobs if j.active), key=lambda j: j.branch)
    result = []
    for i, a in enumerate(active):
        for b in active[i + 1:]:
            if mine and mine not in (a.branch, b.branch):
                continue
            found = job_overlaps(a, b, repo_files)
            if found:
                result.append((a, b, found))
    return result


def unlisted_changes(job, changed):
    """Changed paths the job file does not list. The job's own file and job files
    moved into docs/work/archive/ by `archive` never need listing."""
    own = WORK_DIR + "/" + job_file_name(job.branch) if job.branch else None
    return [p for p in changed if p != own and not p.startswith(ARCHIVE_DIR + "/")
            and not any(matches(pat, p) for pat in job.files)]


# --------------------------------------------------------------- commands


def current_branch():
    name = os.environ.get("GITHUB_HEAD_REF", "").strip()
    if name:
        return name
    try:
        name = git("rev-parse", "--abbrev-ref", "HEAD").strip()
    except subprocess.CalledProcessError:
        return ""
    return "" if name == "HEAD" else name


def cmd_list(no_remote):
    jobs, notes = collect(no_remote, current_branch())
    rows = [("Branch", "Status", "Files", "Pull request")]
    for job in sorted(jobs.values(), key=lambda j: (j.status != "active", j.branch)):
        rows.append((job.branch, job.status or "?", ", ".join(job.files) or "-", job.pull_request or "-"))
    widths = [max(len(r[i]) for r in rows) for i in range(3)]
    for r in rows:
        print("  ".join(r[i].ljust(widths[i]) for i in range(3)) + "  " + r[3])
    for note in notes:
        print("note: " + note, file=sys.stderr)
    for job in jobs.values():
        for problem in job.problems:
            print("problem in %s: %s" % (job.source, problem), file=sys.stderr)
    return 0


def changed_files():
    try:
        out = git("diff", "--name-only", "origin/main...HEAD")
    except subprocess.CalledProcessError:
        return None
    return [line for line in out.splitlines() if line]


def cmd_check(no_remote, mine_only):
    branch = current_branch()
    jobs, notes = collect(no_remote, branch)
    status = 0
    for note in notes:
        print("note: " + note, file=sys.stderr)

    for job in jobs.values():
        for problem in job.problems:
            print("job file problem (%s): %s" % (job.source, problem))
            if job.source.startswith("local"):
                status = 1

    repo_files = [p for p in git("ls-files").splitlines() if p]
    mine = branch if mine_only and branch in jobs else None
    for a, b, found in all_overlaps(list(jobs.values()), repo_files, mine):
        status = 1
        print("OVERLAP between active jobs %s (%s) and %s (%s):" % (a.branch, a.source, b.branch, b.source))
        for pa, pb, where in found[:10]:
            print("  %s  <->  %s   (both claim %s)" % (pa, pb, where))
        if len(found) > 10:
            print("  ... and %d more" % (len(found) - 10))

    if branch and branch not in ("main", "master"):
        changed = changed_files()
        own = jobs.get(branch)
        if changed is None:
            print("warning: cannot compare with origin/main, changed files not checked")
        elif own is None:
            if changed:
                print("warning: no job file %s/%s for branch %s" % (WORK_DIR, job_file_name(branch), branch))
        else:
            for path in unlisted_changes(own, changed):
                print("warning: %s changes %s, which its job file does not list" % (branch, path))
    if status == 0:
        print("No overlapping jobs.")
    return status


# ---------------------------------------------------------------- archive


def archive_plan(jobs, states):
    """(moves, skipped): what `archive` moves and what it leaves, with a reason.

    jobs    Job objects read from local files (source is "local <file name>")
    states  {pull request number: "open" | "merged" | "closed"}; unknown numbers are absent
    moves   [file name] of done jobs whose pull request is merged
    skipped [(file name, reason)] for done jobs that stay
    """
    moves, skipped = [], []
    for job in sorted(jobs, key=lambda j: j.source):
        name = job.source.split(" ", 1)[-1]
        if job.status != "done":
            continue
        n = pr_number(job.pull_request)
        if n is None:
            skipped.append((name, "no pull request number (%s)" % (job.pull_request or "empty")))
        elif states.get(n) == "merged":
            moves.append(name)
        elif n in states:
            skipped.append((name, "pull request %d is %s" % (n, states[n])))
        else:
            skipped.append((name, "pull request %d not found on GitHub" % n))
    return moves, skipped


def cmd_archive(dry_run):
    jobs = local_jobs()
    done = [j for j in jobs if j.status == "done"]
    numbers = sorted({pr_number(j.pull_request) for j in done if pr_number(j.pull_request) is not None})
    states = {}
    try:
        slug = repo_slug()
    except (RuntimeError, subprocess.CalledProcessError, FileNotFoundError) as error:
        print("cannot read owner/repo (%s): nothing moved" % error)
        return 1
    for n in numbers:
        try:
            info = json.loads(gh_api("repos/%s/pulls/%d" % (slug, n), "--jq",
                                     "{state, merged: (.merged_at != null)}"))
        except FileNotFoundError:
            print("gh is not installed: nothing moved")
            return 1
        except subprocess.CalledProcessError as error:
            text = (error.stderr or "").lower()
            if "404" in text or "not found" in text:
                continue  # not a pull request: reported as not found
            print("GitHub call for pull request %d failed: nothing moved" % n)
            return 1
        states[n] = "merged" if info.get("merged") else info.get("state", "closed")
    moves, skipped = archive_plan(jobs, states)
    for name, reason in skipped:
        print("skipped %s: %s" % (name, reason))
    if moves:
        os.makedirs(ARCHIVE_DIR, exist_ok=True)
    for name in moves:
        if dry_run:
            print("would move %s/%s to %s/" % (WORK_DIR, name, ARCHIVE_DIR))
        else:
            git("mv", "%s/%s" % (WORK_DIR, name), "%s/%s" % (ARCHIVE_DIR, name))
            print("moved %s/%s to %s/" % (WORK_DIR, name, ARCHIVE_DIR))
    if not moves:
        print("Nothing to archive.")
    return 0


# ------------------------------------------------------ jobs list (issue 74)

_PR_LIST = re.compile(r"pull requests?\s+#?(\d+(?:(?:,\s*and\s+|,\s*|\s+and\s+)#?\d+)*)", re.I)
_BARE = re.compile(r"(?<![\w&/])#(\d+)\b")


def pull_refs(text):
    """Numbers named as 'pull request N', 'pull requests N, M and K' or '#N', in order."""
    found = []
    for m in _PR_LIST.finditer(text):
        for n in re.findall(r"\d+", m.group(1)):
            found.append((m.start(1), int(n)))
    for m in _BARE.finditer(text):
        found.append((m.start(), int(m.group(1))))
    seen = []
    for _, n in sorted(found, key=lambda x: x[0]):
        if n not in seen:
            seen.append(n)
    return seen


def list_section(body, title_start):
    """Text of the '## <title_start>...' part of the jobs list, '' if missing.
    The title must start with title_start as a whole word ('Queue' does not
    match '## Queued')."""
    lines = body.splitlines()
    out = []
    inside = False
    for line in lines:
        if line.startswith("## "):
            if inside:
                break
            inside = re.match(re.escape(title_start.lower()) + r"\b", line[3:].strip().lower()) is not None
            continue
        if inside:
            out.append(line)
    return "\n".join(out)


def check_lists(body, states):
    """Findings (list of strings) for the queue in the jobs list text.

    states   {pull request number: "open" | "merged" | "closed"}; numbers that
             are not pull requests (issues) are simply absent
    """
    if not any(line.startswith("## ") and re.match(r"queue\b", line[3:].strip().lower())
               for line in body.splitlines()):
        return ["#%d has no '## Queue' section" % JOBS_LIST_ISSUE]
    findings = []
    for n in pull_refs(list_section(body, "Queue")):
        if states.get(n) in ("merged", "closed"):
            findings.append("pull request %d is %s but still named in the queue in #%d"
                            % (n, states[n], JOBS_LIST_ISSUE))
    return findings


def gh_lines(path, jq):
    out = gh_api("--paginate", path, "--jq", jq)
    return [json.loads(x) for x in out.splitlines() if x.strip()]


def fetch_states(slug, numbers, notes):
    """{number: "open" | "merged" | "closed"} for the pull requests among
    `numbers` (issue numbers are left out), or None when GitHub cannot be asked."""
    states = {}
    for n in numbers:
        try:
            info = json.loads(gh_api("repos/%s/pulls/%d" % (slug, n), "--jq", "{state, merged: (.merged_at != null)}"))
        except FileNotFoundError:
            notes.append("gh is not installed: GitHub not checked")
            return None
        except subprocess.CalledProcessError as error:
            text = (error.stderr or "").lower()
            if "404" in text or "not found" in text:
                continue  # not a pull request (an issue number)
            last = (error.stderr or "").strip().splitlines()
            notes.append("GitHub call failed (%s)" % (last[-1] if last else "no details"))
            return None
        states[n] = "merged" if info.get("merged") else info.get("state", "closed")
    return states


def fetch_body(slug, notes):
    try:
        return gh_api("repos/%s/issues/%d" % (slug, JOBS_LIST_ISSUE), "--jq", ".body")
    except FileNotFoundError:
        notes.append("gh is not installed: cannot read issue %d" % JOBS_LIST_ISSUE)
    except subprocess.CalledProcessError as error:
        last = (error.stderr or "").strip().splitlines()
        notes.append("cannot read issue %d (%s)" % (JOBS_LIST_ISSUE, last[-1] if last else "no details"))
    return None


def cmd_check_lists(annotate, body_file):
    notes = []
    body = None
    slug = None
    try:
        slug = repo_slug()
    except (RuntimeError, subprocess.CalledProcessError, FileNotFoundError):
        notes.append("cannot read owner/repo")
    if body_file:
        with open(body_file, encoding="utf-8") as handle:
            body = handle.read()
    elif slug:
        body = fetch_body(slug, notes)
    states = None
    if body is not None and slug:
        states = fetch_states(slug, pull_refs(list_section(body, "Queue")), notes)
    findings = []
    if states is None:
        for note in notes:
            findings.append("could not finish the check: " + note)
    else:
        findings = check_lists(body, states)
        if not findings:
            print("The queue in the jobs list (#%d) names no merged or closed pull request." % JOBS_LIST_ISSUE)
    for f in findings:
        print(("::warning::" + f) if annotate else "warning: " + f)
    return 0


STALE_MINUTES = 40


def latest_runs(runs):
    """{check name: run} keeping only the newest run (by started_at) of each name."""
    best = {}
    for run_ in runs:
        name = run_.get("name") or "?"
        if name not in best or (run_.get("started_at") or "") > (best[name].get("started_at") or ""):
            best[name] = run_
    return best


def run_state(run_):
    """One word for a check run: passed, failed, running, cancelled, skipped, ..."""
    if run_.get("status") != "completed":
        return "running"
    conclusion = run_.get("conclusion") or "unknown"
    return {"success": "passed", "neutral": "passed", "failure": "failed", "timed_out": "failed",
            "action_required": "failed", "startup_failure": "failed"}.get(conclusion, conclusion)


def minutes_since(iso, now):
    """Whole minutes from an ISO time like 2026-11-22T21:00:00Z to `now` (a datetime); None if unreadable."""
    try:
        then = datetime.strptime(iso, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
    except (TypeError, ValueError):
        return None
    return max(0, int((now - then).total_seconds() // 60))


def merge_text(state):
    if not state or state == "unknown":
        return "unknown"
    return {"dirty": "CONFLICT", "clean": "no conflict", "behind": "behind main (no conflict)",
            "blocked": "no conflict (blocked by rules/checks)", "unstable": "no conflict (a check is not green)",
            "has_hooks": "no conflict", "draft": "no conflict (draft)"}.get(state, state)


REQUIRED_CHECKS = ("linux", "windows", "asan-ubsan", "privacy", "analyze", "dependency-review")
TESTS_ISSUE = 47
LINE_CHARS = 100


def clip(text, size=LINE_CHARS):
    text = " ".join((text or "").split())
    return text if len(text) <= size else text[:size - 1] + "\u2026"


def check_text(name, run_, now):
    """'linux passed 10 min', 'windows running 7 min', 'asan-ubsan not started'."""
    if not run_:
        return "%s not started" % name
    state = run_state(run_)
    start = run_.get("started_at")
    end = run_.get("completed_at") if state != "running" else None
    if state == "running":
        minutes = minutes_since(start, now)
    elif start and end:
        minutes = minutes_since(start, datetime.strptime(end, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc))
    else:
        minutes = None
    return "%s %s%s" % (name, state, "" if minutes is None else " %d min" % minutes)


def first_line(text):
    for line in (text or "").splitlines():
        if line.strip():
            return clip(line.strip("# *_`>-").strip() or line.strip())
    return ""


def format_pull(pull, runs, minutes, comment=None, now=None):
    """Lines for one pull request from canned or live data. `pull` has number, title, draft, head.ref,
    mergeable_state; `runs` is the list of check runs of its head commit; `comment` the latest comment text."""
    now = now or datetime.now(timezone.utc)
    last = "unknown" if minutes is None else "%d min ago%s" % (minutes, " (stopped?)" if minutes > STALE_MINUTES else "")
    branch = (pull.get("head") or {}).get("ref", "?")
    lines = ["#%d %s | %s | %s | push %s | merge: %s" % (
        pull["number"], clip(pull.get("title", ""), 60), "draft" if pull.get("draft") else "ready", branch, last,
        merge_text(pull.get("mergeable_state")))]
    latest = latest_runs(runs)
    if not latest:
        lines.append("  checks: none yet")
    else:
        lines.append("  " + "; ".join(check_text(n, latest.get(n), now) for n in REQUIRED_CHECKS))
    if comment is not None:
        lines.append("  latest comment: %s" % (first_line(comment) or "(empty)"))
    return lines


def format_builds(runs):
    """One line per Build run on main (event=push): number, result, start."""
    if not runs:
        return ["  none found"]
    out = []
    for r in runs[:3]:
        out.append("  Build #%s %s, %s" % (r.get("run_number", "?"), run_state(r), (r.get("created_at") or "?").replace("T", " ").rstrip("Z") + " UTC"))
    return out


def queue_lines(body):
    """The numbered lines of the '## Queue' section of the jobs list, each cut to 100 characters."""
    section = list_section(body or "", "Queue")
    return [clip(l) for l in section.splitlines() if re.match(r"\s*\d+\.\s", l)]


def test_lines(body):
    """First line of each test in the owner test list (paragraphs after the intro paragraph)."""
    paragraphs = [p for p in re.split(r"\n\s*\n", (body or "").strip()) if p.strip()]
    out = [first_line(p) for p in paragraphs[1:]]
    return [l for l in out if l and not l.startswith("Generated by") and l.strip("-") != ""]


def read_mergeable(slug, number, tries=5, pause=3):
    """The pull request with mergeable_state; asks again a few times while GitHub
    has not computed it (null or "unknown"). Leaves it "unknown" if it never does."""
    pull = None
    for attempt in range(tries):
        pull = json.loads(gh_api("repos/%s/pulls/%d" % (slug, number)))
        if pull.get("mergeable_state") not in (None, "unknown"):
            return pull
        if attempt + 1 < tries:
            time.sleep(pause)
    pull["mergeable_state"] = "unknown"
    return pull


def say_error(what, error):
    last = ((getattr(error, "stderr", "") or str(error)).strip().splitlines() or ["no details"])[-1]
    print("%s: could not read (%s)" % (what, last))


def cmd_status():
    """One short, read-only answer: open pull requests, last Build runs on main, the queue, the owner tests."""
    try:
        slug = repo_slug()
        numbers = [p["number"] for p in gh_lines("repos/%s/pulls?state=open&per_page=100" % slug, ".[]|{number}")]
    except FileNotFoundError:
        print("gh is not installed: cannot read GitHub")
        return 0
    except (RuntimeError, subprocess.CalledProcessError) as error:
        say_error("open pull requests", error)
        return 0
    now = datetime.now(timezone.utc)
    print("Read from GitHub at %s UTC" % now.strftime("%Y-%m-%d %H:%M"))
    print("")
    print("Open pull requests:" if numbers else "No open pull requests.")
    for number in sorted(numbers):
        try:
            pull = read_mergeable(slug, number)
            sha = pull["head"]["sha"]
            runs = gh_lines("repos/%s/commits/%s/check-runs?per_page=100" % (slug, sha),
                            ".check_runs[]|{name,status,conclusion,started_at,completed_at}")
            date = gh_api("repos/%s/commits/%s" % (slug, sha), "--jq", ".commit.committer.date").strip()
            minutes = minutes_since(date, now)
            comments = json.loads(gh_api("repos/%s/issues/%d/comments?per_page=100" % (slug, number)))
            comment = comments[-1].get("body", "") if comments else None
        except (subprocess.CalledProcessError, KeyError, ValueError) as error:
            say_error("#%d" % number, error)
            continue
        print("\n".join(format_pull(pull, runs, minutes, comment, now)))
    print("")
    print("Last 3 Build runs on main (push):")
    try:
        data = json.loads(gh_api("repos/%s/actions/workflows/build.yml/runs?branch=main&event=push&per_page=3" % slug))
        print("\n".join(format_builds(data.get("workflow_runs", []))))
    except (subprocess.CalledProcessError, ValueError) as error:
        say_error("Build runs", error)
    pkgs = load_packages()
    print("")
    print("Work packages (%s/):" % PACKAGES_DIR)
    if not pkgs:
        print("  none found")
    else:
        for p in pkgs:
            total = p["done"] + p["todo"]
            print("  %s: %s | status=%s | approved=%s | %d/%d" % (
                p.get("id", "?"), p.get("title", p["file"]),
                p.get("status", "?"), p.get("owner-approved", "no"), p["done"], total))
    print("")
    print("Beta tests (%s):" % BETA_TEST_FILE)
    if not os.path.isfile(BETA_TEST_FILE):
        print("  none")
    else:
        with open(BETA_TEST_FILE, encoding="utf-8") as handle:
            open_items = [clip(l.strip()) for l in handle if re.match(r"^\s*-\s+\[ \]", l)]
        print("\n".join("  " + l for l in open_items) if open_items else "  none")
    return 0


PACKAGES_DIR = "docs/packages"
BETA_TEST_FILE = "docs/testing/next-beta.md"
STATUS_FILE = "docs/STATUS.md"


def parse_package(path):
    """Parse header fields and task counts from a docs/packages/WP-*.md file."""
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    meta = {"file": os.path.basename(path)}
    done = 0
    todo = 0
    in_header = True
    for line in text.splitlines():
        if in_header:
            if not line.strip():
                in_header = False
                continue
            m = re.match(r"^(ID|Title|Status|Owner-approved|Priority|Reason):\s*(.*)$", line)
            if m:
                meta[m.group(1).lower()] = m.group(2).strip()
                continue
        if re.match(r"^\s*-\s+\[x\]", line, re.IGNORECASE):
            done += 1
        elif re.match(r"^\s*-\s+\[ \]", line):
            todo += 1
    meta["done"] = done
    meta["todo"] = todo
    return meta


def load_packages(packages_dir=PACKAGES_DIR):
    if not os.path.isdir(packages_dir):
        return []
    files = sorted(f for f in os.listdir(packages_dir) if f.startswith("WP-") and f.endswith(".md"))
    pkgs = [parse_package(os.path.join(packages_dir, f)) for f in files]
    pkgs.sort(key=lambda p: (int(p.get("priority", "999") or "999"), p.get("id", "")))
    return pkgs


def count_open_tests(path=BETA_TEST_FILE):
    if not os.path.isfile(path):
        return 0
    with open(path, encoding="utf-8") as handle:
        return sum(1 for line in handle if re.match(r"^\s*-\s+\[ \]", line))


def render_status(packages, open_pulls=None, open_tests=0):
    lines = [
        "# Project Status (Generated)",
        "",
        "Generated by `python3 tools/work.py render-status --write`. Do not edit by hand.",
        "",
        "## Open Pull Requests",
        "",
    ]
    if open_pulls:
        for pr in open_pulls:
            kind = "draft" if pr.get("draft") else "ready"
            lines.append("- **#%s** (%s) — %s (`%s`)" % (
                pr.get("number", "?"), kind, pr.get("title", ""), (pr.get("head") or {}).get("ref", "?")))
    elif open_pulls is None:
        lines.append("- *(offline render — run without `--no-remote` to include open PRs)*")
    else:
        lines.append("- None running right now.")
    lines += [
        "",
        "## Work Packages (`docs/packages/`)",
        "",
        "| ID | Package | Status | Approved | Progress | Reason |",
        "|---|---|---|---|---|---|",
    ]
    for p in packages:
        total = p["done"] + p["todo"]
        progress = "%d / %d" % (p["done"], total)
        lines.append("| %s | [%s](packages/%s) | %s | %s | %s | %s |" % (
            p.get("id", "?"), p.get("title", p["file"]), p["file"],
            p.get("status", "?"), p.get("owner-approved", "no"),
            progress, p.get("reason", "")))
    lines += [
        "",
        "## Other Lists",
        "",
        "- **Beta test checklist:** [`docs/testing/next-beta.md`](testing/next-beta.md) (%d open item(s))" % open_tests,
        "- **Parked items:** [`docs/packages/PARKED.md`](packages/PARKED.md)",
        "",
    ]
    return "\n".join(lines)


def cmd_render_status(write, no_remote):
    packages = load_packages()
    open_tests = count_open_tests()
    open_pulls = None
    if not no_remote:
        try:
            slug = repo_slug()
            open_pulls = gh_lines("repos/%s/pulls?state=open&per_page=100" % slug,
                                  ".[]|{number,title,draft,head:{ref:.head.ref}}")
        except (FileNotFoundError, RuntimeError, subprocess.CalledProcessError):
            open_pulls = None
    md = render_status(packages, open_pulls=open_pulls, open_tests=open_tests)
    if write:
        with open(STATUS_FILE, "w", encoding="utf-8") as handle:
            handle.write(md)
        print("Wrote %s" % STATUS_FILE)
    else:
        print(md)
    return 0


def main(argv):
    args = [a for a in argv[1:]]
    no_remote = "--no-remote" in args
    mine = "--mine" in args
    annotate = "--annotate" in args
    write = "--write" in args
    values = {}
    for opt in ("--body-file",):
        if opt in args:
            i = args.index(opt)
            values[opt] = args[i + 1] if i + 1 < len(args) else None
            del args[i:i + 2]
    dry_run = "--dry-run" in args
    args = [a for a in args if not a.startswith("--")]
    if len(args) != 1 or args[0] not in ("list", "check", "check-lists", "archive", "status", "render-status"):
        print(__doc__)
        return 2
    if args[0] == "render-status":
        return cmd_render_status(write, no_remote)
    if args[0] == "status":
        return cmd_status()
    if args[0] == "archive":
        return cmd_archive(dry_run)
    if args[0] == "check-lists":
        return cmd_check_lists(annotate, values.get("--body-file"))
    if args[0] == "list":
        return cmd_list(no_remote)
    return cmd_check(no_remote, mine)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
