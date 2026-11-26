#!/usr/bin/env python3
"""The reference-answer runner pinglymotion_rlvref and the Firestorm question script (package B).
Usage: test_rlvref.py <path of pinglymotion_rlvref>
Checks: reference/rlv passes (known differences are listed, none are new), the three states of known.txt
(tests/rlvref/known, tests/rlvref/stale), broken files give exit 1 with the exact reason, the question
script is what the generator makes, every specification case is also a Firestorm question, and the chat
converter turns a pasted chat into a file the runner reads (and refuses private data).
Licensed under the GNU General Public License version 2 or later."""
import importlib.util
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUNNER = sys.argv[1]
env = dict(os.environ, QT_QPA_PLATFORM="offscreen")
problems = []


def run(*args):
    p = subprocess.run([RUNNER, *args], capture_output=True, text=True, timeout=120, env=env, encoding="utf-8")
    return p.returncode, p.stdout.replace("\r\n", "\n")


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def want(cond, text):
    if not cond:
        problems.append(text)


# 1. the real reference files: nothing new, every file says where it comes from, the known list is read
ref = os.path.join(ROOT, "reference", "rlv")
code, out = run(ref)
want(code == 0, "reference/rlv must pass, exit code %d:\n%s" % (code, out))
want("from the specification, not recorded in Second Life" in out, "a specification file must say so:\n" + out)
want("KNOWN    spec_rlv/outfit_worn_shape (intended)" in out, "the known difference must be listed:\n" + out)
want("0 things to look at, 0 files not read." in out, "the last line must say nothing is left:\n" + out[-300:])
want(not re.search(r"^(NEW|BROKEN|STALE) ", out, re.M), "no NEW, BROKEN or STALE case in reference/rlv:\n" + out)

# 2. the three states
code, out = run(os.path.join(ROOT, "tests", "rlvref", "known"))
want(code == 0, "known states must pass, exit code %d:\n%s" % (code, out))
for line in [
    "SAME     known_states/same_case",
    "KNOWN    known_states/intended_case (intended): The sandbox sits nowhere.",
    "KNOWN    known_states/unclear_case (unclear): Nobody has looked at what a viewer says without a group.",
    "FIXED    known_states/fixed_case: The status answer used to have no slash.",
    '  Line 14: expected 2222: "not the null key", but the sandbox gave 2222: "00000000-0000-0000-0000-000000000000".',
    "4 cases: 1 the same, 1 fixed, 2 known differences (1 intended, 1 unclear), 0 things to look at, 0 files not read.",
]:
    want(line + "\n" in out, "missing line %r in:\n%s" % (line, out))
code, out = run(os.path.join(ROOT, "tests", "rlvref", "stale"))
want(code == 1, "stale/broken/new must exit 1, got %d" % code)
for line in [
    "STALE    stale/now_the_same",
    "  Listed as intended, but the sandbox now gives the same answers. Update known.txt.",
    "BROKEN   stale/broke_again (fixed)",
    "NEW      stale/nobody_listed_this",
    "STALE    stale/not_in_the_file",
    "  known.txt line 3 names a case that is not in the file.",
]:
    want(line + "\n" in out, "missing line %r in:\n%s" % (line, out))

# 3. files that must be refused, with the exact reason
bad = os.path.join(ROOT, "tests", "rlvref", "bad")
expected = {
    "no_header": "  Line 1: the header needs \"viewer\", \"date\" and \"source\" before the first case",
    "silent_alone": "  Line 5: \"silent\" stands alone, directly below a \"send\" line",
    "tests_nothing": "  Line 4: the case \"a\" has no \"answer\" or \"silent\" line, so it tests nothing",
    "wrong_source": "  Line 3: \"source\" is \"recorded\" (in Second Life) or \"specification\", not \"guessed\"",
}
for name, text in expected.items():
    code, out = run(os.path.join(bad, name + ".rlvref"))
    want(code == 1, "%s must exit 1, got %d" % (name, code))
    want(out.split("\n")[:2] == ["FAIL " + name, text], "%s: wrong words:\n%s" % (name, out))
