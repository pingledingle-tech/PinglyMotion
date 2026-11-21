#!/usr/bin/env python3
"""Tests for tools/update_combined.sh (issue 245): pull requests from forks or other accounts are
refused before git is touched, and the checked head commit is the one that would be merged.
The GitHub API is mocked with a small fake gh command."""
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT = os.path.join(ROOT, "tools", "update_combined.sh")
REPO = "pingledingle-tech/PinglyMotion"
OWNER = "pingledingle-tech"
SHA = "0123456789abcdef0123456789abcdef01234567"

FAKE_GH = """#!/usr/bin/env python3
import os, sys
# called as: gh api repos/<repo>/pulls/<n>
number = sys.argv[-1].rsplit("/", 1)[-1]
path = os.path.join(os.environ["MOCK_DIR"], number + ".json")
if not os.path.exists(path):
    sys.stderr.write("HTTP 404\\n")
    sys.exit(1)
sys.stdout.write(open(path).read())
"""


def pull(number, state="open", head_repo=REPO, login=OWNER, sha=SHA):
    head = {"sha": sha, "repo": {"full_name": head_repo} if head_repo else None}
    return {"number": number, "state": state, "head": head, "user": {"login": login}}


class Mock:
    def __init__(self):
        self.dir = tempfile.mkdtemp()
        self.gh = os.path.join(self.dir, "gh")
        with open(self.gh, "w") as f:
            f.write(FAKE_GH)
        os.chmod(self.gh, os.stat(self.gh).st_mode | stat.S_IEXEC)

    def add(self, answer):
        with open(os.path.join(self.dir, "%d.json" % answer["number"]), "w") as f:
            json.dump(answer, f)

    def run(self, args, cwd=ROOT, script=SCRIPT):
        env = dict(os.environ, MOCK_DIR=self.dir, PINGLYMOTION_GH=self.gh)
        p = subprocess.run(["bash", script] + args, cwd=cwd, env=env, capture_output=True, text=True)
        return p.returncode, p.stdout + p.stderr


failures = []


def expect(cond, what, out=""):
    if not cond:
        failures.append(what + ("\n" + out if out else ""))


def main():
    m = Mock()
    m.add(pull(1))
    m.add(pull(2, head_repo="someone/PinglyMotion"))
    m.add(pull(3, login="someone"))
    m.add(pull(4, state="closed"))
    m.add(pull(5, head_repo=None))           # the fork was deleted
    m.add(pull(6, sha="not-a-sha"))
    m.add(pull(7, sha="1111111111111111111111111111111111111111"))

    code, out = m.run(["1", "7", "--dry-run"])
    expect(code == 0, "own pull requests pass the dry run", out)
    expect("would merge pull request 1 at " + SHA in out, "the checked head commit is named", out)
    expect("would merge pull request 7 at 1111111111111111111111111111111111111111" in out, "second pull request named", out)

    for number, reason in [(2, "does not come from"), (3, "not opened by the owner"), (4, "not open"),
                           (5, "does not come from"), (6, "no valid head commit")]:
        code, out = m.run([str(number), "--dry-run"])
        expect(code != 0 and "REFUSED pull request %d" % number in out and reason in out,
               "pull request %d is refused (%s)" % (number, reason), out)
        expect("would merge" not in out, "nothing would be merged for %d" % number, out)

    # one bad pull request in the list stops all of them
    code, out = m.run(["1", "2", "--dry-run"])
    expect(code != 0 and "would merge" not in out, "a fork in the list stops the whole run", out)

    code, out = m.run(["99", "--dry-run"])
    expect(code != 0 and "Cannot read pull request 99" in out, "an unknown pull request stops", out)
    code, out = m.run(["12; echo hi", "--dry-run"])
    expect(code == 2 and "Not a pull request number" in out, "only numbers are accepted", out)

    # without --dry-run a fork is refused before git is touched: no test/combined branch appears
    work = tempfile.mkdtemp()
    try:
        subprocess.run(["git", "init", "-q", work], check=True)
        os.makedirs(os.path.join(work, "tools"))
        script = os.path.join(work, "tools", "update_combined.sh")
        shutil.copy(SCRIPT, script)
        code, out = m.run(["2", "--push"], cwd=work, script=script)
        expect(code != 0 and "REFUSED pull request 2" in out, "a real run refuses a fork", out)
        branches = subprocess.run(["git", "-C", work, "branch", "--list", "test/combined"],
                                  capture_output=True, text=True).stdout
        expect(branches.strip() == "", "no test/combined branch was made", branches)
    finally:
        shutil.rmtree(work, ignore_errors=True)
        shutil.rmtree(m.dir, ignore_errors=True)

    if failures:
        for f in failures:
            print("FAIL:", f)
        return 1
    print("update_combined.sh: all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
