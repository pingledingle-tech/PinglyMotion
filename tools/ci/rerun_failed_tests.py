"""Runs the tests that failed in the last ctest run again, one by one, and prints
their Qt Test output from a file. On the Windows runner the output of a Qt Test
program does not always reach ctest, so this makes a failure readable there.
Usage: python tools/ci/rerun_failed_tests.py <build folder>
Always exits 0: the ctest step itself decides pass or fail."""
import json
import os
import subprocess
import sys
import tempfile

build = sys.argv[1]
failed_log = os.path.join(build, "Testing", "Temporary", "LastTestsFailed.log")
if not os.path.exists(failed_log):
    print("no failed tests recorded")
    sys.exit(0)
names = [line.split(":", 1)[1].strip() for line in open(failed_log) if ":" in line]
info = json.loads(subprocess.run(["ctest", "--test-dir", build, "--show-only=json-v1"],
                                 capture_output=True, text=True, check=True).stdout)
for test in info.get("tests", []):
    if test["name"] not in names:
        continue
    env = dict(os.environ)
    cwd = None
    for prop in test.get("properties", []):
        if prop["name"] == "ENVIRONMENT":
            for item in prop["value"]:
                key, _, value = item.partition("=")
                env[key] = value
        elif prop["name"] == "WORKING_DIRECTORY":
            cwd = prop["value"]
    out = os.path.join(tempfile.mkdtemp(), "out.txt")
    cmd = test["command"]
    is_qt = os.path.basename(cmd[0]).startswith("pinglymotion_")
    full = cmd + (["-o", out + ",txt"] if is_qt else [])
    print(f"===== {test['name']}: {' '.join(full)}", flush=True)
    try:
        result = subprocess.run(full, cwd=cwd, env=env, capture_output=True, text=True, timeout=600)
        print(result.stdout[-20000:], result.stderr[-20000:], sep="\n")
        print(f"exit code {result.returncode} ({result.returncode & 0xffffffff:#x})")
        if result.returncode == 0:
            # the output of the failed ctest run itself is lost on the Windows runner, so a test that
            # fails only now and then shows a passing run here (job T2); say so instead of leaving it
            # looking like "all passed but exit code 1"
            print(f"NOTE: this re-run of {test['name']} PASSED, so the failure in the ctest run above did not happen "
                  "again and its output is lost. The test fails only now and then: run it many times "
                  "(cmake -DPINGLYMOTION_REPEAT_TESTS=40, tests/repeat/repeat.cmake) to see the failure.")
    except subprocess.TimeoutExpired:
        print("timed out")
    if os.path.exists(out):
        print(open(out, encoding="utf-8", errors="replace").read()[-20000:])