code, out = run(os.path.join(ROOT, "docs"))
want(code == 1, "a folder without .rlvref files must exit 1, got %d" % code)
code, out = run(os.path.join(ROOT, "no_such_folder"))
want(code == 1, "a missing folder must exit 1, got %d" % code)
code, out = run("--known", os.path.join(ROOT, "no_such_known.txt"), ref)
want(code == 1, "a missing known file must exit 1, got %d" % code)

# 4. the Firestorm script is what the generator makes; the specification cases are Firestorm questions
fs = os.path.join(ROOT, "tools", "firestorm")
gen = load("make_rlv_questions", os.path.join(fs, "make_rlv_questions.py"))
with open(os.path.join(fs, "rlv_questions.lsl"), encoding="utf-8") as f:
    want(f.read() == gen.make(), "tools/firestorm/rlv_questions.lsl is not up to date: run tools/firestorm/make_rlv_questions.py")


def cases(path, only_sends=True):
    result, cur = {}, None
    with open(path, encoding="utf-8") as f:
        for raw in f.read().split("\n"):
            if raw.startswith("case "):
                cur = raw[5:].strip()
                result[cur] = []
            elif cur and raw.startswith(("send ", "mode ", "object ")):
                result[cur].append(raw)
    return result


questions = cases(os.path.join(ref, "questions.txt"))
spec = cases(os.path.join(ref, "spec_rlv.rlvref"))
for name, sends in spec.items():
    want(name in questions, "specification case %s is not a Firestorm question" % name)
    want(questions.get(name) == sends, "case %s has other send lines in questions.txt" % name)
# questions are questions only: nothing in them can spend, send, move or wear
allowed = re.compile(r"^send @(version|versionnew|versionnum|getstatus|getstatusall|getsitid|getgroup|getoutfit|getattach|getinv|getinvworn|getcommand|notify|clear|showhovertext(world|hud|all))\b")
for name, lines in questions.items():
    for l in lines:
        want(bool(allowed.match(l)), "questions.txt case %s: %r is not a question or a harmless restriction" % (name, l[:50]))
lsl = open(os.path.join(fs, "rlv_questions.lsl"), encoding="utf-8").read()
for banned in ("llGiveMoney", "llTeleport", "llInstantMessage", "llRezObject", "llSetPayPrice", "llRequestPermissions", "llHTTPRequest", "llEmail", "llSetClickAction"):
    want(banned not in lsl, "rlv_questions.lsl must not use " + banned)

# 5. the chat converter
conv = load("chat_to_rlvref", os.path.join(fs, "chat_to_rlvref.py"))
sample = open(os.path.join(ROOT, "tests", "rlvref", "chat_sample.txt"), encoding="utf-8").read()
text, notes = conv.convert(sample, date="2026-11-26")
want("viewer RestrainedLife viewer v3.4.3 (RLVa 2.4.2.1)\n" in text, "the viewer must come from the @version answer:\n" + text)
want("source recorded\n" in text and "date 2026-11-26\n" in text, "header:\n" + text)
want("send @getgroup=2222\nany 2222\n" in text and "Hemmelig" not in text, "the group name must be hidden:\n" + text)
want("answer 2222 æøåshowhovertextworldæøåshowhovertexthudæøåshowhovertextall\n" in text, "a long answer must be joined:\n" + text)
want(notes == ["1 line(s) with an unknown word were left out."], "notes: %r" % notes)
with tempfile.TemporaryDirectory() as tmp:
    path = os.path.join(tmp, "chat_sample.rlvref")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    code, out = run(path)
    want(out.startswith("Reference chat_sample: RestrainedLife viewer v3.4.3 (RLVa 2.4.2.1), 2026-11-26, recorded in Second Life\n"), "the runner must read the converted file:\n" + out)
    want("FAIL" not in out, "the converted file must be readable:\n" + out)
try:
    # built here, so that no address stands in a file of the repository
    private = "Object: RLVREF|# start, x\nObject: RLVREF|case a\nObject: RLVREF|send @version=2222\nObject: RLVREF|answer 2222 mail " + "someone" + chr(64) + "example.com\n"
    conv.convert(private)
    problems.append("the converter must refuse a line that holds an e-mail address")
except ValueError:
    pass
try:
    conv.convert("nothing here\n")
    problems.append("the converter must refuse text without the script's start line")
except ValueError:
    pass

if problems:
    print("\n\n".join(problems))
    sys.exit(1)
print("test_rlvref: ok")
