#!/usr/bin/env python3
"""Makes tools/firestorm/rlv_questions.lsl from reference/rlv/questions.txt (package B).

  python3 tools/firestorm/make_rlv_questions.py            write the script
  python3 tools/firestorm/make_rlv_questions.py --check    exit 1 when the script is not up to date

The script template is rlv_questions.lsl.in (the marker @@LINES@@ is replaced by the list of lines).
Only the words case, mode, object and send are taken from questions.txt; notes and empty lines are left out.
Licensed under the GNU General Public License version 2 or later."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
QUESTIONS = os.path.join(ROOT, "reference", "rlv", "questions.txt")
TEMPLATE = os.path.join(HERE, "rlv_questions.lsl.in")
OUT = os.path.join(HERE, "rlv_questions.lsl")
WORDS = ("case ", "mode ", "object ", "send ")


def lines_of(path):
    out = []
    with open(path, encoding="utf-8") as f:
        for raw in f.read().split("\n"):
            raw = raw.rstrip("\r")
            if not raw.strip() or raw.startswith("#"):
                continue
            if not raw.startswith(WORDS):
                raise SystemExit("%s: a line that is not case, mode, object or send: %r" % (path, raw[:60]))
            out.append(raw)
    return out


def lsl_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\t", "\\t") + '"'


def make():
    with open(TEMPLATE, encoding="utf-8") as f:
        template = f.read()
    lines = lines_of(QUESTIONS)
    body = ",\n".join("    " + lsl_string(l) for l in lines)
    return template.replace("@@LINES@@", body)


if __name__ == "__main__":
    text = make()
    if "--check" in sys.argv:
        try:
            with open(OUT, encoding="utf-8") as f:
                same = f.read() == text
        except OSError:
            same = False
        if not same:
            print("tools/firestorm/rlv_questions.lsl is not what make_rlv_questions.py makes; run it.")
            sys.exit(1)
        print("rlv_questions.lsl is up to date.")
    else:
        with open(OUT, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        print("wrote", OUT)
