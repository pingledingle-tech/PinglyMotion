#!/usr/bin/env python3
"""Turns the chat text of the Firestorm question script into a reference file (package B).

  python3 tools/firestorm/chat_to_rlvref.py chat.txt --out reference/rlv/firestorm_<date>.rlvref \
      [--viewer "Firestorm 7.1.14 (RLVa 2.4.2)"] [--date 2026-11-26]

The input is the text copied from nearby chat after tools/firestorm/rlv_questions.lsl ran. Only the
lines that hold "RLVREF|" count; what stands before it on the line (time, object name) is thrown away,
and a line that starts with "+" after it goes on the line before (long answers come in pieces). The
result is a .rlvref file with source "recorded" (src/scriptlab/rlvreference.h). Then run
pinglymotion_rlvref on it and put each difference in reference/rlv/known.txt, or fix the sandbox.

Rules, so nothing private is written:
  - the answers of @getgroup, @getinv and @getinvworn can hold names of the user's group or inventory;
    they are written as "any <channel>" (an answer came, whatever it said);
  - the text from outside is only read: it is never run, and a line that is not one of the known words
    (case, mode, object, send, answer, silent) is left out and counted;
  - a line that looks like an e-mail address, a web address or a key (UUID) other than the null key is
    refused, so the user looks at the chat text before it goes into the public repository.
Licensed under the GNU General Public License version 2 or later."""
import argparse
import datetime
import re
import sys

MARK = "RLVREF|"
KNOWN = ("case", "mode", "object", "send", "answer", "prefix", "number", "any", "silent")
PERSONAL_QUESTIONS = ("@getgroup", "@getinv:", "@getinv=", "@getinvworn")
NULL_KEY = "00000000-0000-0000-0000-000000000000"
SUSPECT = re.compile(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+\.[A-Za-z]{2,}|https?://|secondlife://|[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}")


def pieces(text):
    """The RLVREF lines in order, with "+" lines joined to the line before."""
    out = []
    for raw in text.replace("\r", "").split("\n"):
        i = raw.find(MARK)
        if i < 0:
            continue
        body = raw[i + len(MARK):]
        if body.startswith("+") and out:
            out[-1] += body[1:]
        else:
            out.append(body)
    return out


def convert(text, viewer=None, date=None):
    """Returns (reference text, notes). Raises ValueError for a line that must not be written."""
    notes = []
    lines = []
    version = None
    last_send = ""
    left_out = 0
    started = False
    for body in pieces(text):
        if body.startswith("#"):
            if body.startswith("# start"):
                started = True
            continue
        word = body.split(" ", 1)[0]
        if word not in KNOWN:
            left_out += 1
            continue
        s = SUSPECT.search(body.replace(NULL_KEY, ""))
        if s:
            raise ValueError("a line looks like private data (%r); look at the chat text first: %s" % (s.group(0)[:12] + "...", body[:40]))
        if word == "send":
            last_send = body[5:]
        if word == "answer":
            channel = body.split(" ", 2)[1] if len(body.split(" ", 2)) > 1 else ""
            if any(last_send.startswith(q) for q in PERSONAL_QUESTIONS):
                body = "any " + channel
            elif last_send.startswith("@version=") and version is None:
                version = body.split(" ", 2)[2] if len(body.split(" ", 2)) > 2 else ""
        lines.append(body)
    if not started:
        raise ValueError("no RLVREF start line found: is this the chat text of the script?")
    if left_out:
        notes.append("%d line(s) with an unknown word were left out." % left_out)
    header = [
        "# Reference answers recorded in Second Life with tools/firestorm/rlv_questions.lsl.",
        "# Made by tools/firestorm/chat_to_rlvref.py from the pasted chat text.",
        "viewer %s" % (viewer or version or "unknown viewer"),
        "date %s" % (date or datetime.date.today().isoformat()),
        "source recorded",
        "",
    ]
    out = []
    for body in lines:
        if body.startswith("case "):
            out.append("")
        out.append(body)
    return "\n".join(header + out).replace("\n\n\n", "\n\n") + "\n", notes


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("chat")
    ap.add_argument("--out", required=True)
    ap.add_argument("--viewer")
    ap.add_argument("--date")
    a = ap.parse_args(argv)
    with open(a.chat, encoding="utf-8") as f:
        text = f.read()
    try:
        result, notes = convert(text, a.viewer, a.date)
    except ValueError as e:
        print("Not written: %s" % e)
        return 1
    with open(a.out, "w", encoding="utf-8", newline="\n") as f:
        f.write(result)
    for n in notes:
        print(n)
    print("Wrote %s. Next: pinglymotion_rlvref %s" % (a.out, a.out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
