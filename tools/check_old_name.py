#!/usr/bin/env python3
"""Fails when the old project name comes back (case-insensitive).

    check_old_name.py                 tracked files only
    check_old_name.py BASE HEAD       also the commit messages in BASE..HEAD

The old name is built from two pieces so this file does not contain it.
The only files allowed to contain it are the finished rename job files
docs/work/refactor-rename-* (also after they moved to docs/work/archive/).
Since 22 November 2026 no other exception remains: the live documents no
longer say "formerly called ..." and the security audit describes the Aurora
fork's former live-link branch and module instead of naming them.
"""
import fnmatch
import re
import subprocess
import sys

OLD = re.compile("ping" + "mator", re.IGNORECASE)

# whole files that may contain the old name
ALLOWED_FILES = ["docs/work/refactor-rename-*", "docs/work/archive/refactor-rename-*"]


def match(path, patterns):
    return any(fnmatch.fnmatch(path, p) for p in patterns)


def bad_text(path, text):
    """Line numbers in text with a hit that is not allowed."""
    if match(path, ALLOWED_FILES):
        return []
    hits = []
    for number, line in enumerate(text.splitlines(), 1):
        if OLD.search(line):
            hits.append(number)
    return hits


def tracked_files():
    out = subprocess.check_output(["git", "ls-files", "-z"])
    return [p for p in out.decode("utf-8", "replace").split("\0") if p]


def main(argv):
    problems = []
    for path in tracked_files():
        if OLD.search(path) and not match(path, ALLOWED_FILES):
            problems.append("%s: the old name is in the file name" % path)
            continue
        try:
            with open(path, "rb") as f:
                data = f.read()
        except OSError:
            continue
        if b"\0" in data:
            continue
        for number in bad_text(path, data.decode("utf-8", "replace")):
            problems.append("%s:%d: the old name" % (path, number))
    if len(argv) == 3:
        log = subprocess.check_output(
            ["git", "log", "--format=%H%x00%B%x01", "%s..%s" % (argv[1], argv[2])])
        for entry in log.decode("utf-8", "replace").split("\x01"):
            if "\0" not in entry:
                continue
            sha, message = entry.strip().split("\0", 1)
            if OLD.search(message):
                problems.append("commit %s: the old name in the message" % sha[:10])
    for p in problems:
        print(p)
    if problems:
        print("The old project name must not come back. Write PinglyMotion.")
        return 1
    print("No old project name found.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
