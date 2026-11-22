#!/usr/bin/env python3
"""Work board: one small file per job in docs/work/.

    work.py list            table of all jobs (this checkout and open pull requests)
    work.py check           fail when two active jobs claim overlapping files,
                            warn when this branch changes files its job file
                            does not list
    work.py jobs-draft      print a DRAFT of the "Active jobs" and "Merged" parts
                            of the jobs list (issue 74) from the job files and
                            the open pull requests. Never writes to GitHub.
                            Option: --since YYYY-MM-DD (merged since; default today, UTC)
    work.py archive         git mv every job file with "Status: done" whose pull
                            request is merged (asked from GitHub) into
                            docs/work/archive/. Skips "Pull request: pending"
                            and numbers GitHub does not know. Option: --dry-run
                            (only print what would move)
    work.py check-lists     compare issue 74 with reality and print findings
                            (a pull request listed as active that is merged or
                            closed, an open pull request that is not mentioned,
                            a done job file whose pull request is still listed
                            as active). Always exits 0. Never writes to GitHub.
                            Options: --annotate (print ::warning:: lines for CI),
                            --body-file PATH (use this text instead of issue 74)

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
import datetime

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
    """Text of the '## <title_start>...' part of the jobs list, '' if missing."""
    lines = body.splitlines()
    out = []
    inside = False
    for line in lines:
        if line.startswith("## "):
            if inside:
                break
            inside = line[3:].strip().lower().startswith(title_start.lower())
            continue
        if inside:
            out.append(line)
    return "\n".join(out)


def check_lists(body, states, open_prs, jobs):
    """Findings (list of strings) comparing the jobs list text with reality.

    states   {pull request number: "open" | "merged" | "closed"}; numbers that
             are not pull requests (issues) are simply absent
    open_prs the numbers of all open pull requests
    jobs     Job objects read from the job files
    """
    findings = []
    active = pull_refs(list_section(body, "Active jobs"))
    mentioned = set(pull_refs(body))
    for n in active:
        if states.get(n) in ("merged", "closed"):
            findings.append("pull request %d is %s but still listed under Active jobs in #%d"
                            % (n, states[n], JOBS_LIST_ISSUE))
    for n in sorted(open_prs):
        if n not in mentioned:
            findings.append("pull request %d is open but not mentioned in #%d" % (n, JOBS_LIST_ISSUE))
    for job in sorted(jobs, key=lambda j: j.branch):
        n = pr_number(job.pull_request)
        if job.status == "done" and n in active and states.get(n) != "open":
            findings.append("job file %s/%s says done, but pull request %d is still listed under Active jobs in #%d"
                            % (WORK_DIR, job_file_name(job.branch), n, JOBS_LIST_ISSUE))
    return findings


def jobs_draft(jobs, open_prs, merged, today):
    """Draft text for the 'Active jobs' and 'Merged' parts of the jobs list.

    jobs      Job objects (job files plus the ones on open pull request branches)
    open_prs  {number: title} of the open pull requests
    merged    [(number, title)] of pull requests merged in the period, oldest first
    """
    by_pr = {}
    for job in jobs:
        n = pr_number(job.pull_request)
        if n is not None:
            by_pr[n] = job
    lines = ["DRAFT (never written to GitHub; check it before pasting into #%d)" % JOBS_LIST_ISSUE, ""]
    lines.append("## Active jobs (open pull requests)")
    lines.append("")
    for n in sorted(open_prs):
        job = by_pr.get(n)
        if job:
            lines.append("- **%s** (pull request %d, job file `%s/%s`, status %s): %s"
                         % (job.branch, n, WORK_DIR, job_file_name(job.branch), job.status or "?",
                            job.what or open_prs[n]))
        else:
            lines.append("- **%s** (pull request %d, no job file): needs a job file" % (open_prs[n], n))
    lines.append("")
    lines.append("## Merged today (%s)" % today)
    lines.append("")
    for n, title in merged:
        job = by_pr.get(n)
        text = (job.what if job and job.what else title)
        lines.append("- %s (pull request %d)" % (text, n))
    return "\n".join(lines)


def gh_lines(path, jq):
    out = gh_api("--paginate", path, "--jq", jq)
    return [json.loads(x) for x in out.splitlines() if x.strip()]


def fetch_states(slug, numbers, notes):
    """(open {number: title}, states {number: state}) from GitHub, or None on failure."""
    try:
        opened = {p["number"]: p["title"] for p in gh_lines(
            "repos/%s/pulls?state=open&per_page=100" % slug, ".[] | {number, title}")}
    except FileNotFoundError:
        notes.append("gh is not installed: GitHub not checked")
        return None
    except subprocess.CalledProcessError as error:
        last = (error.stderr or "").strip().splitlines()
        notes.append("GitHub call failed (%s)" % (last[-1] if last else "no details"))
        return None
    states = {n: "open" for n in opened}
    for n in numbers:
        if n in states:
            continue
        try:
            info = json.loads(gh_api("repos/%s/pulls/%d" % (slug, n), "--jq", "{state, merged: (.merged_at != null)}"))
        except subprocess.CalledProcessError:
            continue  # not a pull request (an issue number)
        states[n] = "merged" if info.get("merged") else info.get("state", "closed")
    return opened, states


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
    jobs = local_jobs()
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
    result = None
    if body is not None and slug:
        numbers = pull_refs(list_section(body, "Active jobs"))
        result = fetch_states(slug, numbers, notes)
    findings = []
    if result is not None:
        opened, states = result
        findings = check_lists(body, states, opened.keys(), jobs)
    if result is None:
        for note in notes:
            findings.append("could not finish the comparison: " + note)
    elif not findings:
        print("The jobs list (#%d) agrees with the open pull requests and the job files." % JOBS_LIST_ISSUE)
    for f in findings:
        print(("::warning::" + f) if annotate else "warning: " + f)
    return 0


def cmd_jobs_draft(since):
    notes = []
    today = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d")
    since = since or today
    jobs_by_branch, job_notes = collect(False, current_branch())
    notes.extend(job_notes)
    opened, merged = {}, []
    try:
        slug = repo_slug()
        opened = {p["number"]: p["title"] for p in gh_lines(
            "repos/%s/pulls?state=open&per_page=100" % slug, ".[] | {number, title}")}
        closed = json.loads(gh_api(
            "repos/%s/pulls?state=closed&sort=updated&direction=desc&per_page=100" % slug,
            "--jq", "[.[] | select(.merged_at != null) | {number, title, merged_at}]"))
        merged = sorted(((p["number"], p["title"]) for p in closed if p["merged_at"][:10] >= since))
    except FileNotFoundError:
        notes.append("gh is not installed: the open and merged pull requests are missing; "
                     "only what the job files say is printed")
    except (subprocess.CalledProcessError, RuntimeError) as error:
        last = (getattr(error, "stderr", "") or str(error)).strip().splitlines()
        notes.append("GitHub call failed (%s): only what the job files say is printed"
                     % (last[-1] if last else "no details"))
    jobs = list(jobs_by_branch.values())
    if not opened:
        # without GitHub: list the active job files instead
        opened = {}
        for job in jobs:
            n = pr_number(job.pull_request)
            if job.active and n is not None:
                opened[n] = job.branch
    print(jobs_draft(jobs, opened, merged, today))
    for note in notes:
        print("note: " + note, file=sys.stderr)
    return 0


def main(argv):
    args = [a for a in argv[1:]]
    no_remote = "--no-remote" in args
    mine = "--mine" in args
    annotate = "--annotate" in args
    values = {}
    for opt in ("--since", "--body-file"):
        if opt in args:
            i = args.index(opt)
            values[opt] = args[i + 1] if i + 1 < len(args) else None
            del args[i:i + 2]
    dry_run = "--dry-run" in args
    args = [a for a in args if not a.startswith("--")]
    if len(args) != 1 or args[0] not in ("list", "check", "jobs-draft", "check-lists", "archive"):
        print(__doc__)
        return 2
    if args[0] == "archive":
        return cmd_archive(dry_run)
    if args[0] == "jobs-draft":
        return cmd_jobs_draft(values.get("--since"))
    if args[0] == "check-lists":
        return cmd_check_lists(annotate, values.get("--body-file"))
    if args[0] == "list":
        return cmd_list(no_remote)
    return cmd_check(no_remote, mine)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
