#!/usr/bin/env python3
"""Stop personal data from entering the repository.

Mode 1, commits (check_privacy.py BASE HEAD) checks every commit between BASE
and HEAD:

- author and committer email must be a GitHub noreply address
  (...@users.noreply.github.com, or noreply@github.com for commits GitHub makes)
- commit messages and added lines may not contain an email address other than
  a noreply address
- commit messages, added lines and author names may not contain a blocked word.
  Blocked words are stored only as SHA-256 hashes of the lower-case word, so
  this file does not publish them. Add a hash with:
      python3 -c "import hashlib; print(hashlib.sha256(b'word').hexdigest())"

Mode 2, text (check_privacy.py --event NAME FILE) checks the text of a GitHub
event with the same email rule and blocked words. NAME is the event name
(GITHUB_EVENT_NAME) and FILE the event payload (GITHUB_EVENT_PATH):

- pull_request, pull_request_target, issues: title and body
- issue_comment, pull_request_review_comment: comment body
- pull_request_review: review body
Other events are ignored. The text is read from the file, never from the
command line or the environment, so it does not show up in the log.

Messages name where the problem is, never the matched text.

Usage: check_privacy.py BASE HEAD
       check_privacy.py --event NAME FILE
"""

import hashlib
import json
import os
import re
import subprocess
import sys

BLOCKED_HASHES = {
    "57420bcdd3e60c60399516f9d79bd2e4460b415cd7b291c94b59beae24fb922c",
    "8e17d1a8738801cf7a6f14e289f021df079b3c06a4d82219f9f721a204058bc2",
}

EMAIL = re.compile(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}")
WORD = re.compile(r"[A-Za-zÀ-ɏ]+")


def git(*args):
    return subprocess.run(["git", *args], check=True, capture_output=True, text=True).stdout


def allowed_email(address):
    address = address.lower()
    return address.endswith("@users.noreply.github.com") or address == "noreply@github.com"


def problems_in(text, where):
    found = []
    for address in EMAIL.findall(text):
        if not allowed_email(address):
            found.append(f"{where}: email address that is not a noreply address")
    for word in WORD.findall(text):
        if hashlib.sha256(word.lower().encode()).hexdigest() in BLOCKED_HASHES:
            found.append(f"{where}: blocked word")
    return found


def event_texts(name, payload):
    """(where, text) pairs to check for a GitHub event payload."""
    texts = []

    def add(obj, label, *fields):
        if not isinstance(obj, dict):
            return
        for field in fields:
            value = obj.get(field)
            if isinstance(value, str) and value:
                texts.append((f"{label} {field}", value))

    def number_of(key):
        obj = payload.get(key)
        return obj.get("number") if isinstance(obj, dict) else None

    if name in ("pull_request", "pull_request_target"):
        add(payload.get("pull_request"), f"pull request #{number_of('pull_request')}", "title", "body")
    elif name == "issues":
        add(payload.get("issue"), f"issue #{number_of('issue')}", "title", "body")
    elif name in ("issue_comment", "pull_request_review_comment"):
        parent = number_of("issue") or number_of("pull_request")
        add(payload.get("comment"), f"comment on #{parent}", "body")
    elif name == "pull_request_review":
        add(payload.get("review"), f"review on #{number_of('pull_request')}", "body")
    return texts


def report(problems, summary_title):
    # each problem once; the message never repeats the matched text
    problems = sorted(set(problems))
    for problem in problems:
        print(problem)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary and problems:
        with open(summary, "a", encoding="utf-8") as f:
            f.write(f"### {summary_title}\n\n")
            f.writelines(f"- {problem}\n" for problem in problems)
    if problems:
        print(f"\n{len(problems)} problem(s). Remove personal data.")
        return 1
    print("No personal data found.")
    return 0


def check_event(name, path):
    with open(path, encoding="utf-8") as f:
        payload = json.load(f)
    problems = []
    for where, text in event_texts(name, payload):
        problems += problems_in(text, where)
    url = ""
    for key in ("comment", "review", "issue", "pull_request"):
        obj = payload.get(key)
        if isinstance(obj, dict) and obj.get("html_url"):
            url = obj["html_url"]
            break
    if problems and url:
        problems = [f"{p} ({url})" for p in problems]
    return report(problems, "Personal data found in text")


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--event":
        return check_event(sys.argv[2], sys.argv[3])
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    base, head = sys.argv[1], sys.argv[2]
    problems = []

    for sha in git("rev-list", f"{base}..{head}").split():
        short = sha[:7]
        an, ae, cn, ce = git("show", "-s", "--format=%an%n%ae%n%cn%n%ce", sha).splitlines()
        if not allowed_email(ae):
            problems.append(f"commit {short}: author email is not a GitHub noreply address")
        if not allowed_email(ce):
            problems.append(f"commit {short}: committer email is not a GitHub noreply address")
        problems += problems_in(an + " " + cn, f"commit {short} author/committer name")
        problems += problems_in(git("show", "-s", "--format=%B", sha), f"commit {short} message")

    diff = git("diff", "--unified=0", "--no-color", f"{base}...{head}")
    path = "?"
    for line in diff.splitlines():
        if line.startswith("+++ "):
            path = line[6:] if line.startswith("+++ b/") else line[4:]
        elif line.startswith("+") and not line.startswith("+++"):
            problems += problems_in(line[1:], f"added line in {path}")

    return report(problems, "Personal data found in commits")


if __name__ == "__main__":
    sys.exit(main())
