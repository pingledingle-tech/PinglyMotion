#!/usr/bin/env python3
"""The template folder rlv-tools-template/ (package E): its structure, its rules, its workflow file, and,
with the runner given, that its scenarios pass, its passing examples pass and its failing examples fail.
Usage: test_rlv_template.py [<path of pinglymotion_rlvtest>]
Licensed under the GNU General Public License version 2 or later."""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
T = os.path.join(ROOT, "rlv-tools-template")
problems = []


def want(cond, text):
    if not cond:
        problems.append(text)


def files(sub, ext):
    d = os.path.join(T, sub)
    return sorted(f for f in os.listdir(d) if f.endswith(ext)) if os.path.isdir(d) else []


# the layout
for d in ("scripts", "scenarios", "examples/passing", "examples/failing", "template"):
    want(os.path.isdir(os.path.join(T, d)), "missing folder " + d)
for f in ("README.md", "template/script.lsl", "template/scenario.rlvtest", "template/scenarios.yml"):
    want(os.path.isfile(os.path.join(T, f)), "missing file " + f)

# every product in scripts/ has a scenario, every scenario has expect lines, and none holds a failing one
products = sorted(d for d in os.listdir(os.path.join(T, "scripts")) if os.path.isdir(os.path.join(T, "scripts", d)))
want(products, "scripts/ needs one working example product")
for p in products:
    want(os.path.isfile(os.path.join(T, "scenarios", p + ".rlvtest")), "scripts/%s has no scenarios/%s.rlvtest" % (p, p))
for f in files("scenarios", ".rlvtest"):
    text = open(os.path.join(T, "scenarios", f), encoding="utf-8").read()
    want(re.search(r"^expect ", text, re.M), "scenarios/%s has no expect line" % f)
want(files("examples/failing", ".rlvtest"), "examples/failing needs at least one scenario that must fail")
want(not set(files("examples/failing", ".rlvtest")) & set(files("scenarios", ".rlvtest")), "a failing example must not be in scenarios/")
want(files("examples/passing", ".rlvtest"), "examples/passing needs at least one scenario")

# job 26: the relay trap and the cuff set are products with scenarios, and the README explains the relay
for p in ("relay_trap", "cuff_set"):
    want(p in products, "scripts/%s is missing" % p)
want("relay" in open(os.path.join(T, "scenarios", "relay_trap.rlvtest"), encoding="utf-8").read().lower(), "relay_trap.rlvtest must talk about the relay")
_readme = open(os.path.join(T, "README.md"), encoding="utf-8").read()
for phrase in ("A trap that needs a relay", "The relay that is built into the program", "Running the scenarios on GitHub", "refuse"):
    want(phrase in _readme, "README.md must have: " + phrase)
_wf = open(os.path.join(ROOT, ".github", "workflows", "rlv-scenarios.yml"), encoding="utf-8").read()
want("GITHUB_STEP_SUMMARY" in _wf, "the workflow must write a job summary")

# the rules are said in the README, and the files hold no real account, key, address or money call
readme = open(os.path.join(T, "README.md"), encoding="utf-8").read()
for phrase in ("No real account", "No personal data", "Text from outside", "never a result in Second Life"):
    want(phrase in readme, "README.md must say: " + phrase)
for dirpath, _, names in os.walk(T):
    for n in names:
        path = os.path.join(dirpath, n)
        text = open(path, encoding="utf-8", errors="replace").read()
        rel = os.path.relpath(path, ROOT)
        for banned in ("llGiveMoney", "llTransferLindenDollars", "llHTTPRequest", "llEmail", "llInstantMessage", "llTeleportAgent", "llRezObject"):
            want(banned not in text, "%s uses %s" % (rel, banned))
        want(not re.search(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+\.[A-Za-z]{2,}", text), rel + " holds an e-mail address")

# the workflow to copy calls the shared workflow, pinned by a placeholder the user must replace
wf = open(os.path.join(T, "template", "scenarios.yml"), encoding="utf-8").read()
want("uses: pingledingle-tech/PinglyMotion/.github/workflows/rlv-scenarios.yml@COMMIT" in wf, "template/scenarios.yml must call the shared workflow")
want("engine_ref: COMMIT" in wf, "template/scenarios.yml must pass engine_ref")
want("contents: read" in wf, "template/scenarios.yml must ask for read permission only")
shared = open(os.path.join(ROOT, ".github", "workflows", "rlv-scenarios.yml"), encoding="utf-8").read()
for needle in ("workflow_call:", "engine_ref:", "The failing examples must fail", "--objects scripts"):
    want(needle in shared, "the shared workflow must have: " + needle)
want("contents: read" in shared and "write" not in re.sub(r"[^\n]*#[^\n]*", "", shared).split("permissions:")[1].split("jobs:")[0], "the shared workflow must have read permission only")

# with the runner: what the workflow does
if len(sys.argv) > 1:
    runner = sys.argv[1]
    env = dict(os.environ)
    env.pop("DISPLAY", None)
    env.pop("QT_QPA_PLATFORM", None)

    def run(*args):
        p = subprocess.run([runner, *args], capture_output=True, text=True, timeout=120, env=env, cwd=T)
        return p.returncode, p.stdout

    code, out = run("--objects", "scripts", "scenarios")
    want(code == 0, "scenarios/ must pass:\n" + out)
    want("PASS cuffs_lock (8 expectations)" in out, "the example product must run against its scripts:\n" + out)
    code, out = run("examples/passing")
    want(code == 0, "examples/passing must pass:\n" + out)
    for f in files("examples/failing", ".rlvtest"):
        code, out = run("examples/failing/" + f)
        want(code == 1, "examples/failing/%s must fail with exit code 1, got %d:\n%s" % (f, code, out))
    code, out = run("--objects", "no_such_folder", "scenarios")
    want(code == 1, "--objects with a missing folder must fail")

if problems:
    print("\n\n".join(problems))
    sys.exit(1)
print("test_rlv_template: ok")
