#!/usr/bin/env python3
"""Work board: one small file per job in docs/work/.

    work.py list            table of all jobs (this checkout and open pull requests)
    work.py check           fail when two active jobs claim overlapping files,
                            warn when this branch changes files its job file
                            does not list

Options (both commands):
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

WORK_DIR = "docs/work"
NOT_JOBS = {"README.md", "HISTORY.md"}
STATUSES = ("active", "done")


def run(*cmd):
    return subprocess.run(cmd, check=True, capture_output=True, text=True).stdout


def git(*args):
    return run("git", *args)


def job_file_name(branch):
    return branch.replace("/", "-") + ".md"


# ---------------------------------------------------------------- parsing


class Job:
    def __init__(self, branch, status, pull_request, files, source, problems):
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
    return Job(branch, status, fields.get("Pull request", ""), files, source, problems)


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
    own = WORK_DIR + "/" + job_file_name(job.branch) if job.branch else None
    return [p for p in changed if p != own and not any(matches(pat, p) for pat in job.files)]


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


def main(argv):
    args = [a for a in argv[1:]]
    no_remote = "--no-remote" in args
    mine = "--mine" in args
    args = [a for a in args if not a.startswith("--")]
    if len(args) != 1 or args[0] not in ("list", "check"):
        print(__doc__)
        return 2
    if args[0] == "list":
        return cmd_list(no_remote)
    return cmd_check(no_remote, mine)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
