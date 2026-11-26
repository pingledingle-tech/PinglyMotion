#!/usr/bin/env python3
"""The scenario runner pinglymotion_rlvtest (package A): every scenarios/*.rlvtest passes, exit code 0;
each file in tests/rlvtest/failing fails with exit code 1 and says exactly why, with the line number.
Usage: test_rlvtest.py <path of pinglymotion_rlvtest>
Licensed under the GNU General Public License version 2 or later."""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUNNER = sys.argv[1]
env = dict(os.environ, QT_QPA_PLATFORM="offscreen")
problems = []


def run(*args):
    p = subprocess.run([RUNNER, *args], capture_output=True, text=True, timeout=120, env=env)
    return p.returncode, p.stdout.replace("\r\n", "\n")


# the real scenarios all pass
scenarios = sorted(f for f in os.listdir(os.path.join(ROOT, "scenarios")) if f.endswith(".rlvtest"))
code, out = run(os.path.join(ROOT, "scenarios"))
if code != 0:
    problems.append("scenarios/ must pass, exit code was %d:\n%s" % (code, out))
for f in scenarios:
    name = f[:-len(".rlvtest")]
    if "PASS %s (" % name not in out:
        problems.append("no PASS line for scenario %s:\n%s" % (name, out))
if not out.endswith("%d of %d scenarios passed.\n" % (len(scenarios), len(scenarios))):
    problems.append("wrong last line:\n" + out)

# files that must fail, and the exact words
failing = os.path.join(ROOT, "tests", "rlvtest", "failing")
expected = {
    "wrong_state": [
        "FAIL wrong_state",
        "  Line 4: expected blocked:sendim not to be true, but Send instant messages is blocked.",
        "  Line 5: expected blocked:sendim with \"allowed\", but Send instant messages is blocked.",
    ],
    "unknown_fact": ["FAIL unknown_fact", "  Line 3: \"blocked:nonsense\" is not an action that can be blocked. Some actions: sendim, recvim, sendchat, recvchat, sendchannel, tplm, tploc, tplure ..."],
    "no_expect": ["FAIL no_expect", "  The scenario has no expect line, so it tests nothing."],
}
for name, lines in expected.items():
    code, out = run(os.path.join(failing, name + ".rlvtest"))
    if code != 1:
        problems.append("%s must fail with exit code 1, got %d" % (name, code))
    got = out.split("\n")
    for i, want in enumerate(lines):
        if i >= len(got) or got[i] != want:
            problems.append("%s line %d: wanted %r, got %r" % (name, i + 1, want, got[i] if i < len(got) else None))
# the whole failing folder: exit 1 and one summary line
code, out = run(failing)
if code != 1 or not out.endswith("0 of 3 scenarios passed.\n"):
    problems.append("failing/ must fail all 3, exit code %d:\n%s" % (code, out))
# nothing to run is a failure, not a pass
code, out = run(os.path.join(ROOT, "docs"))
if code != 1:
    problems.append("a folder without scenarios must give exit code 1, got %d" % code)
code, out = run(os.path.join(ROOT, "no_such_folder"))
if code != 1:
    problems.append("a missing folder must give exit code 1, got %d" % code)

if problems:
    print("\n\n".join(problems))
    sys.exit(1)
print("test_rlvtest: ok")
