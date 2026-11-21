#!/usr/bin/env python3
"""Maintainer tool for the Kimodo helper's hash lock and pinned model (issue 247).

  compile [--out FILE]        resolve requirements.in for 64-bit Windows and Python 3.11 with uv,
                              with a SHA-256 hash for every file (needs uv and the internet)
  validate [FILE]             offline: every package in the lock is pinned and has a hash, and the
                              versions agree with kimodo_helper.py
  tamper-test --uv UV [FILE]  installs one small package of the lock with --require-hashes (must
                              work), then with one hash changed (must fail)
  model-revision [--check]    prints the newest commit of the Kimodo model on Hugging Face;
                              --check only checks that the pinned MODEL_REVISION exists

The AI sessions' sandboxes cannot reach PyPI's archives, GitHub's archives or Hugging Face, so
compile and model-revision run in the manual workflow .github/workflows/kimodo-lock.yml, which
prints both results. Standard library only.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import kimodo_helper as kh  # noqa: E402

INPUT = os.path.join(HERE, "requirements.in")
LOCK = kh.LOCK_FILE
PYTHON_VERSION = "3.11"
PLATFORM = "x86_64-pc-windows-msvc"
HASH_RE = re.compile(r"--hash=sha256:([0-9a-f]{64})$")
PIN_RE = re.compile(r"^([A-Za-z0-9][A-Za-z0-9._-]*)==([A-Za-z0-9.+!_-]+)$")
URL_RE = re.compile(r"^([A-Za-z0-9][A-Za-z0-9._-]*) @ (https://\S+)$")


def parse_lock(text):
    """The requirements of a lock file: a list of (name, spec, [hashes], line number)."""
    entries = []
    logical, start = "", 0
    for number, raw in enumerate(text.splitlines(), 1):
        line = raw.split(" #", 1)[0].rstrip() if not raw.lstrip().startswith("#") else ""
        if not logical:
            start = number
        if line.endswith("\\"):
            logical += line[:-1] + " "
            continue
        logical += line
        parts = logical.split()
        logical = ""
        if not parts:
            continue
        hashes = [p for p in parts if p.startswith("--hash=")]
        rest = [p for p in parts if not p.startswith("--hash=")]
        spec = " ".join(rest)
        m = PIN_RE.match(spec) or URL_RE.match(spec)
        name = m.group(1).lower().replace("_", "-") if m else spec
        entries.append((name, spec, hashes, start))
    return entries


def validate(text):
    """Problems found in a lock file (an empty list when it is fine)."""
    problems = []
    entries = parse_lock(text)
    if not entries:
        return ["the lock is empty"]
    names = {}
    for name, spec, hashes, line in entries:
        if spec.startswith("-"):
            problems.append("line %d: options are not allowed in the lock (%s)" % (line, spec))
            continue
        if not (PIN_RE.match(spec) or URL_RE.match(spec)):
            problems.append("line %d: not pinned to one exact version: %s" % (line, spec))
        if not hashes:
            problems.append("line %d: %s has no hash" % (line, name))
        for h in hashes:
            if not HASH_RE.match(h):
                problems.append("line %d: %s has a hash that is not SHA-256: %s" % (line, name, h))
        if name in names:
            problems.append("line %d: %s is listed twice" % (line, name))
        names[name] = spec
    torch = names.get("torch", "")
    want = kh.TORCH_PACKAGE.split("==")[1]
    if not re.match(r"^torch==%s(\+cu\d+)?$" % re.escape(want), torch):
        problems.append("torch is not %s in the lock (found: %s)" % (kh.TORCH_PACKAGE, torch or "nothing"))
    if names.get("kimodo") != kh.KIMODO_PACKAGE:
        problems.append("kimodo is not the pinned commit %s in the lock" % kh.KIMODO_COMMIT)
    return problems


def compile_lock(out, uv):
    cmd = [uv, "pip", "compile", INPUT, "--generate-hashes", "--python-version", PYTHON_VERSION,
           "--python-platform", PLATFORM, "--index-url", kh.PYPI_INDEX, "--extra-index-url", kh.TORCH_INDEX,
           "--index-strategy", "unsafe-best-match", "--no-header", "--no-annotate", "-o", out]
    print("+ " + " ".join(cmd))
    subprocess.run(cmd, check=True)
    header = ("# Kimodo helper: every package pinned and checked by SHA-256 (issue 247).\n"
              "# Made by: python3 tools/kimodo/kimodo_lock.py compile (workflow kimodo-lock.yml).\n"
              "# For 64-bit Windows and Python %s. Do not edit by hand.\n" % PYTHON_VERSION)
    with open(out, encoding="utf-8") as f:
        body = f.read()
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(header + body)
    problems = validate(header + body)
    for p in problems:
        print("PROBLEM: " + p)
    return 1 if problems else 0


def tamper_test(lock_text, uv):
    """One small pure-Python package from the lock: installs with its hashes, fails with one changed."""
    small = ["typing-extensions", "filelock", "packaging", "mpmath", "six", "idna"]
    entries = {name: (spec, hashes) for name, spec, hashes, _ in parse_lock(lock_text)}
    pick = next((n for n in small if n in entries), None)
    if not pick:
        print("No small package to test with in the lock.")
        return 1
    spec, hashes = entries[pick]
    # every file's hash changed in its last digit, so no file of the package can match
    changed = [h[:-1] + ("0" if h[-1] != "0" else "1") for h in hashes]
    results = {}
    for label, hs in (("right", hashes), ("changed", changed)):
        tmp = tempfile.mkdtemp(prefix="kimodo-lock-")
        try:
            req = os.path.join(tmp, "req.txt")
            with open(req, "w") as f:
                f.write(spec + " " + " ".join(hs) + "\n")
            venv = os.path.join(tmp, "venv")
            subprocess.run([uv, "venv", "-q", venv], check=True)
            py = os.path.join(venv, "Scripts" if os.name == "nt" else "bin", "python")
            r = subprocess.run([uv, "pip", "install", "--python", py, "--no-cache", "--require-hashes", "--no-deps",
                                "--index-url", kh.PYPI_INDEX, "-r", req], capture_output=True, text=True)
            results[label] = r.returncode
            print("%s hashes for %s: exit code %d" % (label, spec, r.returncode))
            if r.returncode:
                print((r.stderr or r.stdout).strip()[-600:])
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
    if results.get("right") != 0:
        print("FAIL: the install with the right hashes did not work")
        return 1
    if results.get("changed") == 0:
        print("FAIL: the install with a changed hash worked; hash checking is not on")
        return 1
    print("OK: hash checking refuses a changed hash")
    return 0


def hf_get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "pinglymotion-kimodo-lock"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.load(r)


def model_revision(check):
    base = "https://huggingface.co/api/models/%s" % kh.MODEL_REPO
    if check:
        if not re.match(r"^[0-9a-f]{40}$", kh.MODEL_REVISION):
            print("MODEL_REVISION is not a full commit hash: %s" % kh.MODEL_REVISION)
            return 1
        try:
            info = hf_get(base + "/revision/" + kh.MODEL_REVISION)
        except urllib.error.HTTPError as e:
            print("The pinned model revision was not found (HTTP %d)." % e.code)
            return 1
        print("pinned model revision exists: %s %s" % (kh.MODEL_REPO, info.get("sha")))
        return 0 if info.get("sha") == kh.MODEL_REVISION else 1
    info = hf_get(base)
    print("MODEL_REPO %s" % kh.MODEL_REPO)
    print("MODEL_REVISION_NEWEST %s" % info.get("sha"))
    print("lastModified %s" % info.get("lastModified"))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("compile")
    c.add_argument("--out", default=LOCK)
    c.add_argument("--uv", default="uv")
    v = sub.add_parser("validate")
    v.add_argument("file", nargs="?", default=LOCK)
    t = sub.add_parser("tamper-test")
    t.add_argument("--uv", default="uv")
    t.add_argument("file", nargs="?", default=LOCK)
    m = sub.add_parser("model-revision")
    m.add_argument("--check", action="store_true")
    a = ap.parse_args(argv)
    if a.cmd == "compile":
        return compile_lock(a.out, a.uv)
    if a.cmd == "model-revision":
        return model_revision(a.check)
    try:
        with open(a.file, encoding="utf-8") as f:
            text = f.read()
    except OSError:
        print("No lock file: %s" % a.file)
        return 1
    if a.cmd == "validate":
        problems = validate(text)
        for p in problems:
            print("PROBLEM: " + p)
        if not problems:
            print("lock is valid: %d packages, all pinned with hashes" % len(parse_lock(text)))
        return 1 if problems else 0
    return tamper_test(text, a.uv)


if __name__ == "__main__":
    sys.exit(main())
